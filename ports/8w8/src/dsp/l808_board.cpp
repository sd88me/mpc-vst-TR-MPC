/*
 * l808_board.cpp — see l808_board.h.
 *
 * AGPL-3.0 (with Lemus).
 */
#include "l808_board.h"

#include "tr808/core/parallel.h"
#include "tr808/core/voiceboard.h"

#include <pthread.h>
#include <sched.h>
#include <cstdlib>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

using tr808::Voice;

namespace l808 {
namespace {

enum Board { MAIN = 0, ALT = 1 };

/* 8W8's lane order (sc808_engine.h): BD SD LT MT HT LC MC HC RS CL MA CP CB CH OH CY */
struct LaneMap { int board; Voice v; uint16_t bit; };
const LaneMap kMap[kLanes] = {
    {MAIN, Voice::BD, tr808::kBitBD}, {MAIN, Voice::SD, tr808::kBitSD},
    {MAIN, Voice::LT, tr808::kBitLT}, {MAIN, Voice::MT, tr808::kBitMT},
    {MAIN, Voice::HT, tr808::kBitHT},
    {ALT,  Voice::LT, tr808::kBitLT}, {ALT,  Voice::MT, tr808::kBitMT},   /* LC MC */
    {ALT,  Voice::HT, tr808::kBitHT},                                     /* HC */
    {MAIN, Voice::RS, tr808::kBitRS}, {ALT,  Voice::RS, tr808::kBitRS},   /* RS CL */
    {ALT,  Voice::CP, tr808::kBitCP}, {MAIN, Voice::CP, tr808::kBitCP},   /* MA CP */
    {MAIN, Voice::CB, tr808::kBitCB}, {MAIN, Voice::CH, tr808::kBitCH},
    {MAIN, Voice::OH, tr808::kBitOH}, {MAIN, Voice::CY, tr808::kBitCY},
};
enum { L_BD, L_SD, L_LT, L_MT, L_HT, L_LC, L_MC, L_HC, L_RS, L_CL, L_MA, L_CP, L_CB, L_CH, L_OH, L_CY };

/*
 * REBUILD knobs: which parts a lane's tune or decay swaps. Tune divides capacitors by the ratio
 * (the frequency follows 1/C); decay scales by d = 2^(2 (pot - default)), 0.25x..4x around the
 * parts as built, multiplied (DECAY_UP) or divided (DECAY_DOWN) depending on which way the part
 * lengthens the note. Bases are Lemus's fitted unit (unitCalibration) where it sets them, else the
 * netlist's value (the late "PORTION CHANGED" snare caps for C58/C61).
 */
enum Kind { TUNE_C, DECAY_UP, DECAY_DOWN };
struct Mod { int lane; const char *key; double base; Kind kind; };
const Mod kMods[] = {
    {L_BD, "bd.c41", 0.015e-6, TUNE_C}, {L_BD, "bd.c42", 0.015e-6, TUNE_C},
    {L_SD, "sd.c57", 0.0068e-6, TUNE_C}, {L_SD, "sd.c58", 0.056e-6, TUNE_C},
    {L_SD, "sd.c59", 0.027e-6, TUNE_C}, {L_SD, "sd.c60", 0.0068e-6, TUNE_C},
    {L_SD, "sd.c61", 0.015e-6, TUNE_C},
    {L_LT, "lt.r230", 838e3, DECAY_UP}, {L_MT, "mt.r259", 862.3e3, DECAY_UP},
    {L_HT, "ht.r286", 850.6e3, DECAY_UP},
    {L_LC, "lt.r230", 838e3, DECAY_UP}, {L_MC, "mt.r259", 862.3e3, DECAY_UP},
    {L_HC, "ht.r286", 850.6e3, DECAY_UP},
    {L_RS, "rs.c116", 0.0047e-6, TUNE_C}, {L_RS, "rs.c115", 0.0047e-6, TUNE_C},
    {L_RS, "rs.c119", 2.213e-9, TUNE_C}, {L_RS, "rs.c117", 2.318e-9, TUNE_C},
    {L_RS, "rs.c118", 2.35e-9, TUNE_C}, {L_RS, "rs.c24", 0.47e-6, DECAY_UP},
    {L_CL, "rs.c119", 2.213e-9, TUNE_C}, {L_CL, "rs.c117", 2.318e-9, TUNE_C},
    {L_CL, "rs.r313", 500e3, DECAY_UP},
    {L_MA, "ma.c131", 0.01e-6, TUNE_C}, {L_MA, "ma.c132", 0.001e-6, TUNE_C},
    {L_MA, "ma.c133", 0.001e-6, TUNE_C},
    {L_MA, "ma.c135", 0.1e-6, DECAY_UP}, {L_MA, "ma.c134", 0.033e-6, DECAY_UP},
    {L_CP, "cp.c128", 5.64e-9, TUNE_C}, {L_CP, "cp.c129", 5.05e-9, TUNE_C},
    {L_CP, "cp.c138", 56.4e-9, DECAY_UP},
    {L_CB, "cb.c9", 0.47e-6, DECAY_UP}, {L_CB, "cb.c34", 1.13e-6, DECAY_UP},
    {L_CH, "oh.c63", 0.47e-6, DECAY_UP},
};
const int kNumMods = int(sizeof kMods / sizeof kMods[0]);

/* What a pair of boards is built from: every REBUILD part, and each board's metal ratio. */
struct Spec {
    double part[kNumMods];
    double metal[2]; /* each board's metal source's tune (ALT's is unused: it plays no metal voice) */
    bool operator==(const Spec &o) const
    {
        return std::memcmp(part, o.part, sizeof part) == 0 && metal[0] == o.metal[0] && metal[1] == o.metal[1];
    }
    bool operator!=(const Spec &o) const { return !(*this == o); }
};

/* Rebuilds are quantised so a knob being turned doesn't queue one per step. */
double quant(double x) { return std::exp(std::round(std::log(x) * 200.0) / 200.0); } /* 0.5 % steps */

struct Pair {
    tr808::VoiceBoard b[2];
    tr808::Controls ctl[2];
    tr808::Calibration cal[2];
    float jack[2][tr808::kVoices][kMaxBlock];
    float mix[kMaxBlock];
    bool pend[2] = {false, false};
    /* Quiet: samples every jack of a board has stayed under kQuietLevel. A board quiet for
     * kQuietTime counts as asleep (skipped) until its next hit: Lemus's own sleep waits for every
     * timing capacitor to settle, seconds after the sound has gone (10.7 s after a full-kit hit). */
    long quiet[2] = {0, 0};
    uint16_t data[2] = {0, 0};
    float accent[2] = {-1, -1};
    LaneKnobs knobs[kLanes];   /* the LIVE knobs the boards were last given (applyKnobs) */

