/*
 * l909_board.cpp — see l909_board.h. AGPL-3.0 (with Lemus).
 *
 * Threads. The board belongs to one thread at a time: the audio thread at latency 0, the render-
 * ahead thread ("producer") otherwise; `owner` hands it over with release/acquire. Hits go to the
 * board's owner through a single-producer queue, each stamped with the block it starts in; the
 * producer renders block j once the audio thread has said blocks up to j may be rendered
 * (`target`), into a ring of rendered blocks the audio thread copies out.
 *
 * Blocks are counted from the first render: c is the block the audio thread plays next. At
 * latency R a render call's hits start block max(c + R, the last stamp), so lowering R never puts a
 * hit in a block already rendered; it gets the old latency until the playhead catches up.
 */
#include "l909_board.h"

#include "tr909/core/parallel.h"
#include "tr909/core/voiceboard.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <semaphore.h>
#include <thread>
#include <time.h>
#include <vector>

#if defined(__has_include)
#if __has_include("fast_cores.h")
#include "fast_cores.h"
#define L909_FASTCORES 1
#endif
#endif

#ifndef L909_WORKERS
#define L909_WORKERS 2 /* solver workers besides the rendering thread (Live III: 2 as good as 3) */
#endif

namespace {

using namespace tr909;

constexpr int B = L909_BLOCK;
constexpr int kRing = 8;      // rendered blocks kept (more than L909_MAX_LATENCY + 1)
constexpr int kQueue = 1024;  // hits in flight
constexpr int kPending = 64;  // hits between two render calls
// The trigger pulse: the 909's CPU holds a trigger from one panel scan's write to the next, 1.92-2.11 ms
// (Lemus docs/firmware.md, "Timing"): 2 ms.
constexpr int kPulse = 88;

// The latches (VoiceBoard::write): per voice 0-9, then the open hat (10)
const int kDac[11] = {1, 2, 3, 4, 5, 6, 6, 7, 8, 8, 7};
const int kReg[11] = {9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10};
const int kBit[11] = {1, 2, 3, 4, 5, 1, 6, 2, 4, 5, 2};

struct Hit {
	long block;
	int voice, code;
	float tag;
};

struct Slot {
	std::atomic<long> stamp{-1};
	float lane[L909_LANES][B];
	int hitVoice[L909_LANES];
	float tag[L909_LANES];
};

uint32_t crc32(const uint8_t *p, size_t n)
{
	uint32_t c = 0xFFFFFFFFu;
	for (size_t i = 0; i < n; ++i) {
		c ^= p[i];
		for (int k = 0; k < 8; ++k)
			c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
	}
	return ~c;
}

void setPriority(int p)
{
#ifdef L909_FASTCORES
	p = fastcores::rt_priority(p);
#endif
	sched_param sp{};
	sp.sched_priority = p;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp); // best effort: may be refused
}

Controls toControls(const l909_controls_t &k)
{
	auto c01 = [](float x) { return double(x < 0 ? 0 : x > 1 ? 1 : x); };
	Controls c;
	c.bdTune = c01(k.bd_tune);
	c.bdAttack = c01(k.bd_attack);
	c.bdDecay = c01(k.bd_decay);
	c.sdTune = c01(k.sd_tune);
	c.sdTone = c01(k.sd_tone);
	c.sdSnappy = c01(k.sd_snappy);
	c.ltTune = c01(k.lt_tune);
	c.ltDecay = c01(k.lt_decay);
	c.mtTune = c01(k.mt_tune);
	c.mtDecay = c01(k.mt_decay);
	c.htTune = c01(k.ht_tune);
	c.htDecay = c01(k.ht_decay);
	c.chDecay = c01(k.ch_decay);
	c.ohDecay = c01(k.oh_decay);
	c.crTune = c01(k.cr_tune);
	c.rdTune = c01(k.rd_tune);
	return c;
}

} // namespace

struct l909 {
	VoiceBoard board;
	Parallel *par = nullptr;
	std::vector<uint8_t> rom[3];
	bool romOk[3] = {false, false, false};

	// controls: a seqlock the audio thread writes and the board's owner reads
	std::atomic<unsigned> cseq{0};
	l909_controls_t cbuf{};
	unsigned cApplied = 0;

