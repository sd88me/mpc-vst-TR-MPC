/*
 * TR-MPC: sixteen slots, each playing any voice from any of the four ported kits (6W6, 8W8, CW-78, 9W9).
 *
 * The four engines are the vendored DSP plus a small per-voice tap (ports/trkit/patches, applied by
 * tools/apply_trkit_patches.sh): with a tap set, an engine renders only its voices and hands each voice's
 * post-fader dry sample to this file. So a slot owns its pan and its reverb and delay sends, and ONE reverb,
 * delay and master stage (8W8's own, run through sc808_fx_stereo) serves the whole kit.
 *   - a slot's Level, Tune, Decay and Drive are the voice's own pots in its engine (get/set proxy to the engine);
 *   - Pan, Rev and Dly are the slot's, mixed here: dry goes to L/R by pan, the sends feed the shared FX buses;
 *   - two slots that pick the SAME voice of the same kit share that voice (one engine voice, retriggered);
 *   - an engine that has been silent for a few seconds is not rendered until the next trigger.
 *
 * Pads: MIDI notes 36..51 -> slots 1..16 (MPC drum-pad default, C1 upward), or 0..15 with the drum-pad patch.
 *
 * Gen2 (TRKIT_LEMUS, the aarch64 build): the 808 and 909 voices are 8X8's and 9X9's circuit-solved voicing boards
 * (Lemus), each rendered by its own thread `latency` blocks ahead (lemus_ahead.h). One kit-wide Latency (0..3 blocks of
 * 128, 2.9 ms each) goes to both, and the 606 and CR-78 hits wait that many blocks so the kit stays in time. Gen1 keeps
 * the 8W8 and 9W9 engines and ignores Latency.
 * GPL-3.0, as the kits it links; the Gen2 build AGPL-3.0, with Lemus.
 */
#include <stdint.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

extern "C" {
#include "engine.h"   /* vendored: mpc-vst-plugins/wrapper/engine.h */
}

#include "trkit_tap.h"
#include "sd606_engine.h"
#include "sc808_engine.h"
#include "cr78_engine.h"
extern "C" {
#include "er99_engine.h"
}