    void build(const Spec &s, double rate)
    {
        for (int bi = 0; bi < 2; ++bi) {
            tr808::Calibration c = tr808::unitCalibration();
            for (int i = 0; i < kNumMods; ++i)
                if (kMap[kMods[i].lane].board == bi)
                    c.parts[kMods[i].key] = s.part[i];
            for (auto &hz : c.metalHz)
                hz *= s.metal[bi];
            cal[bi] = c;
            quietSamples = long(0.25 * rate);
            if (rate != 48000.0)
                b[bi].prepare(rate);       /* setCalibration prepares at the board's rate */
            b[bi].setCalibration(c);
            b[bi].reset();
        }
    }
    /* 2.5 mV at the jack (-72 dB re 10 V, ~60 dB under a hit): Lemus's own sleep floors (the
     * snare's VCOs leak 0.13 mV, the clap's Q37 ~1.3 mV p-p of noise for seconds after a hit) sit
     * just under it; at 100 uV the main board never counted as quiet and rendered every block. */
    static constexpr float kQuietLevel = 2.5e-4f;
    long quietSamples = 12000;                  /* set from the rate: kQuietTime 0.25 s */
    bool resting(int bi) const { return quiet[bi] >= quietSamples || b[bi].asleep(); }
    bool asleep() const { return resting(0) && resting(1) && !pend[0] && !pend[1]; }