	// hits: the audio thread pushes, the board's owner pops
	Hit queue[kQueue];
	std::atomic<unsigned> qHead{0}, qTail{0};

	Slot ring[kRing];
	Slot inl; // latency 0's block

	std::atomic<int> owner{0}; // 0 the audio thread, 1 the producer
	std::atomic<bool> stopReq{false};
	std::atomic<long> target{0};   // the producer may render blocks below this
	std::atomic<long> produced{0}; // the next block the producer renders (its board's time)
	std::atomic<bool> quit{false};
	std::atomic<long> underruns{0};
	sem_t sem;
	std::thread producer;

	// the audio thread's own
	long c = 0;         // the block it plays next
	int pos = 0;        // frames of block c already played
	long lastStamp = -1;
	int R = 0;          // the latency in force
	std::atomic<int> reqR{0};
	bool draining = false; // going to latency 0: playing out the rendered blocks
	Hit pending[kPending];
	int npending = 0;
	const Slot *cur = nullptr;
	Slot silence;

	// ---- the board's owner
	void applyControls()
	{
		const unsigned s0 = cseq.load(std::memory_order_acquire);
		if (s0 == cApplied || (s0 & 1u))
			return;
		l909_controls_t k;
		std::memcpy(&k, &cbuf, sizeof k);
		std::atomic_thread_fence(std::memory_order_acquire);
		if (cseq.load(std::memory_order_relaxed) != s0)
			return; // written meanwhile: next block
		board.setControls(toControls(k));
		cApplied = s0;
	}
	void startHit(const Hit &h, Slot &s, bool &any)
	{
		const int v = h.voice;
		if (v < 0 || v > 10)
			return;
		const int code = h.code < 0 ? 0 : h.code > 63 ? 63 : h.code;
		board.write(kDac[v], uint8_t(code << 1));
		if (v == 7 || v == L909_OH) // CLOSED/OPEN, a held level
			board.write(10, uint8_t((board.latch(10) & ~0x08) | (v == 7 ? 0x08 : 0)));
		board.write(kReg[v], uint8_t(board.latch(kReg[v]) | (1u << kBit[v])));
		const int lane = v == L909_OH ? 7 : v;
		s.hitVoice[lane] = v;
		s.tag[lane] = h.tag;
		any = true;
	}
	bool popHit(long upTo, Hit &h)
	{
		const unsigned t = qTail.load(std::memory_order_relaxed);
		if (t == qHead.load(std::memory_order_acquire) || queue[t % kQueue].block > upTo)
			return false;
		h = queue[t % kQueue];
		qTail.store(t + 1, std::memory_order_release);
		return true;
	}
	// One block of the board into s: the hits stamped up to j (and `extra`), then the trigger pulse.
	void renderBlock(long j, Slot &s, const Hit *extra, int nextra)
	{
		applyControls();
		for (int l = 0; l < L909_LANES; ++l)
			s.hitVoice[l] = -1;
		bool any = false;
		Hit h;
		while (popHit(j, h))
			startHit(h, s, any);
		for (int i = 0; i < nextra; ++i)
			startHit(extra[i], s, any);
		float L[B], Rt[B];
		float *jp[L909_LANES];
		for (int l = 0; l < L909_LANES; ++l)
			jp[l] = s.lane[l];
		if (!any) {
			board.render(L, Rt, B, jp);
			return;
		}
		board.render(L, Rt, kPulse, jp);
		board.write(9, 0);
		board.write(10, uint8_t(board.latch(10) & 0x08));
		for (int l = 0; l < L909_LANES; ++l)
			jp[l] = s.lane[l] + kPulse;
		board.render(L, Rt, B - kPulse, jp);
	}

