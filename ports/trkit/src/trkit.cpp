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
 * GPL-3.0, as the kits it links.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

extern "C" {
#include "engine.h"   /* vendored: mpc-vst-plugins/wrapper/engine.h */
}

#include "sd606_engine.h"
#include "sc808_engine.h"
#include "cr78_engine.h"
extern "C" {
#include "er99_engine.h"
}

namespace {

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
};
constexpr int kNumKnobs = 4;

/* Default flat voice per slot (808 kick/snare/toms, 606 hats, 909 clap/rim/cymbals, CR78 percussion) */
int default_flat(int slot) {
    static const struct { int b, v; } d[kSlots] = {
        {1, 0}, {1, 1}, {1, 2}, {1, 4}, {0, 4}, {0, 5}, {3, 6}, {3, 5},
        {1, 11}, {1, 12}, {2, 10}, {2, 6}, {0, 6}, {3, 9}, {2, 9}, {1, 7},
    };
    int flat = d[slot].v;
    for (int i = 0; i < d[slot].b; ++i) flat += kBackends[i].nvoices;
    return flat;
}

constexpr int kFxBackend = 1;                 /* 8W8's reverb, delay and master stage serve the whole kit */
constexpr int kSleepAfter = 5 * 44100;        /* samples of silence before an engine or the FX stage stops rendering */
constexpr float kSilence = 1e-5f;

struct Inst {
    char dir[512];
    void *eng[kNumBackends];
    float *tap[kNumBackends];  /* per engine: nvoices x kBlock dry samples */
    int quiet[kNumBackends];   /* samples of near-silence since the engine last sounded */
    int fx_quiet;              /* same for the shared FX stage's output */
    int slot_flat[kSlots];
    int pan[kSlots], rev[kSlots], dly[kSlots];   /* 0..127; pan 64 = centre */
};

void *engine_for(Inst *in, int b) {
    if (!in->eng[b]) {
        in->eng[b] = kBackends[b].create(in->dir);
        in->tap[b] = (float *)calloc((size_t)kBackends[b].nvoices * kBlock, sizeof(float));
        if (in->eng[b] && in->tap[b]) kBackends[b].set_tap(in->eng[b], in->tap[b]);
    }
    return in->eng[b];
}

/* try "<id>_<suffix>", then "<id>_c_<suffix>" (9W9 kick/snare/toms) */
bool forward(Inst *in, int slot, const Knob &k, int *val, bool set) {
    int b, v;
    flat_to_bv(in->slot_flat[slot], &b, &v);
    void *e = engine_for(in, b);
    if (!e) return false;
    const char *id = kBackends[b].voice_id(v);
    char key[48];
    for (int s = 0; s < 3 && k.suffixes[s]; ++s) {
        const char *infix[2] = { "", "c_" };
        for (int c = 0; c < 2; ++c) {
            snprintf(key, sizeof key, "%s_%s%s", id, infix[c], k.suffixes[s]);
            if (set ? kBackends[b].set(e, key, *val) : kBackends[b].get(e, key, val)) return true;
        }
    }
    return false;
}

void *create(const char *dir) {
    Inst *in = (Inst *)calloc(1, sizeof(Inst));
    if (!in) return nullptr;
    snprintf(in->dir, sizeof in->dir, "%s", dir ? dir : "");
    for (int s = 0; s < kSlots; ++s) in->slot_flat[s] = default_flat(s);
    for (int s = 0; s < kSlots; ++s) { in->pan[s] = 64; in->rev[s] = 0; in->dly[s] = 0; }
    engine_for(in, kFxBackend);
    for (int s = 0; s < kSlots; ++s) { int b, v; flat_to_bv(in->slot_flat[s], &b, &v); engine_for(in, b); }
    return in;
}

void destroy(void *p) {
    Inst *in = (Inst *)p;
    if (!in) return;
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
    flat_to_bv(in->slot_flat[slot], &b, &v);
    if (void *e = engine_for(in, b)) { kBackends[b].trigger(e, v, m[2]); in->quiet[b] = 0; }
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
    if (!strcmp(rest, "pan")) return &in->pan[slot];
    if (!strcmp(rest, "rev")) return &in->rev[slot];
    if (!strcmp(rest, "dly")) return &in->dly[slot];
    return nullptr;
}