    /* Renders silence until both boards rest (at most 3 s of board time): a pair built off the
     * audio thread arrives settled, so it costs the audio thread nothing until its first hit. */
    void settle(double rate)
    {
        float *jk[tr808::kVoices];
        for (int bi = 0; bi < 2; ++bi) {
            for (int v = 0; v < tr808::kVoices; ++v) jk[v] = jack[bi][v];
            for (long done = 0; done < long(3 * rate) && !resting(bi); done += kMaxBlock) {
                b[bi].render(mix, kMaxBlock, jk);
                float peak = 0;
                for (int v = 0; v < tr808::kVoices; ++v)
                    for (int i = 0; i < kMaxBlock; ++i) peak = std::max(peak, std::fabs(jack[bi][v][i]));
                quiet[bi] = peak < kQuietLevel ? quiet[bi] + kMaxBlock : 0;
            }
        }
    }
};

/* A lane's LIVE knobs into a pair's boards. */
void applyKnobs(Pair &p, const LaneKnobs *knobs)
{
    for (int l = 0; l < kLanes; ++l) p.knobs[l] = knobs[l];
    for (int bi = 0; bi < 2; ++bi) {
        tr808::Controls &c = p.ctl[bi];
        c.bdLevel = c.sdLevel = c.ltLevel = c.mtLevel = c.htLevel = 1.0;
        c.rsLevel = c.cpLevel = c.cbLevel = c.cyLevel = c.ohLevel = c.chLevel = 1.0;
        c.bdTone = knobs[L_BD].tone;
        c.bdDecay = knobs[L_BD].decay;
        c.sdTone = knobs[L_SD].decay;      /* 8W8's SD has Decay, no Tone: the hardware's TONE */
        c.sdSnappy = knobs[L_SD].snappy;
        c.ohDecay = knobs[L_OH].decay;
        c.cyDecay = knobs[L_CY].decay;
        const bool alt = bi == ALT;
        c.lc = c.mc = c.hc = c.cl = c.ma = alt;
        c.ltTuning = knobs[alt ? L_LC : L_LT].tunePot;
        c.mtTuning = knobs[alt ? L_MC : L_MT].tunePot;
        c.htTuning = knobs[alt ? L_HC : L_HT].tunePot;
        if (p.accent[bi] >= 0)
            c.accent = p.accent[bi];
        p.b[bi].setControls(c);
    }
    /* the cowbell's TM1/TM2: a trim, no rebuild */
    const tr808::Calibration &c = p.cal[MAIN];
    const double r = knobs[L_CB].ratio;
    p.b[MAIN].setTrims(c.cbHz1 * r, c.cbHz2 * r, c.noiseRms, c.cpOffset);
}

} // namespace

/*
 * The voices of a board run as jobs on worker threads besides the audio thread (Lemus's Parallel:
 * the output is the same to the bit with or without them). On a Live III, MPC's audio workers sit
 * on the four Cortex-A76 cores; ours run there too, at real-time priority just under the audio
 * thread's, so a hit's burst of Newton solves is shared instead of landing on one core.
 * L808_WORKERS overrides the count (0: everything on the audio thread).
 */
#ifndef L808_WORKERS
#define L808_WORKERS 3
#endif

void workerStart(int)
{
    sched_param sp{};
    sp.sched_priority = 70;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);   /* best effort: may be refused */
}

int workerCount()
{
    if (const char *w = std::getenv("L808_WORKERS"))
        return std::atoi(w);
    return L808_WORKERS;
}

struct Engine::Impl {
    double rate;
    tr808::Parallel *par = nullptr;
    LaneKnobs knobs[kLanes];
    bool liveDirty = true;
    /* LIVE knobs reach the boards at most every kLiveEvery samples per lane, and at most one lane
     * per block (round-robin): each change makes the circuits it touches rebuild their matrices on
     * the next step, on the audio thread, and a Q-Link sweep moves knobs every block. Sending every
     * lane's change at once stacked those rebuilds into one block. ~10 ms is still a smooth turn. */
    static constexpr long kLiveEvery = 480;
    LaneKnobs applied[kLanes];   /* the LIVE knobs as the current pair has them */
    long laneWait[kLanes] = {};
    int liveNext = 0;
    int metalLane = L_CH;        /* the CH/OH/CY lane whose tune was moved last: they share the metal source */

    bool laneOn[kLanes] = {};
    Pair *cur = nullptr;
    static constexpr int kRetire = 3;
    Pair *retiring[kRetire] = {};

    /* audio -> worker: the wanted spec */
    std::mutex wantMx;
    Spec want{};
    LaneKnobs wantKnobs[kLanes];  /* the LIVE knobs as they were then, for the build's settle */
    long wantStamp = 0;          /* samples rendered when the spec last changed */
    std::atomic<long> clock{0};
    Spec pendingWant{};          /* audio thread's copy, retried if the lock was busy */
    LaneKnobs pendingKnobs[kLanes];
    bool pendingPush = false;

    /* worker -> audio: a built pair; audio -> worker: pairs to free */
    std::atomic<Pair *> ready{nullptr};
    std::atomic<Pair *> trash[8] = {};

    std::thread worker;
    std::atomic<bool> quit{false};
    Spec built{};

