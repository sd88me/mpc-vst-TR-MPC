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
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "host/plugin_api_v1.h"

extern plugin_api_v2_t *move_plugin_init_v2(const host_api_v1_t *host);

#define MIDI_SOURCE_EXTERNAL 2
#define DRUM_PAD_BASE_NOTE 36
#define DRUM_PAD_COUNT 16

static plugin_api_v2_t *api;

/* The kits read the host transport through Schwung's host_api_v1 (get_clock_status for "is the transport running",
 * get_bpm for the synced delay and the CW-78 preset rhythms). MPC has one transport for every plugin instance and these
 * callbacks take no instance, so the state is process-wide. vst2_wrap.c (vst.json "defines": HAS_TRANSPORT, HAS_LFO_BPM)
 * delivers it as the params "transport" ("1"/"0"; "1" again = the song position jumped back) and "lfo_bpm". */
static volatile int g_playing;
static volatile int g_restart;          /* transport restarted while playing: report "stopped" for one block so the DSP resets its clock */
static volatile float g_bpm = 120.0f;
static host_api_v1_t g_host;

static int host_clock(void) { return (g_playing && !g_restart) ? MOVE_CLOCK_STATUS_RUNNING : MOVE_CLOCK_STATUS_STOPPED; }
static float host_bpm(void) { return g_bpm; }
static double host_beat(void) { return -1.0; }   /* the DSPs fall back to counting samples from the transport start */

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
static void set_param(void *i, const char *k, const char *v) {
    if (!strcmp(k, "transport")) {
        int on = v[0] == '1';
        if (on && g_playing) g_restart = 1;
        g_playing = on;
        return;
    }
    if (!strcmp(k, "lfo_bpm")) { float b = (float)atof(v); if (b > 20.0f) g_bpm = b; return; }
    api->set_param(i, k, v);
}
static int get_param(void *i, const char *k, char *b, int n) { return api->get_param(i, k, b, n); }
static void render(void *i, int16_t *out, int frames) { api->render_block(i, out, frames); g_restart = 0; }

static const mpc_engine_t engine = { create, destroy, midi, set_param, get_param, render, NULL };

const mpc_engine_t *mpc_engine(void) {
    if (!api) {
        g_host.api_version = 1;
        g_host.sample_rate = 44100;
        g_host.frames_per_block = 128;
        g_host.get_clock_status = host_clock;
        g_host.get_bpm = host_bpm;
        g_host.get_beat_position = host_beat;
        api = move_plugin_init_v2(&g_host);
    }
    return api ? &engine : NULL;
}