void set_param(void *p, const char *key, const char *val) {
    Inst *in = (Inst *)p;
    int slot; const char *rest;
    const int x = atoi(val);
    if (parse_slot(key, &slot, &rest)) {
        if (!strcmp(rest, "src")) {
            if (x >= 0 && x < kTotalVoices) { in->slot_flat[slot] = x; int b, v; flat_to_bv(x, &b, &v); engine_for(in, b); }
            return;
        }
        if (int *o = owned(in, slot, rest)) { *o = x < 0 ? 0 : (x > 127 ? 127 : x); return; }
        for (int i = 0; i < kNumKnobs; ++i)
            if (!strcmp(rest, kKnobs[i].name)) { int v = x; forward(in, slot, kKnobs[i], &v, true); return; }
    } else if (!strncmp(key, "fx_", 3)) {
        kBackends[kFxBackend].set(in->eng[kFxBackend], key + 3, x);
    } else if (!strcmp(key, "lfo_bpm")) {   /* host tempo, for the synced delay */
        sc808_set_param((sc808_engine_t *)in->eng[kFxBackend], "dly_bpm", val);
    }
}

int get_param(void *p, const char *key, char *buf, int n) {
    Inst *in = (Inst *)p;
    int slot; const char *rest;
    if (parse_slot(key, &slot, &rest)) {
        if (!strcmp(rest, "src")) return snprintf(buf, n, "%d", in->slot_flat[slot]);
        if (int *o = owned(in, slot, rest)) return snprintf(buf, n, "%d", *o);
        for (int i = 0; i < kNumKnobs; ++i)
            if (!strcmp(rest, kKnobs[i].name)) { int v = 0; if (!forward(in, slot, kKnobs[i], &v, false)) v = 0; return snprintf(buf, n, "%d", v); }
    } else if (!strncmp(key, "fx_", 3)) {
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

void render(void *p, int16_t *out, int frames) {
    Inst *in = (Inst *)p;
    flush_denormals();
    static thread_local float scratch[kBlock], dl[kBlock], dr[kBlock], sr[kBlock], sd[kBlock], ol[kBlock], orr[kBlock];
    for (int done = 0; done < frames; done += kBlock) {
        const int n = frames - done < kBlock ? frames - done : kBlock;
        memset(dl, 0, sizeof(float) * n); memset(dr, 0, sizeof(float) * n);
        memset(sr, 0, sizeof(float) * n); memset(sd, 0, sizeof(float) * n);
        bool awake = false;
        for (int b = 0; b < kNumBackends; ++b) {
            if (!in->eng[b] || !in->tap[b] || in->quiet[b] >= kSleepAfter) continue;
            awake = true;
            memset(in->tap[b], 0, sizeof(float) * kBlock * kBackends[b].nvoices);
            kBackends[b].render(in->eng[b], scratch, n);
            float pk = 0.0f;
            for (int s = 0; s < kSlots; ++s) {
                int sb, v;
                flat_to_bv(in->slot_flat[s], &sb, &v);
                if (sb != b) continue;
                const float *src = in->tap[b] + v * kBlock;
                pk = peak_of(src, n, pk);
                /* constant-power pan, unity at the centre */
                const float ang = (float)in->pan[s] * (1.5707963f / 127.0f);
                const float gl = cosf(ang) * 1.41421356f, gr = sinf(ang) * 1.41421356f;
                const float rv = (float)in->rev[s] * (1.0f / 127.0f), dy = (float)in->dly[s] * (1.0f / 127.0f);
                for (int i = 0; i < n; ++i) {
                    const float x = src[i];
                    dl[i] += x * gl; dr[i] += x * gr;
                    sr[i] += x * rv; sd[i] += x * dy;
                }
            }
            in->quiet[b] = pk > kSilence ? 0 : in->quiet[b] + n;
        }
        if (awake || in->fx_quiet < kSleepAfter) {
            sc808_fx_stereo((sc808_engine_t *)in->eng[kFxBackend], dl, dr, sr, sd, ol, orr, n);
            const float pk = peak_of(orr, n, peak_of(ol, n, 0.0f));
            in->fx_quiet = (awake || pk > kSilence) ? 0 : in->fx_quiet + n;
        } else {
            memset(ol, 0, sizeof(float) * n); memset(orr, 0, sizeof(float) * n);
        }
        for (int i = 0; i < n; ++i) {
            float l = ol[i] * 32767.0f, r = orr[i] * 32767.0f;
            l = l > 32767.0f ? 32767.0f : (l < -32768.0f ? -32768.0f : l);
            r = r > 32767.0f ? 32767.0f : (r < -32768.0f ? -32768.0f : r);
            out[(done + i) * 2] = (int16_t)l;
            out[(done + i) * 2 + 1] = (int16_t)r;
        }
    }
}

const mpc_engine_t kEngine = { create, destroy, midi, set_param, get_param, render, nullptr };

}  // namespace

extern "C" const mpc_engine_t *mpc_engine(void) { return &kEngine; }