    Spec specFromKnobs() const
    {
        Spec s{};
        for (int i = 0; i < kNumMods; ++i) {
            const Mod &m = kMods[i];
            const LaneKnobs &k = knobs[m.lane];
            double v = m.base;
#if LEMUS_TUNE_REBUILD
            const double d = std::pow(2.0, 2.0 * (double(k.decay) - double(k.decayDef)));
            switch (m.kind) {
            case TUNE_C: v = m.base / quant(k.ratio); break;
            case DECAY_UP: v = m.base * quant(d); break;
            case DECAY_DOWN: v = m.base / quant(d); break;
            }
#endif
            s.part[i] = v;
        }
#if LEMUS_TUNE_REBUILD
        s.metal[MAIN] = quant(knobs[metalLane].ratio);
        s.metal[ALT] = 1.0;
#else
        s.metal[MAIN] = s.metal[ALT] = 1.0;
#endif
        return s;
    }

    void applyLive(Pair &p) { applyKnobs(p, applied); }

    static bool liveDiffers(const LaneKnobs &a, const LaneKnobs &b)
    {
        return a.tunePot != b.tunePot || a.decay != b.decay || a.tone != b.tone || a.snappy != b.snappy ||
               a.ratio != b.ratio;
    }
    /* one lane's pending LIVE change to the current pair, if any lane is due */
    void liveTick(int n)
    {
        for (long &w : laneWait)
            if (w > 0) w -= n;
        if (!liveDirty) return;
        bool pending = false;
        for (int k = 0; k < kLanes; ++k) {
            const int l = (liveNext + k) % kLanes;
            if (!liveDiffers(applied[l], knobs[l])) continue;
            pending = true;
            if (laneWait[l] > 0) continue;
            applied[l] = knobs[l];
            applyLive(*cur);
            laneWait[l] = kLiveEvery;
            liveNext = (l + 1) % kLanes;
            return;   /* stays dirty: the next block looks again */
        }
        liveDirty = pending;
    }

    void work()
    {
        using namespace std::chrono;
        while (!quit.load()) {
            for (auto &t : trash)
                if (Pair *p = t.exchange(nullptr))
                    delete p;
            Spec w;
            LaneKnobs wk[kLanes];
            long stamp;
            {
                std::lock_guard<std::mutex> lk(wantMx);
                w = want;
                std::copy(wantKnobs, wantKnobs + kLanes, wk);
                stamp = wantStamp;
            }
            const long quiet = long(0.15 * rate);
            if (w != built && clock.load() - stamp > quiet && ready.load() == nullptr) {
                Pair *p = new Pair;
                p->build(w, rate);
                applyKnobs(*p, wk);
                p->settle(rate);
                built = w;
                ready.store(p);
            }
            std::this_thread::sleep_for(milliseconds(20));
        }
    }

    void retire(Pair *p)
    {
        for (auto &r : retiring)
            if (!r) { r = p; return; }
        /* full: the oldest goes now (its tails are cut; only on a storm of rebuilds) */
        dispose(retiring[0]);
        for (int i = 1; i < kRetire; ++i) retiring[i - 1] = retiring[i];
        retiring[kRetire - 1] = p;
    }

    void dispose(Pair *p)
    {
        for (auto &t : trash) {
            Pair *e = nullptr;
            if (t.compare_exchange_strong(e, p)) return;
        }
        /* the worker is far behind: leak rather than free on the audio thread */
    }

    void renderPair(Pair &p, int n, float *const lanes[kLanes], bool owner)
    {
        for (int bi = 0; bi < 2; ++bi) {
            tr808::VoiceBoard &b = p.b[bi];
            const bool trig = p.pend[bi];
            /* Asleep (every voice, the trig and the mixer settled), a board's jacks sit within
             * -90 dB of zero (tools/l808_ab): skip it. Its free-running squares and noise stop
             * meanwhile, which only moves their phase at the next hit. Lemus's own sleep still
             * costs ~7 % of an A76 core a board: the CPU-less board's checks, every sample. */
            if (!trig && p.resting(bi))
                continue;
            if (trig)
                p.quiet[bi] = 0;
            if (trig) {
                if (p.accent[bi] >= 0) {
                    p.ctl[bi].accent = p.accent[bi];
                    b.setControls(p.ctl[bi]);
                }
                b.setLines(true, p.data[bi]);
            }
            float *jk[tr808::kVoices];
            for (int v = 0; v < tr808::kVoices; ++v) jk[v] = p.jack[bi][v];
            b.render(p.mix, n, jk);
            if (trig) {
                b.setLines(false, p.data[bi]);
                p.pend[bi] = false;
                p.data[bi] = 0;
                p.accent[bi] = -1;
            }
            (void)owner;
            float peak = 0;
            for (int v = 0; v < tr808::kVoices; ++v)
                for (int i = 0; i < n; ++i)
                    peak = std::max(peak, std::fabs(p.jack[bi][v][i]));
            if (peak < Pair::kQuietLevel)
                p.quiet[bi] += n;
            else
                p.quiet[bi] = 0;
            for (int l = 0; l < kLanes; ++l)
                if (kMap[l].board == bi && b.voiceAwake(kMap[l].v)) {
                    laneOn[l] = true;
                    const float *src = p.jack[bi][int(kMap[l].v)];
                    for (int i = 0; i < n; ++i) lanes[l][i] += src[i];
                }
        }
    }
};

