/*
 * l909_board.cpp — see l909_board.h. AGPL-3.0 (with Lemus).
 *
 * The board is a lemus::Ahead core (lemus_ahead.h: the render-ahead thread, the command queue, latency changes).
 * A hit is the 909 CPU's write: the voice's accent DAC code and its trigger bit, then the trigger pulse's end 2 ms
 * into the block.
 */
#include "l909_board.h"

#include "lemus_ahead.h"
#include "tr909/core/parallel.h"
#include "tr909/core/voiceboard.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#ifndef L909_WORKERS
#define L909_WORKERS 2 /* solver workers besides the rendering thread (Live III: 2 as good as 3) */
#endif

namespace {

using namespace tr909;

// The trigger pulse: the 909's CPU holds a trigger from one panel scan's write to the next, 1.92-2.11 ms
// (Lemus docs/firmware.md, "Timing"): 2 ms.
constexpr int kPulse = 88;

// The latches (VoiceBoard::write): per voice 0-9, then the open hat (10)
const int kDac[11] = {1, 2, 3, 4, 5, 6, 6, 7, 8, 8, 7};
const int kReg[11] = {9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10};
const int kBit[11] = {1, 2, 3, 4, 5, 1, 6, 2, 4, 5, 2};

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

struct Core {
	static constexpr int kLanes = L909_LANES, kBlock = L909_BLOCK;
	enum { HIT, CONTROLS, MODS };
	enum { kKickPitch = 0, kKickDepth = 1 }; // behave.h KickVco::kModPitch / kModDepth
	struct Cmd {
		int type, voice, code;
		float tag;
		l909_controls_t ctl;
		l909_mods_t mods;
	};
	struct Info {
		int hitVoice[kLanes];
		float tag[kLanes];
		void clear()
		{
			for (int &v : hitVoice)
				v = -1;
		}
		void merge(const Info &o)
		{
			for (int l = 0; l < kLanes; ++l)
				if (o.hitVoice[l] >= 0) {
					hitVoice[l] = o.hitVoice[l];
					tag[l] = o.tag[l];
				}
		}
	};

	VoiceBoard board;
	std::unique_ptr<Parallel> par;
	int workers = L909_WORKERS;
	Info hits; // the hits started since the last block
	bool pulse = false;
	float chTune = 1, ohTune = 1, hhRatio = 1; // hhRatio: the clock as last set (1: as prepared, untouched)

	Core() { hits.clear(); }
	~Core() { board.setParallel(nullptr); }
	void threadStart()
	{
		lemus::fastThread();
		if (workers > 0)
			par.reset(new Parallel(workers, [](int) { lemus::fastThread(); }));
	}
	void apply(const Cmd &c)
	{
		if (c.type == CONTROLS) {
			board.setControls(toControls(c.ctl));
			return;
		}
		if (c.type == MODS) {
			board.setBehaviourMod(Voice::BD, kKickPitch, c.mods.bd_pitch);
			board.setBehaviourMod(Voice::BD, kKickDepth, c.mods.bd_depth);
			chTune = c.mods.ch_tune;
			ohTune = c.mods.oh_tune;
			return;
		}
		const int v = c.voice;
		if (v < 0 || v > 10)
			return;
		if (v == 7 || v == L909_OH) { // the hats share one ROM clock: each hit sets it for its own tune (only if it moves)
			const float r = v == 7 ? chTune : ohTune;
			if (r != hhRatio) {
				board.setHiHatClockRatio(double(r));
				hhRatio = r;
			}
		}
		const int code = c.code < 0 ? 0 : c.code > 63 ? 63 : c.code;
		board.write(kDac[v], uint8_t(code << 1));
		if (v == 7 || v == L909_OH) // CLOSED/OPEN, a held level
			board.write(10, uint8_t((board.latch(10) & ~0x08) | (v == 7 ? 0x08 : 0)));
		board.write(kReg[v], uint8_t(board.latch(kReg[v]) | (1u << kBit[v])));
		const int lane = v == L909_OH ? 7 : v;
		hits.hitVoice[lane] = v;
		hits.tag[lane] = c.tag;
		pulse = true;
	}
	void renderBlock(float *const lanes[kLanes], Info &info)
	{
		info = hits;
		hits.clear();
		float Lm[kBlock], Rm[kBlock];
		if (!pulse) {
			board.render(Lm, Rm, kBlock, lanes);
			return;
		}
		pulse = false;
		board.render(Lm, Rm, kPulse, lanes);
		board.write(9, 0);
		board.write(10, uint8_t(board.latch(10) & 0x08));
		float *rest[kLanes];
		for (int l = 0; l < kLanes; ++l)
			rest[l] = lanes[l] + kPulse;
		board.render(Lm, Rm, kBlock - kPulse, rest);
	}
};

} // namespace

