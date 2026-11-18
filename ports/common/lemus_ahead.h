/*
 * lemus_ahead.h — render-ahead for a circuit-solved voicing board (8X8's l808_board, 9X9's l909_board, TR-MPC).
 *
 * A Core renders whole blocks of kBlock frames and takes commands (hits, knob changes) between them. Ahead<Core>
 * runs it either on the audio thread inside render() (latency 0) or on a thread of its own that many blocks ahead
 * (latency 1-3, kBlock frames each: 2.9 ms at 128 / 44.1 kHz); render() then only copies. Commands posted before a
 * render() call take effect at the block that call's latency puts them in, so a hit sounds `latency` blocks later
 * and nothing else changes: a latency-2 hit is sample-identical to a latency-0 hit two blocks later.
 *
 * The latency can change while playing: between 1 and 3 seamlessly (lowering it never stamps a command into a
 * block already rendered: commands keep the old latency until the playhead catches up); to 0, the blocks already
 * rendered play out first, then the audio thread takes the board; from 0, at the next block boundary.
 *
 * The board belongs to one thread at a time (`owner`, handed over with release/acquire): the audio thread at
 * latency 0, the producer otherwise. Commands go to the owner through a single-producer queue stamped with their
 * block; rendered blocks come back through a ring.
 *
 * Core provides:
 *   static constexpr int kLanes, kBlock;
 *   struct Cmd;                           // POD: what post() carries
 *   struct Info { void clear(); void merge(const Info &later); };   // POD: what a block reports (hits, lanes on)
 *   void apply(const Cmd &c);             // on the board's owner, between blocks
 *   void renderBlock(float *const lanes[kLanes], Info &info);       // kBlock frames
 *   void threadStart();                   // on the producer thread, first (start solver workers from there)
 *
 * Realtime: post(), setLatency() and render() never allocate or lock.
 *
 * AGPL-3.0 (with Lemus).
 */
#ifndef LEMUS_AHEAD_H
#define LEMUS_AHEAD_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <thread>
#include <time.h>

#if defined(__has_include)
#if __has_include("fast_cores.h")
#include "fast_cores.h"
#define LEMUS_AHEAD_FASTCORES 1
#endif
#endif

namespace lemus {

constexpr int kMaxLatency = 3;

// This thread on the fast cores (big.LITTLE), at a SCHED_FIFO priority below MPC's audio threads there
// (mpc-vst-plugins wrapper/fast_cores.h). Best effort.
inline void fastThread(int priority = 14)
{
#ifdef LEMUS_AHEAD_FASTCORES
	fastcores::pin_this_thread();
	priority = fastcores::rt_priority(priority);
#endif
	sched_param sp{};
	sp.sched_priority = priority;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
}

template <class Core>
class Ahead {
public:
	static constexpr int L = Core::kLanes, B = Core::kBlock;
	using Cmd = typename Core::Cmd;
	using Info = typename Core::Info;

