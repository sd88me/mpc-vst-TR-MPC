/* Schwung plugin_api_v2 DSP -> mpc_engine_t, with the MPC drum-pad note remap.
 *
 * Same glue as mpc-vst-plugins' adapters/schwung/schwung_engine.c (44.1 kHz, 128-frame int16 blocks, MIDI as source 2),
 * plus: with the MPC OS drum-pad patch (mpc-vst-machinedrum release/mpc_patch) a plugin's drum program sends pad n as
 * MIDI note n-1 (notes 0-15). The kits' own drum-rack maps start at note 36 (pad n = note 35+n in voice order), so notes
 * 0-15 are moved up by 36. Notes 16 and above pass through unchanged, so the melodic layout still works.
 *
 * One copy of this file lives in ports/common/; each port's src/ holds an identical copy (a port builds in a container
 * that only sees its own folder). tools/sync_common.py refreshes them.
 */
#include <stddef.h>
#include "engine.h"

typedef struct {
    uint32_t api_version;
    void *(*create_instance)(const char *module_dir, const char *json_defaults);
    void (*destroy_instance)(void *instance);
    void (*on_midi)(void *instance, const uint8_t *msg, int len, int source);
    void (*set_param)(void *instance, const char *key, const char *val);
    int (*get_param)(void *instance, const char *key, char *buf, int buf_len);
    int (*get_error)(void *instance, char *buf, int buf_len);
    void (*render_block)(void *instance, int16_t *out_lr, int frames);
} plugin_api_v2_t;
extern plugin_api_v2_t *move_plugin_init_v2(const void *host);

#define MIDI_SOURCE_EXTERNAL 2
#define DRUM_PAD_BASE_NOTE 36
#define DRUM_PAD_COUNT 16

static plugin_api_v2_t *api;

static void *create(const char *dir) { return api->create_instance(dir ? dir : "", NULL); }
static void destroy(void *i) { api->destroy_instance(i); }
static void midi(void *i, const uint8_t *m, int n) {
    const uint8_t type = m[0] & 0xF0;
    if (n >= 2 && (type == 0x80 || type == 0x90 || type == 0xA0) && m[1] < DRUM_PAD_COUNT) {
        uint8_t t[4] = { m[0], (uint8_t)(m[1] + DRUM_PAD_BASE_NOTE), n > 2 ? m[2] : 0, n > 3 ? m[3] : 0 };
        api->on_midi(i, t, n > 4 ? 4 : n, MIDI_SOURCE_EXTERNAL);
        return;
    }
    api->on_midi(i, m, n, MIDI_SOURCE_EXTERNAL);
}
static void set_param(void *i, const char *k, const char *v) { api->set_param(i, k, v); }
static int get_param(void *i, const char *k, char *b, int n) { return api->get_param(i, k, b, n); }
static void render(void *i, int16_t *out, int frames) { api->render_block(i, out, frames); }

static const mpc_engine_t engine = { create, destroy, midi, set_param, get_param, render, NULL };

const mpc_engine_t *mpc_engine(void) {
    if (!api) api = move_plugin_init_v2(NULL);
    return api ? &engine : NULL;
}
