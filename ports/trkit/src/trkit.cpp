/*
 * TR-MPC: sixteen slots, each playing any voice from any of the four ported kits (6W6, 8W8, CW-78, 9W9).
 *
 * Prototype, engine-level mixing. Each kit's engine renders ONE mono bus with its own reverb, delay,
 * compressor and master stage, and exposes no per-voice output. So a slot does not own a voice; it
 * routes a pad to (kit, voice) and forwards its knobs to that voice's own keys. Consequences:
 *   - up to four engines render per block (created on first use), summed to the output;
 *   - two slots that pick the SAME voice of the same kit share one voice (they retrigger it);
 *   - level/tune/decay/drive/rev/dly are the voice's own pots, so they are not stored per slot: get/set
 *     proxy to the engine and the projects's chunk holds them as the VST params.
 * Per-slot pan and true per-slot isolation need a per-voice render added to the engines.
 *
 * Pads: MIDI notes 36..51 -> slots 1..16 (MPC drum-pad default, C1 upward).
 * GPL-3.0, as the kits it links.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
    int (*set)(void *, const char *key, int pot);
    int (*get)(void *, const char *key, int *pot);
};

/* ---- 6W6 / 8W8 / CW-78 share a C API shape; 9W9 has its own ---- */
#define STRING_API(P, T)                                                                            \
    void *P##_c(const char *) { return P##_create(kSr); }                                           \
    void P##_d(void *e) { P##_destroy((T *)e); }                                                    \
    void P##_t(void *e, int v, int vel) { P##_trigger((T *)e, v, vel); }                            \
    void P##_r(void *e, float *o, int n) { P##_render((T *)e, o, n); }                              \
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
void er99_t(void *e, int v, int vel) { er99_engine_trigger((er99_engine_t *)e, (er99_trigger_t)v, vel); }
void er99_r(void *e, float *o, int n) { er99_engine_render((er99_engine_t *)e, o, n); }
int er99_s(void *e, const char *k, int p) { return er99_engine_set_param((er99_engine_t *)e, k, (float)p); }
int er99_g(void *e, const char *k, int *p) { float f; if (!er99_engine_get_param((er99_engine_t *)e, k, &f)) return 0; *p = (int)lrintf(f); return 1; }

const Backend kBackends[4] = {
    { "606",  SD606_NUM_VOICES, sd606_voice_id, sd606_c, sd606_d, sd606_t, sd606_r, sd606_s, sd606_g },
    { "808",  SC808_NUM_VOICES, sc808_voice_id, sc808_c, sc808_d, sc808_t, sc808_r, sc808_s, sc808_g },
    { "CR78", CR78_NUM_VOICES,  cr78_voice_id,  cr78_c,  cr78_d,  cr78_t,  cr78_r,  cr78_s,  cr78_g },
    { "909",  ER99_NUM_TRIGGERS, er99_vid,      er99_c,  er99_d,  er99_t,  er99_r,  er99_s,  er99_g },
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
    { "rev",   { "rev", nullptr, nullptr } },
    { "dly",   { "dly", nullptr, nullptr } },
};
constexpr int kNumKnobs = 6;

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

struct Inst {
    char dir[512];
    void *eng[kNumBackends];
    int slot_flat[kSlots];
    int vol[kNumBackends];     /* kit volume pot, forwarded to the engine's "volume" key */
};

void *engine_for(Inst *in, int b) {
    if (!in->eng[b]) in->eng[b] = kBackends[b].create(in->dir);
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
    for (int b = 0; b < kNumBackends; ++b) in->vol[b] = 100;
    for (int s = 0; s < kSlots; ++s) { int b, v; flat_to_bv(in->slot_flat[s], &b, &v); engine_for(in, b); }
    return in;
}

void destroy(void *p) {
    Inst *in = (Inst *)p;
    if (!in) return;
    for (int b = 0; b < kNumBackends; ++b) if (in->eng[b]) kBackends[b].destroy(in->eng[b]);
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
    if (void *e = engine_for(in, b)) kBackends[b].trigger(e, v, m[2]);
}

/* keys: sNN_src (flat voice index), sNN_level/tune/decay/drive/rev/dly, kN_volume */
bool parse_slot(const char *key, int *slot, const char **rest) {
    if (key[0] != 's' || key[1] < '0' || key[1] > '9' || key[2] < '0' || key[2] > '9' || key[3] != '_') return false;
    *slot = (key[1] - '0') * 10 + (key[2] - '0') - 1;
    *rest = key + 4;
    return *slot >= 0 && *slot < kSlots;
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
        for (int i = 0; i < kNumKnobs; ++i)
            if (!strcmp(rest, kKnobs[i].name)) { int v = x; forward(in, slot, kKnobs[i], &v, true); return; }
    } else if (key[0] == 'k' && key[1] >= '0' && key[1] < '0' + kNumBackends && !strcmp(key + 2, "_volume")) {
        const int b = key[1] - '0';
        in->vol[b] = x;
        if (in->eng[b]) kBackends[b].set(in->eng[b], "volume", x);
    }
}

int get_param(void *p, const char *key, char *buf, int n) {
    Inst *in = (Inst *)p;
    int slot; const char *rest;
    if (parse_slot(key, &slot, &rest)) {
        if (!strcmp(rest, "src")) return snprintf(buf, n, "%d", in->slot_flat[slot]);
        for (int i = 0; i < kNumKnobs; ++i)
            if (!strcmp(rest, kKnobs[i].name)) { int v = 0; if (!forward(in, slot, kKnobs[i], &v, false)) v = 0; return snprintf(buf, n, "%d", v); }
    } else if (key[0] == 'k' && key[1] >= '0' && key[1] < '0' + kNumBackends && !strcmp(key + 2, "_volume")) {
        return snprintf(buf, n, "%d", in->vol[key[1] - '0']);
    }
    return -1;
}

void render(void *p, int16_t *out, int frames) {
    Inst *in = (Inst *)p;
    static thread_local float mix[kBlock], tmp[kBlock];
    for (int done = 0; done < frames; done += kBlock) {
        const int n = frames - done < kBlock ? frames - done : kBlock;
        memset(mix, 0, sizeof(float) * n);
        for (int b = 0; b < kNumBackends; ++b) {
            if (!in->eng[b]) continue;
            memset(tmp, 0, sizeof(float) * n);
            kBackends[b].render(in->eng[b], tmp, n);
            for (int i = 0; i < n; ++i) mix[i] += tmp[i];
        }
        for (int i = 0; i < n; ++i) {
            float s = mix[i] * 32767.0f;
            s = s > 32767.0f ? 32767.0f : (s < -32768.0f ? -32768.0f : s);
            out[(done + i) * 2] = out[(done + i) * 2 + 1] = (int16_t)s;
        }
    }
}

const mpc_engine_t kEngine = { create, destroy, midi, set_param, get_param, render, nullptr };

}  // namespace

extern "C" const mpc_engine_t *mpc_engine(void) { return &kEngine; }