	void produce()
	{
#ifdef L909_FASTCORES
		fastcores::pin_this_thread(); // before the workers start: they inherit it
#endif
		setPriority(14);
		while (!quit.load(std::memory_order_acquire)) {
			if (owner.load(std::memory_order_acquire) != 1) {
				wait();
				continue;
			}
			if (stopReq.load(std::memory_order_acquire)) {
				stopReq.store(false, std::memory_order_relaxed);
				owner.store(0, std::memory_order_release);
				continue;
			}
			const long j = produced.load(std::memory_order_relaxed);
			if (j < target.load(std::memory_order_acquire)) {
				Slot &s = ring[j % kRing];
				renderBlock(j, s, nullptr, 0);
				s.stamp.store(j, std::memory_order_release);
				produced.store(j + 1, std::memory_order_release);
				continue;
			}
			wait();
		}
	}
	void wait()
	{
		timespec t;
		clock_gettime(CLOCK_REALTIME, &t);
		t.tv_nsec += 5000000;
		if (t.tv_nsec >= 1000000000) {
			t.tv_nsec -= 1000000000;
			++t.tv_sec;
		}
		sem_timedwait(&sem, &t);
	}

	// ---- the audio thread
	void post()
	{
		const long b = std::max(lastStamp, c + R + (pos ? 1 : 0));
		for (int i = 0; i < npending; ++i) {
			const unsigned h = qHead.load(std::memory_order_relaxed);
			if (h - qTail.load(std::memory_order_acquire) >= unsigned(kQueue))
				break; // full: drop
			Hit e = pending[i];
			e.block = b;
			queue[h % kQueue] = e;
			qHead.store(h + 1, std::memory_order_release);
		}
		npending = 0;
		lastStamp = b;
		target.store(b + 1, std::memory_order_release);
		sem_post(&sem);
	}
	// The block c to play, at its start.
	const Slot *next()
	{
		if (!draining && owner.load(std::memory_order_acquire) == 0) { // ours: render it here
			renderBlock(c, inl, pending, npending);
			npending = 0;
			return &inl;
		}
		const Slot &s = ring[c % kRing];
		if (s.stamp.load(std::memory_order_acquire) == c)
			return &s;
		underruns.fetch_add(1, std::memory_order_relaxed);
		return &silence;
	}
	void render(int n, float *const lanes[L909_LANES], int hitVoice[L909_LANES], float tag[L909_LANES])
	{
		for (int l = 0; l < L909_LANES; ++l)
			hitVoice[l] = -1;
		const int want = reqR.load(std::memory_order_relaxed);
		const bool mine = owner.load(std::memory_order_acquire) == 0;
		if (!mine && !draining) {
			if (want == 0) { // stop the producer; play what it rendered, then render here
				draining = true;
				stopReq.store(true, std::memory_order_release);
				sem_post(&sem);
			} else {
				R = want;
			}
		}
		if (!mine) // hits to the producer (while draining, those it won't reach wait in the queue for us)
			post();

		int done = 0;
		while (done < n) {
			if (pos == 0) {
				if (draining && owner.load(std::memory_order_acquire) == 0 && produced.load(std::memory_order_acquire) <= c) {
					draining = false; // the producer has stopped and its blocks are played: ours from here
					R = 0;
				}
				cur = next();
				for (int l = 0; l < L909_LANES; ++l)
					if (cur->hitVoice[l] >= 0) {
						hitVoice[l] = cur->hitVoice[l];
						tag[l] = cur->tag[l];
					}
			}
			const int k = std::min(n - done, B - pos);
			for (int l = 0; l < L909_LANES; ++l)
				std::memcpy(lanes[l] + done, cur->lane[l] + pos, sizeof(float) * size_t(k));
			done += k;
			pos += k;
			if (pos == B) {
				pos = 0;
				++c;
			}
		}
		// latency 0 -> 1-3: block c-1 was rendered here; the producer takes the board from block c
		if (owner.load(std::memory_order_relaxed) == 0 && !draining && want > 0 && pos == 0) {
			R = want;
			produced.store(c, std::memory_order_relaxed);
			lastStamp = c - 1;
			target.store(c + R + 1, std::memory_order_relaxed);
			owner.store(1, std::memory_order_release);
			sem_post(&sem);
		}
	}
};