	explicit Ahead(Core &core) : m_core(core)
	{
		sem_init(&m_sem, 0, 0);
		std::memset(m_silence.lane, 0, sizeof m_silence.lane);
		m_silence.info.clear();
		std::atomic<bool> ready{false};
		m_producer = std::thread([this, &ready] {
			m_core.threadStart();
			ready.store(true, std::memory_order_release);
			produce();
		});
		while (!ready.load(std::memory_order_acquire))
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	~Ahead()
	{
		m_quit.store(true, std::memory_order_release);
		sem_post(&m_sem);
		if (m_producer.joinable())
			m_producer.join();
		sem_destroy(&m_sem);
	}
	Ahead(const Ahead &) = delete;
	Ahead &operator=(const Ahead &) = delete;

	// The audio thread (the render() caller's thread):
	void post(const Cmd &c)
	{
		if (m_npending < kPending)
			m_pending[m_npending++] = c;
	}
	void setLatency(int blocks) { m_reqR.store(std::max(0, std::min(kMaxLatency, blocks)), std::memory_order_relaxed); }
	int latency() const { return m_R; } // the latency new commands get now
	long underruns() const { return m_underruns.load(std::memory_order_relaxed); }

	// n frames of every lane; info: what the blocks started in these frames reported, merged in order.
	void render(int n, float *const lanes[L], Info &info)
	{
		info.clear();
		const int want = m_reqR.load(std::memory_order_relaxed);
		const bool mine = m_owner.load(std::memory_order_acquire) == 0;
		if (!mine && !m_draining) {
			if (want == 0) { // stop the producer; play what it rendered, then render here
				m_draining = true;
				m_stopReq.store(true, std::memory_order_release);
				sem_post(&m_sem);
			} else {
				m_R = want;
			}
		}
		if (!mine) // commands to the producer (while draining, those it won't reach wait in the queue for us)
			postAll();
		int done = 0;
		while (done < n) {
			if (m_pos == 0) {
				if (m_draining && m_owner.load(std::memory_order_acquire) == 0 &&
				    m_produced.load(std::memory_order_acquire) <= m_c) {
					m_draining = false; // the producer has stopped and its blocks are played: ours from here
					m_R = 0;
				}
				m_cur = next();
				info.merge(m_cur->info);
			}
			const int k = std::min(n - done, B - m_pos);
			for (int l = 0; l < L; ++l)
				std::memcpy(lanes[l] + done, m_cur->lane[l] + m_pos, sizeof(float) * size_t(k));
			done += k;
			m_pos += k;
			if (m_pos == B) {
				m_pos = 0;
				++m_c;
			}
		}
		// latency 0 -> 1-3: block c-1 was rendered here; the producer takes the board from block c
		if (m_owner.load(std::memory_order_relaxed) == 0 && !m_draining && want > 0 && m_pos == 0) {
			m_R = want;
			m_produced.store(m_c, std::memory_order_relaxed);
			m_lastStamp = m_c - 1;
			m_target.store(m_c + m_R + 1, std::memory_order_relaxed);
			m_owner.store(1, std::memory_order_release);
			sem_post(&m_sem);
		}
	}

private:
	static constexpr int kRing = 8;     // rendered blocks kept (more than kMaxLatency + 1)
	static constexpr int kQueue = 1024; // commands in flight
	static constexpr int kPending = 64; // commands between two render calls
	struct Stamped {
		long block;
		Cmd cmd;
	};
	struct Slot {
		std::atomic<long> stamp{-1};
		float lane[L][B];
		Info info;
	};

	Core &m_core;
	Stamped m_queue[kQueue];
	std::atomic<unsigned> m_qHead{0}, m_qTail{0};
	Slot m_ring[kRing];
	Slot m_inline, m_silence;
	std::atomic<int> m_owner{0}; // 0 the audio thread, 1 the producer
	std::atomic<bool> m_stopReq{false}, m_quit{false};
	std::atomic<long> m_target{0};   // the producer may render blocks below this
	std::atomic<long> m_produced{0}; // the next block the producer renders (the board's time)
	std::atomic<long> m_underruns{0};
	std::atomic<int> m_reqR{0};
	sem_t m_sem;
	std::thread m_producer;
	// the audio thread's own
	long m_c = 0;  // the block it plays next
	int m_pos = 0; // frames of block m_c already played
	long m_lastStamp = -1;
	int m_R = 0; // the latency in force
	bool m_draining = false;
	Cmd m_pending[kPending];
	int m_npending = 0;
	const Slot *m_cur = nullptr;

	// ---- the board's owner
	bool pop(long upTo, Cmd &c)
	{
		const unsigned t = m_qTail.load(std::memory_order_relaxed);
		if (t == m_qHead.load(std::memory_order_acquire) || m_queue[t % kQueue].block > upTo)
			return false;
		c = m_queue[t % kQueue].cmd;
		m_qTail.store(t + 1, std::memory_order_release);
		return true;
	}
	// Block j into s: the commands stamped up to j, then `extra` (latency 0's, posted this call), then the render.
	void renderBlock(long j, Slot &s, const Cmd *extra, int nextra)
	{
		Cmd c;
		while (pop(j, c))
			m_core.apply(c);
		for (int i = 0; i < nextra; ++i)
			m_core.apply(extra[i]);
		float *lp[L];
		for (int l = 0; l < L; ++l)
			lp[l] = s.lane[l];
		s.info.clear();
		m_core.renderBlock(lp, s.info);
	}
	void produce()
	{
		while (!m_quit.load(std::memory_order_acquire)) {
			if (m_owner.load(std::memory_order_acquire) != 1) {
				wait();
				continue;
			}
			if (m_stopReq.load(std::memory_order_acquire)) {
				m_stopReq.store(false, std::memory_order_relaxed);
				m_owner.store(0, std::memory_order_release);
				continue;
			}
			const long j = m_produced.load(std::memory_order_relaxed);
			if (j < m_target.load(std::memory_order_acquire)) {
				Slot &s = m_ring[j % kRing];
				renderBlock(j, s, nullptr, 0);
				s.stamp.store(j, std::memory_order_release);
				m_produced.store(j + 1, std::memory_order_release);
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
		sem_timedwait(&m_sem, &t);
	}

	// ---- the audio thread
	void postAll()
	{
		const long b = std::max(m_lastStamp, m_c + m_R + (m_pos ? 1 : 0));
		for (int i = 0; i < m_npending; ++i) {
			const unsigned h = m_qHead.load(std::memory_order_relaxed);
			if (h - m_qTail.load(std::memory_order_acquire) >= unsigned(kQueue))
				break; // full: drop
			m_queue[h % kQueue] = Stamped{b, m_pending[i]};
			m_qHead.store(h + 1, std::memory_order_release);
		}
		m_npending = 0;
		m_lastStamp = b;
		m_target.store(b + 1, std::memory_order_release);
		sem_post(&m_sem);
	}
	// block m_c, at its start
	const Slot *next()
	{
		if (!m_draining && m_owner.load(std::memory_order_acquire) == 0) { // ours: render it here
			renderBlock(m_c, m_inline, m_pending, m_npending);
			m_npending = 0;
			return &m_inline;
		}
		const Slot &s = m_ring[m_c % kRing];
		if (s.stamp.load(std::memory_order_acquire) == m_c)
			return &s;
		m_underruns.fetch_add(1, std::memory_order_relaxed);
		return &m_silence;
	}
};

} // namespace lemus

#endif