Engine::Engine(double sampleRate, const LaneKnobs *initial) : m(new Impl)
{
    m->rate = sampleRate;
    if (initial)
        for (int l = 0; l < kLanes; ++l) m->knobs[l] = initial[l];
    m->pendingWant = m->specFromKnobs();
    m->built = m->specFromKnobs();
    {
        std::lock_guard<std::mutex> lk(m->wantMx);
        m->want = m->built;
    }
    if (const int w = workerCount(); w > 0)
        m->par = new tr808::Parallel(w, workerStart);
    m->cur = new Pair;
    m->cur->build(m->built, sampleRate);
    for (int l = 0; l < kLanes; ++l) m->applied[l] = m->knobs[l];
    m->applyLive(*m->cur);
    m->cur->settle(sampleRate);
    for (auto &b : m->cur->b) b.setParallel(m->par);
    m->liveDirty = false;
    m->worker = std::thread([this] { m->work(); });
}

Engine::~Engine()
{
    m->quit.store(true);
    m->worker.join();
    delete m->cur;
    delete m->par;
    for (auto *p : m->retiring) delete p;
    delete m->ready.load();
    for (auto &t : m->trash) delete t.load();
    delete m;
}

void Engine::trigger(int lane, float accent01)
{
    if (lane < 0 || lane >= kLanes) return;
    Pair &p = *m->cur;
    const int bi = kMap[lane].board;
    p.pend[bi] = true;
    p.data[bi] |= kMap[lane].bit;
    if (accent01 > 0) {
        p.data[bi] |= tr808::kBitAC;
        if (accent01 > p.accent[bi]) p.accent[bi] = accent01;
    }
}

void Engine::setKnobs(int lane, const LaneKnobs &k)
{
    if (lane < 0 || lane >= kLanes) return;
    LaneKnobs &o = m->knobs[lane];
    if ((lane == L_CH || lane == L_OH || lane == L_CY) && k.ratio != o.ratio) m->metalLane = lane;
    o = k;
    m->liveDirty = true;
    const Spec s = m->specFromKnobs();
    if (s != m->pendingWant) {
        m->pendingWant = s;
        m->pendingPush = true;
    }
    if (m->pendingPush)
        std::copy(m->knobs, m->knobs + kLanes, m->pendingKnobs);
}

void Engine::render(int n, float *const lanes[kLanes])
{
    Impl &im = *m;
    for (int l = 0; l < kLanes; ++l) std::memset(lanes[l], 0, sizeof(float) * size_t(n));
    for (bool &a : im.laneOn) a = false;

    if (im.pendingPush && im.wantMx.try_lock()) {
        im.want = im.pendingWant;
        std::copy(im.pendingKnobs, im.pendingKnobs + kLanes, im.wantKnobs);
        im.wantStamp = im.clock.load();
        im.wantMx.unlock();
        im.pendingPush = false;
    }
    if (Pair *p = im.ready.exchange(nullptr)) {
        /* hits already queued on the old pair move to the new one */
        for (int bi = 0; bi < 2; ++bi) {
            p->pend[bi] = im.cur->pend[bi];
            p->data[bi] = im.cur->data[bi];
            p->accent[bi] = im.cur->accent[bi];
            im.cur->pend[bi] = false;
            im.cur->data[bi] = 0;
        }
        im.retire(im.cur);
        im.cur = p;
        for (auto &b : p->b) b.setParallel(im.par);
        /* it was settled with the worker's snapshot of the knobs: liveTick sends what has moved
         * since, a lane per block, as for any knob */
        for (int l = 0; l < kLanes; ++l) im.applied[l] = p->knobs[l];
        im.liveDirty = true;
    }
    im.liveTick(n);   /* retiring pairs keep their knobs: they only ring out */
    im.renderPair(*im.cur, n, lanes, true);
    for (auto &r : im.retiring) {
        if (!r) continue;
        im.renderPair(*r, n, lanes, false);
        if (r->asleep()) { im.dispose(r); r = nullptr; }
    }
    im.clock.fetch_add(n);
}

bool Engine::active(int lane) const { return lane >= 0 && lane < kLanes && m->laneOn[lane]; }

int Engine::pairs() const
{
    int n = 1;
    for (auto *r : m->retiring) n += r != nullptr;
    return n;
}

} // namespace l808