extern "C" {

l909_t *l909_create(float sample_rate, const char *rom_dir, const l909_controls_t *initial)
{
	auto *b = new l909();
	sem_init(&b->sem, 0, 0);
	std::memset(b->silence.lane, 0, sizeof b->silence.lane);
	for (int l = 0; l < L909_LANES; ++l)
		b->silence.hitVoice[l] = -1;
	// the ROMs, by MAME's names and CRCs
	static const char *names[3] = {"hn61256p__c43.ic69", "hn61256p__c42.ic62", "hn61256p__c44.ic54"};
	static const uint32_t crcs[3] = {0x2aaae11bu, 0xdc61cf1au, 0x01a9b435u};
	SampleRoms roms;
	for (int k = 0; k < 3 && rom_dir; ++k) {
		char path[1024];
		std::snprintf(path, sizeof path, "%s/%s", rom_dir, names[k]);
		if (FILE *f = std::fopen(path, "rb")) {
			std::vector<uint8_t> d(32768);
			const size_t got = std::fread(d.data(), 1, d.size(), f);
			std::fclose(f);
			if (got == d.size() && crc32(d.data(), d.size()) == crcs[k]) {
				b->rom[k] = std::move(d);
				b->romOk[k] = true;
			} else {
				std::fprintf(stderr, "9X9: %s is not the 909's ROM (size %zu); that voice falls back\n", path, got);
			}
		}
	}
	roms.hihat = b->romOk[0] ? b->rom[0].data() : nullptr;
	roms.crash = b->romOk[1] ? b->rom[1].data() : nullptr;
	roms.ride = b->romOk[2] ? b->rom[2].data() : nullptr;
	b->board.setSampleRoms(roms);
	b->board.setCalibration(unitCalibration());
	if (initial) {
		b->cbuf = *initial;
		b->board.setControls(toControls(*initial));
	}
	b->board.prepare(sample_rate);
	b->board.reset();

	// the producer starts its workers from its own (pinned) thread
	int workers = L909_WORKERS;
	if (const char *w = std::getenv("L909_WORKERS"))
		workers = std::atoi(w);
	std::atomic<bool> ready{false};
	b->producer = std::thread([b, workers, &ready] {
		if (workers > 0) {
#ifdef L909_FASTCORES
			fastcores::pin_this_thread();
#endif
			b->par = new Parallel(workers, [](int) {
#ifdef L909_FASTCORES
				fastcores::pin_this_thread();
#endif
				setPriority(14);
			});
		}
		ready.store(true, std::memory_order_release);
		b->produce();
	});
	while (!ready.load(std::memory_order_acquire))
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	if (b->par)
		b->board.setParallel(b->par);
	// settle: every circuit at rest before the first hit (about a second)
	float L[B], Rt[B];
	for (int k = 0; k < 350; ++k)
		b->board.render(L, Rt, B);
	return b;
}

void l909_destroy(l909_t *b)
{
	if (!b)
		return;
	b->quit.store(true, std::memory_order_release);
	sem_post(&b->sem);
	if (b->producer.joinable())
		b->producer.join();
	b->board.setParallel(nullptr);
	delete b->par;
	sem_destroy(&b->sem);
	delete b;
}

int l909_rom_ok(const l909_t *b, int which) { return b && which >= 0 && which < 3 && b->romOk[which]; }

void l909_set_controls(l909_t *b, const l909_controls_t *c)
{
	const unsigned s = b->cseq.load(std::memory_order_relaxed);
	b->cseq.store(s + 1, std::memory_order_relaxed); // odd: writing
	std::atomic_thread_fence(std::memory_order_release);
	std::memcpy(&b->cbuf, c, sizeof *c);
	b->cseq.store(s + 2, std::memory_order_release);
}

void l909_set_latency(l909_t *b, int blocks)
{
	b->reqR.store(blocks < 0 ? 0 : blocks > L909_MAX_LATENCY ? L909_MAX_LATENCY : blocks, std::memory_order_relaxed);
}

int l909_latency(const l909_t *b) { return b->R; }

void l909_trigger(l909_t *b, int voice, int code, float tag)
{
	if (b->npending < kPending)
		b->pending[b->npending++] = Hit{0, voice, code, tag};
}

void l909_render(l909_t *b, int n, float *const lanes[L909_LANES], int hit_voice[L909_LANES], float tag[L909_LANES])
{
	b->render(n, lanes, hit_voice, tag);
}

long l909_underruns(const l909_t *b) { return b->underruns.load(std::memory_order_relaxed); }

} // extern "C"