namespace {

trtap::Shared g_tap;   /* see trkit_tap.h; the first Inst to claim g_tap.owner publishes into it */

constexpr int kSlots = 16;
constexpr int kBlock = 128;
constexpr float kSr = 44100.0f;

struct Backend {
    const char *tag;
    int nvoices;
    const char *(*voice_id)(int);
    void *(*create)(const char *dir);
    void (*destroy)(void *);
    void (*trigger)(void *, int voice, int vel);
    void (*render)(void *, float *mono, int n);
    void (*set_tap)(void *, float *tap);   /* nvoices x 128 floats, see the patches */
    int (*set)(void *, const char *key, int pot);
    int (*get)(void *, const char *key, int *pot);
};

/* ---- 6W6 / 8W8 / CW-78 share a C API shape; 9W9 has its own ---- */
#define STRING_API(P, T)                                                                            \
    void *P##_c(const char *) { return P##_create(kSr); }                                           \
    void P##_d(void *e) { P##_destroy((T *)e); }                                                    \
    void P##_t(void *e, int v, int vel) { P##_trigger((T *)e, v, vel); }                            \
    void P##_r(void *e, float *o, int n) { P##_render((T *)e, o, n); }                              \
    void P##_tp(void *e, float *t) { P##_set_tap((T *)e, t); }                                      \
    int P##_s(void *e, const char *k, int p) { char b[16]; snprintf(b, sizeof b, "%d", p); return P##_set_param((T *)e, k, b); } \
    int P##_g(void *e, const char *k, int *p) { char b[32]; if (P##_get_param((T *)e, k, b, sizeof b) <= 0) return 0; *p = atoi(b); return 1; }

STRING_API(sd606, sd606_engine_t)
STRING_API(sc808, sc808_engine_t)
STRING_API(cr78, cr78_engine_t)

const char *kEr99Ids[ER99_NUM_TRIGGERS] = { "bd", "sd", "lt", "mt", "ht", "rs", "hc", "ohh", "chh", "rc", "cr" };
const char *er99_vid(int v) { return (v >= 0 && v < ER99_NUM_TRIGGERS) ? kEr99Ids[v] : ""; }
void *er99_c(const char *dir) {
    er99_engine_t *e = (er99_engine_t *)calloc(1, sizeof(er99_engine_t));
    if (e) er99_engine_init(e, kSr, dir);
    return e;
}
void er99_d(void *e) { if (e) { er99_engine_free((er99_engine_t *)e); free(e); } }
void er99_tp(void *e, float *t) { er99_engine_set_tap((er99_engine_t *)e, t); }
void er99_t(void *e, int v, int vel) { er99_engine_trigger((er99_engine_t *)e, (er99_trigger_t)v, vel); }
void er99_r(void *e, float *o, int n) { er99_engine_render((er99_engine_t *)e, o, n); }
int er99_s(void *e, const char *k, int p) { return er99_engine_set_param((er99_engine_t *)e, k, (float)p); }
int er99_g(void *e, const char *k, int *p) { float f; if (!er99_engine_get_param((er99_engine_t *)e, k, &f)) return 0; *p = (int)lrintf(f); return 1; }

const Backend kBackends[4] = {
    { "606",  SD606_NUM_VOICES, sd606_voice_id, sd606_c, sd606_d, sd606_t, sd606_r, sd606_tp, sd606_s, sd606_g },
    { "808",  SC808_NUM_VOICES, sc808_voice_id, sc808_c, sc808_d, sc808_t, sc808_r, sc808_tp, sc808_s, sc808_g },
    { "CR78", CR78_NUM_VOICES,  cr78_voice_id,  cr78_c,  cr78_d,  cr78_t,  cr78_r,  cr78_tp, cr78_s,  cr78_g },
    { "909",  ER99_NUM_TRIGGERS, er99_vid,      er99_c,  er99_d,  er99_t,  er99_r,  er99_tp, er99_s,  er99_g },
};
constexpr int kNumBackends = 4;
constexpr int kTotalVoices = SD606_NUM_VOICES + SC808_NUM_VOICES + CR78_NUM_VOICES + ER99_NUM_TRIGGERS;

/* flat voice index <-> (backend, voice): backends in the order above, voices in each engine's own order */
void flat_to_bv(int flat, int *b, int *v) {
    for (int i = 0; i < kNumBackends; ++i) {
        if (flat < kBackends[i].nvoices) { *b = i; *v = flat; return; }
        flat -= kBackends[i].nvoices;
    }
    *b = 0; *v = 0;
}

/* which of the slot's knobs forward to which engine key suffix (first key that the engine accepts wins) */
struct Knob { const char *name; const char *suffixes[3]; };
const Knob kKnobs[] = {
    { "level", { "level", "volume", nullptr } },
    { "tune",  { "tune", "pitch", nullptr } },
    { "decay", { "decay", nullptr, nullptr } },
    { "drive", { "drive", nullptr, nullptr } },
    { "dist",  { "dist_type", nullptr, nullptr } },   /* the voice's distortion character, an enum (0..6) */
    /* the voice-specific pots, one control each; a voice without that pot ignores it (the skin only shows the ones it has) */
    { "attack",  { "attack", nullptr, nullptr } },
    { "tone",    { "tone", nullptr, nullptr } },
    { "snappy",  { "snappy", nullptr, nullptr } },
    { "noise",   { "noise", nullptr, nullptr } },
    { "rate",    { "rate", nullptr, nullptr } },
    { "sweep",   { "sweep_depth", nullptr, nullptr } },
    { "pmod",    { "pitch_mod", nullptr, nullptr } },
    { "ndecay",  { "noise_decay", nullptr, nullptr } },
    { "sat",     { "saturation", nullptr, nullptr } },
};
constexpr int kNumKnobs = 14;

/* Default flat voice per slot: every page of four mixes the four kits (kick, snare, hats; kick, snare, clap, rim; toms and
 * bongo; ride, cymbal, cowbell, guiro). Must match DEFAULT_FLAT in tools/gen_trkit_params.py. */
int default_flat(int slot) {
    static const struct { int b, v; } d[kSlots] = {
        {1, 0}, {3, 1}, {0, 4}, {2, 3}, {3, 0}, {0, 1}, {1, 11}, {2, 2},
        {0, 2}, {1, 3}, {3, 4}, {2, 7}, {3, 9}, {0, 6}, {1, 12}, {2, 12},
    };
    int flat = d[slot].v;
    for (int i = 0; i < d[slot].b; ++i) flat += kBackends[i].nvoices;
    return flat;
}

constexpr int kFxBackend = 1;                 /* 8W8's reverb, delay and master stage serve the whole kit */
constexpr int kSleepAfter = 5 * 44100;        /* samples of silence before an engine or the FX stage stops rendering */
constexpr float kSilence = 1e-5f;

constexpr int kPresets = 32, kNameMax = 20;
constexpr int kFactory = 23, kProgs = kFactory + kPresets;   /* the PRESET menu: the factory presets, then the user slots */

struct Inst {
    char dir[512];
    void *eng[kNumBackends];
    float *tap[kNumBackends];  /* per engine: nvoices x kBlock dry samples */
    int quiet[kNumBackends];   /* samples of near-silence since the engine last sounded */
    int fx_quiet;              /* same for the shared FX stage's output */
    int slot_flat[kSlots];
    int edit_slot;             /* 0..15: the slot the editor page's e_* controls (and edit_voice) act on */
    int rnd_sel[kSlots];       /* the randomise module: which slots it acts on, how far, and whether it may change the voice */
    int rnd_amount, rnd_voice;
    int int_rev, int_dly, int_comp;   /* the kit's own reverb / delay / compressor on (1) or off (0: the sends then only leave through a tap) */
    int voice_owner[kNumBackends][32];   /* per engine voice: the slot it was last triggered from; only that slot gets its sound */
    int comp_saved;                   /* the Comp value while int_comp is off (the engine's own is set to 0) */
    uint32_t rng;
    int pan[kSlots], rev[kSlots], dly[kSlots];   /* 0..127; pan 64 = centre */
    int on[kSlots];            /* 0: the slot is switched off (plays nothing, keeps its voice; its kit's engine is only made
                                  for slots that are on) */
    bool tap_owner;            /* this Inst publishes the slot planes for the Tap plugins */
    int16_t pending[kBlock * 2];   /* the owner outputs one block behind what it publishes, so a tap called before it in a period still finds its block */
    bool have_pending;
    int latency;                   /* 0..3 blocks (Gen2): the circuit engines render this far ahead */
    int lat_sent[kNumBackends];    /* what each engine was last told (-1: not yet) */
    struct Held { int b, v, vel, blocks; } held[64];   /* 606 / CR-78 hits waiting out the latency */
    int nheld;
    /* A kit's engine is made when a slot that is on first needs it, on this thread or a non-audio caller's (the audio
     * thread only sets `want` and skips the kit until it exists: an engine reads its samples, and on Gen2 its circuit
     * board takes about a second and starts threads) */
    /* user presets (shared by every project, in <dir>/trmpc-presets.txt): the PRESET menu's 32 slots, each a name and the whole
     * kit's state string (get_state); the drawn naming keyboard */
    int preset;                      /* the picked program: 0..kFactory-1 factory, then the user slots */
    long long created_ms;
    long long load_ms;               /* when set_state last ran (steady clock): the host re-picks the program right after a load */
    int loading;                     /* set_state is running: a "preset" in it only says which was picked */
    char pr_name[kPresets][kNameMax + 1];
    char *pr_state[kPresets];        /* malloc'd; null = an empty slot */
    int kb_open, kb_lower;           /* the keyboard is showing; letters are lower case (SHIFT) */
    char kb_buf[kNameMax + 1];
    char *kb_snap;                   /* the state as it was when SAVE PRESET was pressed */
    struct Maker { std::thread t; std::atomic<unsigned> want{0}; std::atomic<bool> quit{false}; std::mutex mx; } *maker;
};

#ifndef TRKIT_LEMUS
#define TRKIT_LEMUS 0
#endif
/* the engines that render ahead by themselves (Gen2's circuit boards) */
inline bool renders_ahead(int b) { return TRKIT_LEMUS && (b == 1 || b == 3); }

/* the slot's engine and voice; false while the slot is switched off */
bool slot_voice(const Inst *in, int slot, int *b, int *v) {
    if (!in->on[slot] || in->slot_flat[slot] >= kTotalVoices) return false;
    flat_to_bv(in->slot_flat[slot], b, v);
    return true;
}

/* sNN_fam, which picks the slot's faceplate in the skin: 0 6W6, 1 8W8, 2 CW-78, 3 9W9, 4 the 9W9 kick, 5 OFF; the Gen2
 * build adds 6 (its faceplates say 8X8 and 9X9, and so do its voice menus) */
int slot_family(const Inst *in, int slot) {
    int b, v, f = 5;
    if (slot_voice(in, slot, &b, &v)) f = (b == 3 && v == 0) ? 4 : b;
    return f + (TRKIT_LEMUS ? 6 : 0);
}

void *engine_for(Inst *in, int b);
void build_board(void *e, int b) {
#if TRKIT_LEMUS
    if (b == 1) sc808_lemus_build((sc808_engine_t *)e);
    else if (b == 3) er99_engine_lemus_build((er99_engine_t *)e);
#else
    (void)e; (void)b;
#endif
}
void maker_loop(Inst *in) {
    Inst::Maker &mk = *in->maker;
    unsigned done = 0;
    while (!mk.quit.load(std::memory_order_acquire)) {
        const unsigned w = mk.want.load(std::memory_order_acquire) & ~done;
        for (int b = 0; b < kNumBackends && !mk.quit.load(std::memory_order_relaxed); ++b)
            if (w & (1u << b)) {
                if (void *e = engine_for(in, b)) build_board(e, b);
                done |= 1u << b;
            }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}
inline int effective_latency(const Inst *in) { return TRKIT_LEMUS ? in->latency : 0; }

/* the kit's engine if it exists (any thread) */
inline void *eng_get(const Inst *in, int b) { return __atomic_load_n(&in->eng[b], __ATOMIC_ACQUIRE); }

/* the kit's engine, made if need be: never on the audio thread (midi / render use eng_get and `want`) */
void *engine_for(Inst *in, int b) {
    if (void *e = eng_get(in, b)) return e;
    std::lock_guard<std::mutex> lk(in->maker->mx);
    if (!in->eng[b]) {
        void *e = kBackends[b].create(in->dir);
        in->tap[b] = (float *)calloc((size_t)kBackends[b].nvoices * kBlock, sizeof(float));
        if (e && in->tap[b]) kBackends[b].set_tap(e, in->tap[b]);
        __atomic_store_n(&in->eng[b], e, __ATOMIC_RELEASE);   /* after its tap: the audio thread renders it from here */
    }
    return in->eng[b];
}

/* try "<id>_<suffix>", then "<id>_c_<suffix>" (9W9 kick/snare/toms) */
bool forward(Inst *in, int slot, const Knob &k, int *val, bool set) {
    int b, v;   /* on or off: a slot switched off keeps its voice's pots (the engine without its board is light) */
    flat_to_bv(in->slot_flat[slot], &b, &v);
    void *e = engine_for(in, b);
    if (!e) return false;
    const char *id = kBackends[b].voice_id(v);
    const char *const *sfx = k.suffixes;
    char key[48];
    for (int s = 0; s < 3 && sfx[s]; ++s) {
        const char *infix[2] = { "", "c_" };
        for (int c = 0; c < 2; ++c) {
            snprintf(key, sizeof key, "%s_%s%s", id, infix[c], sfx[s]);
            if (set ? kBackends[b].set(e, key, *val) : kBackends[b].get(e, key, val)) return true;
        }
    }
    return false;
}

void presets_load(Inst *in);
void presets_clear(Inst *in);
long long now_ms();

void *create(const char *dir) {
    Inst *in = (Inst *)calloc(1, sizeof(Inst));
    if (!in) return nullptr;
    snprintf(in->dir, sizeof in->dir, "%s", dir ? dir : "");
    for (int s = 0; s < kSlots; ++s) in->slot_flat[s] = default_flat(s);
    for (int s = 0; s < kSlots; ++s) { in->pan[s] = 64; in->rev[s] = 0; in->dly[s] = 0; in->on[s] = 1; }
    in->int_rev = in->int_dly = in->int_comp = 1;
    in->rnd_amount = 64; in->rng = 0x9e3779b9u ^ (uint32_t)(uintptr_t)in;
    presets_load(in);
    in->preset = 0;   /* F01 Init: the layout a new instance starts with */
    in->created_ms = now_ms();   /* a new instance: MPC picks program 0 at once, which must neither apply F01 nor become the picked one */
    in->latency = 2;   /* 5.8 ms: at 1, MPC's audio threads preempting the boards' threads cost missed blocks (Live III) */
    for (int b = 0; b < kNumBackends; ++b) in->lat_sent[b] = -1;
    { const void *none = nullptr; in->tap_owner = g_tap.owner.compare_exchange_strong(none, in); }
    in->maker = new Inst::Maker;
    in->maker->t = std::thread(maker_loop, in);
    /* the other engines are created when a slot first needs one (a hit, a knob, a voice change) */
    engine_for(in, kFxBackend);
    return in;
}

void destroy(void *p) {
    Inst *in = (Inst *)p;
    if (!in) return;
    { const void *me = in; g_tap.owner.compare_exchange_strong(me, nullptr); }
    in->maker->quit.store(true, std::memory_order_release);
    if (in->maker->t.joinable()) in->maker->t.join();   /* a board being built finishes first */
    delete in->maker;
    presets_clear(in);
    free(in->kb_snap);
    for (int b = 0; b < kNumBackends; ++b) { if (in->eng[b]) kBackends[b].destroy(in->eng[b]); free(in->tap[b]); }
    free(in);
}

void midi(void *p, const uint8_t *m, int len) {
    Inst *in = (Inst *)p;
    if (len < 3 || (m[0] & 0xF0) != 0x90 || m[2] == 0) return;   /* one-shots: note-offs ignored */
    /* notes 36-51, or 0-15 (what the MPC OS drum-pad patch sends: pad n = note n-1) */
    const int slot = m[1] < kSlots ? (int)m[1] : (int)m[1] - 36;
    if (slot < 0 || slot >= kSlots) return;
    int b, v;
    if (!slot_voice(in, slot, &b, &v)) return;   /* OFF */
    void *e = eng_get(in, b);
    if (!e) { in->maker->want.fetch_or(1u << b, std::memory_order_release); return; }   /* being made: this hit is lost */
    in->voice_owner[b][v] = slot;
    in->quiet[b] = 0;
    const int lat = effective_latency(in);
    if (lat > 0 && !renders_ahead(b)) {   /* held back to sound with the circuit boards' hits */
        if (in->nheld < (int)(sizeof in->held / sizeof in->held[0])) in->held[in->nheld++] = { b, v, m[2], lat };
        return;
    }
    kBackends[b].trigger(e, v, m[2]);
}

/* keys: sNN_src (flat voice index), sNN_level/tune/decay/drive (the voice's pots), sNN_pan/rev/dly (the slot's),
 * fx_<key> (8W8's reverb, delay and master keys, applied to the shared stage) */
bool parse_slot(const char *key, int *slot, const char **rest) {
    if (key[0] != 's' || key[1] < '0' || key[1] > '9' || key[2] < '0' || key[2] > '9' || key[3] != '_') return false;
    *slot = (key[1] - '0') * 10 + (key[2] - '0') - 1;
    *rest = key + 4;
    return *slot >= 0 && *slot < kSlots;
}

int *owned(Inst *in, int slot, const char *rest) {
    if (!strcmp(rest, "on")) return &in->on[slot];
    if (!strcmp(rest, "pan")) return &in->pan[slot];
    if (!strcmp(rest, "rev")) return &in->rev[slot];
    if (!strcmp(rest, "dly")) return &in->dly[slot];
    return nullptr;
}

void set_state(Inst *in, const char *st);

uint32_t next_rand(Inst *in) {   /* xorshift32 */
    uint32_t x = in->rng ? in->rng : 0x1234567u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return in->rng = x;
}
float unit(Inst *in) { return (next_rand(in) >> 8) * (1.0f / 16777216.0f); }   /* 0..1 */

/* Randomise the selected slots around where they are: each control moves by up to its range x the amount (0..127),
 * Level is left alone, Drive and the sends stay on the quiet side. With the voice option on, a slot also swaps to another voice (of any kit). */
void randomise(Inst *in, bool voices_only = false) {
    const float amt = in->rnd_amount / 127.0f;
    for (int sl = 0; sl < kSlots; ++sl) {
        if (!in->rnd_sel[sl]) continue;
        if (in->rnd_voice || voices_only) {   /* any voice of any kit (the slot then reskins to that kit), never the one it already has */
            int nv = (int)(unit(in) * kTotalVoices) % kTotalVoices;
            if (nv == in->slot_flat[sl]) nv = (nv + 1 + (int)(unit(in) * (kTotalVoices - 1)) % (kTotalVoices - 1)) % kTotalVoices;
            in->slot_flat[sl] = nv;
            int b, v;
            flat_to_bv(nv, &b, &v);
            engine_for(in, b);
        }
        if (voices_only) continue;
        auto wander = [&](const char *name, float range, float lo, float hi, float bias) {
            for (int i = 0; i < kNumKnobs; ++i) {
                if (strcmp(kKnobs[i].name, name)) continue;
                int cur = 0;
                if (!forward(in, sl, kKnobs[i], &cur, false)) return;   /* the voice has no such control */
                float nv = cur + (unit(in) * 2.0f - 1.0f) * range * amt + bias * amt;
                nv = nv < lo ? lo : (nv > hi ? hi : nv);
                int iv = (int)(nv + 0.5f);
                forward(in, sl, kKnobs[i], &iv, true);
            }
        };
        wander("tune", 50, 0, 127, 0); wander("decay", 60, 0, 127, 0); wander("drive", 70, 0, 100, 10);
        wander("attack", 60, 0, 127, 0); wander("tone", 60, 0, 127, 0); wander("snappy", 60, 0, 127, 0);
        wander("noise", 60, 0, 127, 0); wander("rate", 60, 0, 127, 0); wander("sweep", 60, 0, 127, 0);
        wander("pmod", 60, 0, 127, 0); wander("ndecay", 60, 0, 127, 0); wander("sat", 60, 0, 127, 0);
        if (amt > 0.25f) {   /* a different distortion character */
            int d = (int)(unit(in) * 7) % 7;
            for (int i = 0; i < kNumKnobs; ++i) if (!strcmp(kKnobs[i].name, "dist")) forward(in, sl, kKnobs[i], &d, true);
        }
        auto nudge = [&](int &val, float range, float lo, float hi) {
            float nv = val + (unit(in) * 2.0f - 1.0f) * range * amt;
            val = (int)((nv < lo ? lo : (nv > hi ? hi : nv)) + 0.5f);
        };
        nudge(in->pan[sl], 64, 0, 127); nudge(in->rev[sl], 45, 0, 100); nudge(in->dly[sl], 45, 0, 100);
    }
}

/* The editor page's controls act on the edit slot: edit_voice -> sNN_src, e_<knob> -> sNN_<knob> */
bool editor_key(const Inst *in, const char *key, char *out, size_t n) {
    if (!strcmp(key, "edit_voice")) { snprintf(out, n, "s%02d_src", in->edit_slot + 1); return true; }
    if (key[0] == 'e' && key[1] == '_') { snprintf(out, n, "s%02d_%s", in->edit_slot + 1, key + 2); return true; }
    return false;
}

/* ---- user presets ------------------------------------------------------------------------------------------------------------------ */
int get_state(Inst *in, char *buf, int n);
void set_param(void *p, const char *key, const char *val);
long long now_ms() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
constexpr int kStateMax = 12288;

void presets_clear(Inst *in) {
    for (int i = 0; i < kPresets; ++i) { free(in->pr_state[i]); in->pr_state[i] = nullptr; in->pr_name[i][0] = 0; }
}

/* "TRMPP1", then a line per used slot: <slot>\t<name>\t<state> (plain text: names hold keyboard characters only) */
void presets_load(Inst *in) {
    presets_clear(in);
    char path[560];
    snprintf(path, sizeof path, "%s/user/trmpc-presets.txt", in->dir);   /* engine/user: kept on updates (the release lists it as user data) */
    FILE *f = fopen(path, "r");
    if (!f) return;
    char *line = (char *)malloc(kStateMax + 128);
    if (line && fgets(line, kStateMax + 128, f) && !strncmp(line, "TRMPP1", 6))
        while (fgets(line, kStateMax + 128, f)) {
            char *t1 = strchr(line, '\t'); if (!t1) continue;
            char *t2 = strchr(t1 + 1, '\t'); if (!t2) continue;
            const int sl = atoi(line);
            if (sl < 0 || sl >= kPresets) continue;
            *t1 = 0; *t2 = 0;
            size_t len = strlen(t2 + 1);
            while (len && (t2[len] == '\n' || t2[len] == '\r')) t2[len--] = 0;
            snprintf(in->pr_name[sl], sizeof in->pr_name[sl], "%s", t1 + 1);
            free(in->pr_state[sl]);
            in->pr_state[sl] = strdup(t2 + 1);
        }
    free(line);
    fclose(f);
}

void presets_save(Inst *in) {
    char path[560], tmp[580];
    { char d[560]; snprintf(d, sizeof d, "%s/user", in->dir); mkdir(d, 0755); }
    snprintf(path, sizeof path, "%s/user/trmpc-presets.txt", in->dir);
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f) return;
    fputs("TRMPP1\n", f);
    for (int i = 0; i < kPresets; ++i)
        if (in->pr_state[i]) fprintf(f, "%d\t%s\t%s\n", i, in->pr_name[i], in->pr_state[i]);
    fclose(f);
    rename(tmp, path);
}

/* ---- factory presets: a layout of the 16 slots (voices "kit:id", "-" = the slot off; none = the slots stay as they are) and
 * settings applied to all slots (-1 = leave alone), with sends, a pan spread and a few FX values ---- */
#define NOLAYOUT { nullptr }
struct Factory {
    const char *name;           /* 8X8 / 9X9 here read 8W8 / 9W9 on Gen1 */
    const char *voice[16];
    int drive, rev, dly;        /* rev / dly go to the voices `sends` lists (all slots if it is empty) */
    int decay, dist, spread;    /* decay 0..127, distortion type, pan spread 0..64 */
    int latency;                /* -1: leave it */
    const char *sends;          /* voice ids, space separated */
    const char *fx;             /* extra "key=value;" pairs for the shared stage */
};
const Factory kFactoryPresets[kFactory] = {
    { "Init (Four-Kit Mix)", { "808:bd", "909:sd", "606:ch", "CR78:hh", "909:bd", "606:sd", "808:cp", "CR78:rs", "606:lt", "808:mt", "909:ht", "CR78:hb", "909:rc", "606:cy", "808:cb", "CR78:gu" }, 0, 0, 0, -1, -1, -1, -1, "", "" },
    { "All 8X8 Kit", { "808:bd", "808:sd", "808:lt", "808:mt", "808:ht", "808:lc", "808:mc", "808:hc", "808:rs", "808:cl", "808:ma", "808:cp", "808:cb", "808:ch", "808:oh", "808:cy" }, 0, 0, 0, -1, -1, -1, -1, "", "" },
    { "All 9X9 Kit", { "909:bd", "909:sd", "909:lt", "909:mt", "909:ht", "909:rs", "909:hc", "909:ohh", "909:chh", "909:rc", "909:cr", "-", "-", "-", "-", "-" }, 0, 0, 0, -1, -1, -1, -1, "", "" },
    { "All 6W6 Kit", { "606:bd", "606:sd", "606:lt", "606:ht", "606:ch", "606:oh", "606:cy", "606:cp", "-", "-", "-", "-", "-", "-", "-", "-" }, 0, 0, 0, -1, -1, -1, -1, "", "" },
    { "All CW-78 Kit", { "CR78:bd", "CR78:sd", "CR78:rs", "CR78:hh", "CR78:cy", "CR78:ma", "CR78:cl", "CR78:hb", "CR78:lb", "CR78:lc", "CR78:cb", "CR78:tb", "CR78:gu", "CR78:mb", "-", "-" }, 0, 0, 0, -1, -1, -1, -1, "", "" },
    { "909 Kick + 8X8 Kit", { "909:bd", "909:sd", "808:cp", "909:rs", "808:lt", "808:mt", "808:ht", "909:chh", "909:ohh", "808:cb", "909:rc", "909:cr", "808:ma", "808:cl", "808:lc", "808:mc" }, 0, 0, 0, -1, -1, -1, -1, "", "" },
    { "Lo-Fi Box (6W6 + CW-78)", { "606:bd", "CR78:sd", "606:sd", "606:cp", "CR78:cl", "606:ch", "606:oh", "CR78:cy", "606:lt", "606:ht", "CR78:lc", "CR78:cb", "CR78:tb", "CR78:gu", "CR78:ma", "606:cy" }, 40, 0, 0, -1, -1, -1, -1, "", "" },
    { "8X8 Dub Room", { "808:bd", "808:sd", "808:lt", "808:mt", "808:ht", "808:lc", "808:mc", "808:hc", "808:rs", "808:cl", "808:ma", "808:cp", "808:cb", "808:ch", "808:oh", "808:cy" }, 0, 45, 60, -1, -1, -1, -1, "sd rs cl cp cb hc",
      "fx_rev_level=90;fx_rev_decay=80;fx_dly_level=90;fx_dly_fdbk=70;" },
    { "9X9 Warehouse", { "909:bd", "909:sd", "909:lt", "909:mt", "909:ht", "909:rs", "909:hc", "909:ohh", "909:chh", "909:rc", "909:cr", "-", "-", "-", "-", "-" }, 55, 30, 0, -1, -1, -1, -1, "sd hc rs ohh cr",
      "fx_rev_level=80;fx_rev_decay=90;fx_master_drive=40;" },
    { "Boom Bap", { "909:bd", "808:sd", "808:cp", "CR78:hh", "808:oh", "909:rs", "808:lt", "808:mt", "808:ht", "CR78:cl", "808:cb", "CR78:cy", "606:ch", "606:oh", "909:cr", "808:ma" }, 15, 0, 0, -1, -1, -1, -1, "", "" },
    { "Electro", { "808:bd", "808:cp", "606:sd", "808:sd", "808:ch", "808:oh", "808:cl", "808:cb", "808:lt", "808:mt", "808:ht", "808:cy", "606:cp", "909:bd", "808:rs", "808:ma" }, 20, 0, 0, 45, -1, -1, -1, "", "" },
    { "Techno Rack", { "909:bd", "909:hc", "909:chh", "909:ohh", "909:rs", "909:sd", "909:lt", "909:mt", "909:ht", "909:rc", "909:cr", "909:bd", "-", "-", "-", "-" }, 45, 25, 0, -1, -1, -1, -1, "hc ohh cr rc",
      "fx_rev_level=70;fx_rev_decay=60;" },
    { "Latin Perc", { "808:lc", "808:mc", "808:hc", "808:cl", "808:ma", "808:cb", "808:rs", "CR78:lc", "CR78:lb", "CR78:hb", "CR78:cb", "CR78:tb", "CR78:gu", "CR78:mb", "CR78:cl", "CR78:ma" }, 0, 20, 0, -1, -1, -1, -1, "", "" },
    { "Hats and Percs Only", { "606:ch", "606:oh", "606:cy", "808:ch", "808:oh", "808:cy", "CR78:hh", "CR78:cy", "909:chh", "909:ohh", "909:rc", "909:cr", "808:ma", "CR78:ma", "808:cl", "808:cb" }, 0, 0, 0, -1, -1, -1, -1, "", "" },
    { "Kicks and Snares Palette", { "606:bd", "808:bd", "CR78:bd", "909:bd", "606:sd", "808:sd", "CR78:sd", "909:sd", "808:cp", "606:cp", "909:rs", "808:rs", "CR78:rs", "606:lt", "808:lt", "909:lt" }, 0, 0, 0, -1, -1, -1, -1, "", "" },
    { "Dry and Clean", NOLAYOUT, 0, 0, 0, -1, 0, 0, -1, "", "fx_master_drive=0;fx_master_dist=0;" },
    { "Everything Off", { "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-" }, -1, -1, -1, -1, -1, -1, -1, "", "" },
    { "Tape Drive", NOLAYOUT, 70, -1, -1, -1, -1, -1, -1, "", "fx_master_drive=50;" },
    { "Wide Mix", NOLAYOUT, -1, -1, -1, -1, -1, 52, -1, "", "" },
    { "Tight Decays", NOLAYOUT, -1, -1, -1, 25, -1, -1, -1, "", "" },
    { "Open Decays", NOLAYOUT, -1, -1, -1, 100, -1, -1, -1, "", "" },
    { "Live Safe (Latency 2)", NOLAYOUT, -1, -1, -1, -1, -1, -1, 2, "", "" },
    { "Light (Latency 0)", NOLAYOUT, -1, -1, -1, -1, -1, -1, 0, "", "" },
};

int flat_of(const char *spec) {   /* "808:bd" -> the flat voice index, -1 for none */
    const char *c = strchr(spec, ':');
    if (!c) return -1;
    for (int b = 0, base = 0; b < kNumBackends; base += kBackends[b].nvoices, ++b) {
        if (strlen(kBackends[b].tag) != (size_t)(c - spec) || strncmp(kBackends[b].tag, spec, c - spec)) continue;
        for (int v = 0; v < kBackends[b].nvoices; ++v) if (!strcmp(kBackends[b].voice_id(v), c + 1)) return base + v;
    }
    return -1;
}

/* the factory preset as a state string (the same form a user preset keeps) */
void factory_state(int idx, char *buf, int n) {
    const Factory &f = kFactoryPresets[idx];
    int len = snprintf(buf, n, "trmpc1;");
    const bool layout = f.voice[0] != nullptr;
    auto add = [&](int sl, const char *key, int v) { if (len < n - 40) len += snprintf(buf + len, n - len, "s%02d_%s=%d;", sl + 1, key, v); };
    for (int sl = 0; sl < 16; ++sl) {
        int fl = -1;
        bool send = !f.sends[0];   /* no list: every slot */
        if (layout) {
            fl = flat_of(f.voice[sl]);
            if (fl >= 0 && f.sends[0]) {
                const char *id = strchr(f.voice[sl], ':') + 1;
                send = false;
                for (const char *p = f.sends; *p; ) {
                    const char *e = strchr(p, ' '); const size_t l = e ? (size_t)(e - p) : strlen(p);
                    if (strlen(id) == l && !strncmp(id, p, l)) send = true;
                    p += l + (e ? 1 : 0);
                }
            }
            if (fl >= 0) add(sl, "src", fl);
        }
        if (fl >= 0 || !layout) {
            if (f.drive >= 0) add(sl, "drive", f.drive);
            if (f.decay >= 0) add(sl, "decay", f.decay);
            if (f.dist >= 0) add(sl, "dist", f.dist);
        }
        if (layout) add(sl, "pan", 64);
        if (f.spread >= 0) add(sl, "pan", 64 + f.spread * (2 * sl - 15) / 15);
        if (f.rev >= 0 || layout) add(sl, "rev", send && f.rev > 0 ? f.rev : 0);
        if (f.dly >= 0 || layout) add(sl, "dly", send && f.dly > 0 ? f.dly : 0);
        if (layout) add(sl, "on", fl >= 0);
    }
    if (f.latency >= 0 && len < n - 40) len += snprintf(buf + len, n - len, "latency=%d;", f.latency);
    snprintf(buf + len, n - len, "%s", f.fx);
}

void factory_name(int idx, char *buf, int n) {
    const char *nm = kFactoryPresets[idx].name;
    if (!TRKIT_LEMUS) {   /* Gen1 has 8W8 and 9W9 */
        char t[48]; snprintf(t, sizeof t, "%s", nm);
        for (char *p = t; *p; ++p) if ((p[0] == '8' || p[0] == '9') && p[1] == 'X' && p[2] == p[0]) { p[1] = 'W'; p[2] = 'W'; }
        snprintf(buf, n, "F%02d %s", idx + 1, t);
    } else snprintf(buf, n, "F%02d %s", idx + 1, nm);
}

/* a preset's values: everything but what belongs to the project's own bookkeeping */
void apply_state(Inst *in, const char *st, bool factory = false) {
    if (!st || strncmp(st, "trmpc1;", 7)) return;
    st += 7;
    char k[24], v[16];
    while (*st) {
        const char *eq = strchr(st, '='), *end = strchr(st, ';');
        if (!eq || !end || eq > end) break;
        snprintf(k, sizeof k, "%.*s", (int)(eq - st) < 23 ? (int)(eq - st) : 23, st);
        snprintf(v, sizeof v, "%.*s", (int)(end - eq - 1) < 15 ? (int)(end - eq - 1) : 15, eq + 1);
        if (strcmp(k, "edit_slot") && strncmp(k, "rnd_", 4) && (factory || strcmp(k, "latency")) && strcmp(k, "preset")) set_param(in, k, v);
        st = end + 1;
    }
}

void preset_apply(Inst *in, int idx) {
    if (idx < 0 || idx >= kProgs) return;
    in->preset = idx;
    if (idx < kFactory) { char st[3072]; factory_state(idx, st, sizeof st); apply_state(in, st, true); return; }
    apply_state(in, in->pr_state[idx - kFactory]);
}

void keyboard_key(Inst *in, const char *key) {   /* a drawn key was pressed */
    const char *k = key + 3;
    const size_t len = strlen(in->kb_buf);
    if (!strcmp(k, "bksp")) { if (len) in->kb_buf[len - 1] = 0; return; }
    if (!strcmp(k, "shift")) { in->kb_lower = !in->kb_lower; return; }
    if (!strcmp(k, "cancel")) { in->kb_open = 0; free(in->kb_snap); in->kb_snap = nullptr; return; }
    if (!strcmp(k, "save")) {
        if (!in->kb_snap) { in->kb_open = 0; return; }
        char name[kNameMax + 1];
        snprintf(name, sizeof name, "%s", len ? in->kb_buf : "PRESET");
        presets_load(in);   /* the file may have changed under another instance */
        int slot = -1;
        for (int i = 0; i < kPresets && slot < 0; ++i) if (in->pr_state[i] && !strcmp(in->pr_name[i], name)) slot = i;   /* same name: replace it */
        for (int i = 0; i < kPresets && slot < 0; ++i) if (!in->pr_state[i]) slot = i;
        if (slot < 0) slot = in->preset >= kFactory ? in->preset - kFactory : kPresets - 1;   /* all 32 used: the picked one, else the last */
        snprintf(in->pr_name[slot], sizeof in->pr_name[slot], "%s", name);
        free(in->pr_state[slot]);
        in->pr_state[slot] = in->kb_snap;
        in->kb_snap = nullptr;
        in->preset = kFactory + slot;
        presets_save(in);
        in->kb_open = 0;
        return;
    }
    if (len >= (size_t)kNameMax) return;
    char c = 0;
    if (!strcmp(k, "space")) c = ' ';
    else if (!strcmp(k, "dash")) c = '-';
    else if (k[0] && !k[1]) c = (k[0] >= 'a' && k[0] <= 'z') ? (in->kb_lower ? k[0] : (char)(k[0] - 32)) : k[0];
    if (c) { in->kb_buf[len] = c; in->kb_buf[len + 1] = 0; }
}

void preset_key(Inst *in, const char *key) {     /* SAVE PRESET / DELETE PRESET */
    if (!strcmp(key, "pr_save")) {
        presets_load(in);
        free(in->kb_snap);
        in->kb_snap = (char *)malloc(kStateMax);
        if (!in->kb_snap) return;
        get_state(in, in->kb_snap, kStateMax);
        { const int u = in->preset - kFactory; snprintf(in->kb_buf, sizeof in->kb_buf, "%s", u >= 0 && in->pr_state[u] ? in->pr_name[u] : ""); }
        in->kb_lower = 0;
        in->kb_open = 1;
    } else {   /* pr_del: the picked user preset */
        presets_load(in);
        const int u = in->preset - kFactory;   /* factory presets stay */
        if (u >= 0 && u < kPresets && in->pr_state[u]) {
            free(in->pr_state[u]);
            in->pr_state[u] = nullptr;
            in->pr_name[u][0] = 0;
            presets_save(in);
        }
    }
}

void set_param(void *p, const char *key, const char *val) {
    Inst *in = (Inst *)p;
    int slot; const char *rest;
    const int x = atoi(val);
    if (!strcmp(key, "state")) { set_state(in, val); return; }
    if (!strcmp(key, "edit_slot")) { in->edit_slot = x < 0 ? 0 : (x >= kSlots ? kSlots - 1 : x); return; }
    if (!strcmp(key, "latency")) { in->latency = x < 0 ? 0 : (x > 3 ? 3 : x); return; }
    if (!strcmp(key, "preset")) { if (now_ms() - in->created_ms < 2000) return; if (in->loading || now_ms() - in->load_ms < 2000) in->preset = x < 0 ? 0 : (x >= kProgs ? kProgs - 1 : x); else preset_apply(in, x); return; }
    if (!strcmp(key, "pr_save") || !strcmp(key, "pr_del")) { if (x) preset_key(in, key); return; }
    if (!strncmp(key, "kb_", 3)) { if (strcmp(key, "kb_open") && strcmp(key, "kb_name") && x) keyboard_key(in, key); return; }
    if (!strncmp(key, "rnd_", 4)) {
        if (!strcmp(key, "rnd_go")) { if (x) randomise(in); }
        else if (!strcmp(key, "rnd_go_voice")) { if (x) randomise(in, true); }
        else if (!strcmp(key, "rnd_all") || !strcmp(key, "rnd_none")) { if (x) for (int sl = 0; sl < kSlots; ++sl) in->rnd_sel[sl] = key[4] == 'a'; }
        else if (!strcmp(key, "rnd_amount")) in->rnd_amount = x < 0 ? 0 : (x > 127 ? 127 : x);
        else if (!strcmp(key, "rnd_voice")) in->rnd_voice = x != 0;
        else if (key[4] == 's' && key[5] >= '0' && key[5] <= '9' && key[6] >= '0' && key[6] <= '9') {
            const int sl = (key[5] - '0') * 10 + (key[6] - '0') - 1;
            if (sl >= 0 && sl < kSlots) in->rnd_sel[sl] = x != 0;
        }
        return;
    }
    if (!strncmp(key, "int_", 4)) {
        const int on = x != 0;
        if (!strcmp(key, "int_rev")) in->int_rev = on;
        else if (!strcmp(key, "int_dly")) in->int_dly = on;
        else if (!strcmp(key, "int_comp") && on != in->int_comp) {
            in->int_comp = on;
            void *e = in->eng[kFxBackend];
            if (!on) { int v = 0; kBackends[kFxBackend].get(e, "comp", &v); in->comp_saved = v; kBackends[kFxBackend].set(e, "comp", 0); }
            else kBackends[kFxBackend].set(e, "comp", in->comp_saved);
        }
        return;
    }
    char ek[24];
    if (editor_key(in, key, ek, sizeof ek)) { set_param(p, ek, val); return; }
    if (parse_slot(key, &slot, &rest)) {
        if (!strcmp(rest, "vg")) return;   /* derived; the skin only reads it */
        if (!strcmp(rest, "fam")) return;   /* derived from the voice; the skin only reads it */
        if (!strcmp(rest, "src")) {
            /* the menu's last entry, OFF, switches the slot off and keeps its voice; a voice switches it on */
            if (x == kTotalVoices) { in->on[slot] = 0; return; }
            if (x >= 0 && x < kTotalVoices) { in->slot_flat[slot] = x; in->on[slot] = 1; int b, v; flat_to_bv(x, &b, &v); engine_for(in, b); }
            return;
        }
        if (int *o = owned(in, slot, rest)) { *o = x < 0 ? 0 : (x > 127 ? 127 : x); return; }
        for (int i = 0; i < kNumKnobs; ++i)
            if (!strcmp(rest, kKnobs[i].name)) { int v = x; forward(in, slot, kKnobs[i], &v, true); return; }
    } else if (!strncmp(key, "fx_", 3)) {
        if (!in->int_comp && !strcmp(key + 3, "comp")) { in->comp_saved = x; return; }   /* held until the compressor is switched back on */
        kBackends[kFxBackend].set(in->eng[kFxBackend], key + 3, x);
    } else if (!strcmp(key, "lfo_bpm")) {   /* host tempo, for the synced delay */
        sc808_set_param((sc808_engine_t *)in->eng[kFxBackend], "dly_bpm", val);
    }
}

const char *const kFxKeys[] = { "rev_decay", "rev_tone", "rev_hpf", "rev_level", "dly_time", "dly_fdbk", "dly_tone",
                                "dly_hpf", "dly_level", "master_dist", "master_drive", "comp", "volume" };
/* the voice (saved as the voice itself, even while the slot is off: get_state), its pots, then "on" last: picking a voice
 * switches a slot on, so a slot saved off is switched off after */
const char *const kSlotKeys[] = { "src", "level", "tune", "decay", "drive", "dist", "attack", "tone", "snappy", "noise", "rate", "sweep", "pmod", "ndecay", "sat", "pan", "rev", "dly", "on" };

int get_param(void *p, const char *key, char *buf, int n);

/* The project chunk (the wrapper stores whatever "state" returns): every slot and FX value as key=value; pairs,
 * slot keys first within a slot so that src is applied before the voice's own pots. */
int get_state(Inst *in, char *buf, int n) {
    int len = snprintf(buf, n, "trmpc1;edit_slot=%d;rnd_amount=%d;rnd_voice=%d;", in->edit_slot, in->rnd_amount, in->rnd_voice);
    for (int sl = 0; sl < kSlots; ++sl) len += snprintf(buf + len, n - len, "rnd_s%02d=%d;", sl + 1, in->rnd_sel[sl]);
    char k[24], v[16];
    for (int s = 1; s <= kSlots; ++s)
        for (const char *sk : kSlotKeys) {
            snprintf(k, sizeof k, "s%02d_%s", s, sk);
            if (!strcmp(sk, "src")) snprintf(v, sizeof v, "%d", in->slot_flat[s - 1]);   /* the voice, not OFF */
            else if (get_param(in, k, v, sizeof v) < 0) continue;
            len += snprintf(buf + len, n - len, "%s=%s;", k, v);
            if (len >= n - 32) return len;
        }
    for (const char *fk : kFxKeys) {
        snprintf(k, sizeof k, "fx_%s", fk);
        if (get_param(in, k, v, sizeof v) < 0) continue;
        len += snprintf(buf + len, n - len, "%s=%s;", k, v);
        if (len >= n - 32) break;
    }
    /* after the fx values: switching the compressor off saves the Comp value set above */
    len += snprintf(buf + len, n - len, "int_rev=%d;int_dly=%d;int_comp=%d;latency=%d;preset=%d;", in->int_rev, in->int_dly, in->int_comp, in->latency, in->preset);
    return len;
}

void set_param(void *p, const char *key, const char *val);

void set_state(Inst *in, const char *st) {
    if (strncmp(st, "trmpc1;", 7)) return;
    st += 7;
    char k[24], v[16];
    in->loading = 1;
    while (*st) {
        const char *eq = strchr(st, '=');
        const char *end = strchr(st, ';');
        if (!eq || !end || eq > end) break;
        snprintf(k, sizeof k, "%.*s", (int)(eq - st) < 23 ? (int)(eq - st) : 23, st);
        snprintf(v, sizeof v, "%.*s", (int)(end - eq - 1) < 15 ? (int)(end - eq - 1) : 15, eq + 1);
        set_param(in, k, v);
        st = end + 1;
    }
    in->loading = 0;
    in->load_ms = now_ms();
}

int get_param(void *p, const char *key, char *buf, int n) {
    Inst *in = (Inst *)p;
    int slot; const char *rest;
    if (!strcmp(key, "state")) return get_state(in, buf, n);
    if (!strcmp(key, "edit_slot")) return snprintf(buf, n, "%d", in->edit_slot);
    if (!strcmp(key, "latency")) return snprintf(buf, n, "%d", in->latency);
    if (!strcmp(key, "preset")) return snprintf(buf, n, "%d", in->preset);
    if (!strncmp(key, "preset:", 7)) {   /* a program's name, for MPC's PRESET menu */
        const int i = atoi(key + 7);
        if (i < 0 || i >= kProgs) return -1;
        if (i < kFactory) { factory_name(i, buf, n); return (int)strlen(buf); }
        const int u = i - kFactory;
        return in->pr_state[u] ? snprintf(buf, n, "U%02d %s", u + 1, in->pr_name[u]) : snprintf(buf, n, "U%02d -", u + 1);
    }
    if (!strcmp(key, "pr_name")) {
        const int i = in->preset;
        if (i < 0 || i >= kProgs) return snprintf(buf, n, "-");
        if (i < kFactory) { factory_name(i, buf, n); return (int)strlen(buf); }
        const int u = i - kFactory;
        return in->pr_state[u] ? snprintf(buf, n, "U%02d %s", u + 1, in->pr_name[u]) : snprintf(buf, n, "U%02d -", u + 1);
    }
    if (!strcmp(key, "kb_name")) return snprintf(buf, n, "%s_", in->kb_buf);
    if (!strcmp(key, "kb_open")) return snprintf(buf, n, "%d", in->kb_open);
    if (!strncmp(key, "kb_", 3) || !strcmp(key, "pr_save") || !strcmp(key, "pr_del")) return snprintf(buf, n, "0");
    if (!strncmp(key, "rnd_", 4)) {
        if (!strcmp(key, "rnd_go") || !strcmp(key, "rnd_go_voice") || !strcmp(key, "rnd_all") || !strcmp(key, "rnd_none")) return snprintf(buf, n, "0");
        if (!strcmp(key, "rnd_amount")) return snprintf(buf, n, "%d", in->rnd_amount);
        if (!strcmp(key, "rnd_voice")) return snprintf(buf, n, "%d", in->rnd_voice);
        if (key[4] == 's' && key[5] >= '0' && key[5] <= '9' && key[6] >= '0' && key[6] <= '9') {
            const int sl = (key[5] - '0') * 10 + (key[6] - '0') - 1;
            if (sl >= 0 && sl < kSlots) return snprintf(buf, n, "%d", in->rnd_sel[sl]);
        }
        return -1;
    }
    if (!strcmp(key, "int_rev")) return snprintf(buf, n, "%d", in->int_rev);
    if (!strcmp(key, "int_dly")) return snprintf(buf, n, "%d", in->int_dly);
    if (!strcmp(key, "int_comp")) return snprintf(buf, n, "%d", in->int_comp);
    char ek[24];
    if (editor_key(in, key, ek, sizeof ek)) return get_param(p, ek, buf, n);
    if (parse_slot(key, &slot, &rest)) {
        if (!strcmp(rest, "src")) return snprintf(buf, n, "%d", in->on[slot] ? in->slot_flat[slot] : kTotalVoices);   /* OFF while off */
        if (!strcmp(rest, "vg")) return snprintf(buf, n, "%d", in->slot_flat[slot] + (TRKIT_LEMUS ? kTotalVoices + 1 : 0));   /* read-only: the skin's per-voice, per-build controls */
        if (!strcmp(rest, "fam")) return snprintf(buf, n, "%d", slot_family(in, slot));   /* read-only: picks the faceplate */
        if (int *o = owned(in, slot, rest)) return snprintf(buf, n, "%d", *o);
        for (int i = 0; i < kNumKnobs; ++i)
            if (!strcmp(rest, kKnobs[i].name)) { int v = 0; if (!forward(in, slot, kKnobs[i], &v, false)) v = 0; return snprintf(buf, n, "%d", v); }
    } else if (!strncmp(key, "fx_", 3)) {
        if (!in->int_comp && !strcmp(key + 3, "comp")) return snprintf(buf, n, "%d", in->comp_saved);
        int v = 0;
        if (kBackends[kFxBackend].get(in->eng[kFxBackend], key + 3, &v)) return snprintf(buf, n, "%d", v);
    }
    return -1;
}

inline float peak_of(const float *x, int n, float pk) {
    for (int i = 0; i < n; ++i) { const float a = x[i] < 0 ? -x[i] : x[i]; if (a > pk) pk = a; }
    return pk;
}

/* denormals are slow on the Force's ARM core and the reverb and delay tails decay into them */
inline void flush_denormals() {
#if defined(__arm__)
    unsigned fpscr;
    asm volatile("vmrs %0, fpscr" : "=r"(fpscr));
    fpscr |= 1u << 24;   /* FZ */
    asm volatile("vmsr fpscr, %0" : : "r"(fpscr));
#elif defined(__aarch64__)
    unsigned long fpcr;
    asm volatile("mrs %0, fpcr" : "=r"(fpcr));
    fpcr |= 1ul << 24;   /* FZ */
    asm volatile("msr fpcr, %0" : : "r"(fpcr));
#elif defined(__SSE__)
    _mm_setcsr(_mm_getcsr() | 0x8040);
#endif
}

/* Output limiter: sixteen pads add up (a kick, a snare and a clap together exceed full scale), and a hard clip is what
 * crackles. Below kKnee the signal is untouched; above it the peak is rounded off toward 1.0 (the slope is 1 at the
 * knee and it never exceeds full scale). */
constexpr float kKnee = 0.5f;
inline float soft_limit(float x) {
    const float a = x < 0 ? -x : x;
    if (a <= kKnee) return x;
    const float u = (a - kKnee) * (1.0f / (1.0f - kKnee));
    const float y = kKnee + (1.0f - kKnee) * (u / (1.0f + u));
    return x < 0 ? -y : y;
}

void render(void *p, int16_t *out, int frames) {
    Inst *in = (Inst *)p;
    flush_denormals();
    static thread_local float scratch[kBlock], dl[kBlock], dr[kBlock], sr[kBlock], sd[kBlock], ol[kBlock], orr[kBlock];
    for (int done = 0; done < frames; done += kBlock) {
        const int n = frames - done < kBlock ? frames - done : kBlock;
        /* the latency to the circuit engines; held 606 / CR-78 hits whose wait is over (queued with the latency before
         * call k, the boards' hits sound from call k + latency: count it down, fire at the next) */
        const int lat = effective_latency(in);
        {   /* the kits the slots that are on need: their engines (and Gen2 boards) are made off this thread, once */
            unsigned w = 0;
            for (int s = 0; s < kSlots; ++s) { int b, v; if (slot_voice(in, s, &b, &v)) w |= 1u << b; }
            if (w & ~in->maker->want.load(std::memory_order_relaxed)) in->maker->want.fetch_or(w, std::memory_order_release);
        }
        for (int b = 0; b < kNumBackends; ++b)
            if (renders_ahead(b) && eng_get(in, b) && in->lat_sent[b] != lat) { kBackends[b].set(in->eng[b], "latency", lat); in->lat_sent[b] = lat; }
        for (int h = 0; h < in->nheld; ) {
            Inst::Held &hd = in->held[h];
            if (hd.blocks > 0) { hd.blocks--; ++h; continue; }
            if (eng_get(in, hd.b)) { kBackends[hd.b].trigger(in->eng[hd.b], hd.v, hd.vel); in->quiet[hd.b] = 0; }
            in->held[h] = in->held[--in->nheld];
        }
        memset(dl, 0, sizeof(float) * n); memset(dr, 0, sizeof(float) * n);
        memset(sr, 0, sizeof(float) * n); memset(sd, 0, sizeof(float) * n);
        const bool pub = in->tap_owner && n == kBlock;
        const uint32_t wr = g_tap.written.load(std::memory_order_relaxed);
        int16_t (*planes)[trtap::kFrames] = g_tap.data[wr % trtap::kRing];
        /* the planes are only filled while some tap reads something (the latency scheme runs regardless) */
        bool tapping = false;
        if (pub) {
            tapping = g_tap.tapsRev.load(std::memory_order_relaxed) > 0 || g_tap.tapsDel.load(std::memory_order_relaxed) > 0;
            for (int s = 0; s < kSlots && !tapping; ++s) tapping = g_tap.tapped[s].load(std::memory_order_relaxed) > 0;
            if (tapping) memset(planes, 0, sizeof(g_tap.data[0]));
        }
        auto q = [](float x) { x *= 32767.0f; return (int16_t)(x > 32767.0f ? 32767.0f : (x < -32768.0f ? -32768.0f : x)); };
        bool awake = false;
        for (int b = 0; b < kNumBackends; ++b) {
            if (!eng_get(in, b) || !in->tap[b] || in->quiet[b] >= kSleepAfter) continue;
            awake = true;
            memset(in->tap[b], 0, sizeof(float) * kBlock * kBackends[b].nvoices);
            kBackends[b].render(in->eng[b], scratch, n);
            float pk = 0.0f;
            for (int s = 0; s < kSlots; ++s) {
                int sb, v;
                if (!slot_voice(in, s, &sb, &v)) continue;   /* OFF */
                if (sb != b || in->voice_owner[b][v] != s) continue;   /* two slots on one voice: the one that was hit */
                const float *src = in->tap[b] + v * kBlock;
                pk = peak_of(src, n, pk);
                /* constant-power pan, unity at the centre */
                const float ang = (float)in->pan[s] * (1.5707963f / 127.0f);
                const float gl = cosf(ang) * 1.41421356f, gr = sinf(ang) * 1.41421356f;
                const float rv = (float)in->rev[s] * (1.0f / 127.0f), dy = (float)in->dly[s] * (1.0f / 127.0f);
                const bool tapped = tapping && g_tap.tapped[s].load(std::memory_order_relaxed) > 0;   /* its dry goes out through a tap, not the main mix */
                for (int i = 0; i < n; ++i) {
                    const float x = src[i];
                    if (tapping) { planes[2 * s][i] = q(x * gl); planes[2 * s + 1][i] = q(x * gr); }
                    if (!tapped) { dl[i] += x * gl; dr[i] += x * gr; }
                    sr[i] += x * rv; sd[i] += x * dy;
                }
            }
            in->quiet[b] = pk > kSilence ? 0 : in->quiet[b] + n;
        }
        if (tapping) {
            for (int i = 0; i < n; ++i) { planes[trtap::kPlaneRev][i] = q(sr[i]); planes[trtap::kPlaneDel][i] = q(sd[i]); }
            /* a send that leaves through a tap no longer feeds the kit's own reverb / delay */
            if (g_tap.tapsRev.load(std::memory_order_relaxed) > 0) memset(sr, 0, sizeof(float) * n);
            if (g_tap.tapsDel.load(std::memory_order_relaxed) > 0) memset(sd, 0, sizeof(float) * n);
        }
        if (!in->int_rev) memset(sr, 0, sizeof(float) * n);
        if (!in->int_dly) memset(sd, 0, sizeof(float) * n);
        if (awake || in->fx_quiet < kSleepAfter) {
            sc808_fx_stereo((sc808_engine_t *)in->eng[kFxBackend], dl, dr, sr, sd, ol, orr, n);
            const float pk = peak_of(orr, n, peak_of(ol, n, 0.0f));
            in->fx_quiet = (awake || pk > kSilence) ? 0 : in->fx_quiet + n;
        } else {
            memset(ol, 0, sizeof(float) * n); memset(orr, 0, sizeof(float) * n);
        }
        int16_t *dst = out + done * 2;
        int16_t cur[kBlock * 2];
        for (int i = 0; i < n; ++i) {
            cur[2 * i] = q(soft_limit(ol[i]));
            cur[2 * i + 1] = q(soft_limit(orr[i]));
        }
        if (pub) {
            g_tap.written.store(wr + 1, std::memory_order_release);
            /* one block of latency: output the block published last time, keep this one */
            if (in->have_pending) memcpy(dst, in->pending, sizeof(int16_t) * n * 2); else memset(dst, 0, sizeof(int16_t) * n * 2);
            memcpy(in->pending, cur, sizeof(int16_t) * n * 2);
            in->have_pending = true;
            g_tap.hostRead.store(wr, std::memory_order_release);   /* blocks the host has taken (the one just output is wr - 1) */
        } else {
            memcpy(dst, cur, sizeof(int16_t) * n * 2);
        }
    }
    if (in->tap_owner)
        g_tap.hostCallUs.store(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(), std::memory_order_release);
}

const mpc_engine_t kEngine = { create, destroy, midi, set_param, get_param, render, nullptr };

}  // namespace

extern "C" const mpc_engine_t *mpc_engine(void) { return &kEngine; }
extern "C" __attribute__((visibility("default"))) trtap::Shared *trmpc_tap_shared(void) { return &g_tap; }