struct l909 {
	Core core;
	std::vector<uint8_t> rom[3];
	bool romOk[3] = {false, false, false};
	std::unique_ptr<lemus::Ahead<Core>> ahead;
	l909_controls_t lastCtl{};
	l909_mods_t lastMods{1, 1, 1, 1};
};

extern "C" {

l909_t *l909_create(float sample_rate, const char *rom_dir, const l909_controls_t *initial)
{
	auto *b = new l909();
	// the ROMs, by MAME's names and CRCs
	static const char *names[3] = {"hn61256p__c43.ic69", "hn61256p__c42.ic62", "hn61256p__c44.ic54"};
	static const uint32_t crcs[3] = {0x2aaae11bu, 0xdc61cf1au, 0x01a9b435u};
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
	SampleRoms roms;
	roms.hihat = b->romOk[0] ? b->rom[0].data() : nullptr;
	roms.crash = b->romOk[1] ? b->rom[1].data() : nullptr;
	roms.ride = b->romOk[2] ? b->rom[2].data() : nullptr;
	VoiceBoard &board = b->core.board;
	board.setSampleRoms(roms);
	board.setCalibration(unitCalibration());
	if (initial) {
		b->lastCtl = *initial;
		board.setControls(toControls(*initial));
	}
	board.prepare(sample_rate);
	board.reset();
	if (const char *w = std::getenv("L909_WORKERS"))
		b->core.workers = std::atoi(w);
	b->ahead.reset(new lemus::Ahead<Core>(b->core)); // starts the producer, which starts the workers
	if (b->core.par)
		board.setParallel(b->core.par.get());
	// settle: every circuit at rest before the first hit (about a second)
	float L[L909_BLOCK], R[L909_BLOCK];
	for (int k = 0; k < 350; ++k)
		board.render(L, R, L909_BLOCK);
	return b;
}

void l909_destroy(l909_t *b)
{
	if (!b)
		return;
	b->ahead.reset(); // the producer stops first
	delete b;
}

int l909_rom_ok(const l909_t *b, int which) { return b && which >= 0 && which < 3 && b->romOk[which]; }

void l909_set_controls(l909_t *b, const l909_controls_t *c)
{
	if (std::memcmp(c, &b->lastCtl, sizeof *c) == 0)
		return;
	b->lastCtl = *c;
	Core::Cmd cmd{};
	cmd.type = Core::CONTROLS;
	cmd.ctl = *c;
	b->ahead->post(cmd);
}

void l909_set_mods(l909_t *b, const l909_mods_t *m)
{
	if (std::memcmp(m, &b->lastMods, sizeof *m) == 0)
		return;
	b->lastMods = *m;
	Core::Cmd cmd{};
	cmd.type = Core::MODS;
	cmd.mods = *m;
	b->ahead->post(cmd);
}

void l909_set_latency(l909_t *b, int blocks) { b->ahead->setLatency(blocks); }

int l909_latency(const l909_t *b) { return b->ahead->latency(); }

void l909_trigger(l909_t *b, int voice, int code, float tag)
{
	Core::Cmd cmd{};
	cmd.type = Core::HIT;
	cmd.voice = voice;
	cmd.code = code;
	cmd.tag = tag;
	b->ahead->post(cmd);
}

void l909_render(l909_t *b, int n, float *const lanes[L909_LANES], int hit_voice[L909_LANES], float tag[L909_LANES])
{
	Core::Info info;
	b->ahead->render(n, lanes, info);
	for (int l = 0; l < L909_LANES; ++l) {
		hit_voice[l] = info.hitVoice[l];
		tag[l] = info.tag[l];
	}
}

long l909_underruns(const l909_t *b) { return b->ahead->underruns(); }

} // extern "C"
