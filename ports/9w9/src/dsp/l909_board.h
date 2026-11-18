/*
 * l909_board.h — 9X9's voices: Lemus's TR-909 voicing board (circuit solved from the service
 * notes' netlists, src/vendor/lemus), with an optional render-ahead.
 *
 * Latency 0: the board renders on the audio thread (and its solver workers) inside each render
 * call: no added delay, but a block where several voices trigger can overrun the audio deadline.
 * Latency 1-3: a thread of ours (pinned to the fast cores, below MPC's audio priority) renders the
 * board that many 128-frame blocks ahead (2.9 ms each); render() only copies, and hits land that
 * many blocks later. On the Live III, 2 blocks never missed a deadline in a 150 s soak of a busy
 * beat with all ten voices hit at once every bar; 1 block missed only on the all-ten hits.
 * The latency can change while playing: up or down between 1-3 without a gap; to and from 0 at a
 * block boundary, the blocks already rendered playing out first.
 *
 * A hit is a 909 trigger: its accent DAC code (0-63) and a 2 ms trigger pulse, as the 909's CPU
 * writes them. Each hit carries a tag (any float) that comes back with the block it sounds in, so
 * the caller can apply per-hit gains in time with the sound.
 *
 * Realtime: trigger(), set_controls(), set_latency() and render() never allocate or lock.
 *
 * AGPL-3.0 (with Lemus).
 */
#ifndef L909_BOARD_H
#define L909_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#define L909_LANES 10   /* the board's jacks: BD SD LT MT HT RS HC HH CR RD */
#define L909_BLOCK 128
#define L909_OH    10   /* trigger() voice for the open hat (lane 7, the HH jack; 7 is the closed hat) */
#define L909_MAX_LATENCY 3

typedef struct l909 l909_t;

/* The panel's voice pots as rotations, 0..1 (Lemus's Controls). */
typedef struct {
    float bd_tune, bd_attack, bd_decay;
    float sd_tune, sd_tone, sd_snappy;
    float lt_tune, lt_decay, mt_tune, mt_decay, ht_tune, ht_decay;
    float ch_decay, oh_decay, cr_tune, rd_tune;
} l909_controls_t;

/* Builds and settles the board (not realtime: about a second of rendering). rom_dir holds the
 * user's hn61256p__c43.ic69 (hi-hat), __c42.ic62 (crash) and __c44.ic54 (ride), checked by CRC;
 * NULL or a missing/wrong file leaves that voice silent (see l909_rom_ok). */
l909_t *l909_create(float sample_rate, const char *rom_dir, const l909_controls_t *initial);
void l909_destroy(l909_t *b);

/* 0 hi-hat, 1 crash, 2 ride: whether that ROM loaded (its voice sounds). */
int l909_rom_ok(const l909_t *b, int which);

void l909_set_controls(l909_t *b, const l909_controls_t *c);

/* 0..L909_MAX_LATENCY blocks. Takes effect at the next block boundary. */
void l909_set_latency(l909_t *b, int blocks);
/* The latency new hits get now (after a change, once it has taken effect). */
int l909_latency(const l909_t *b);

/* A hit at the next render(): voice 0-9 (lane), or L909_OH; code 0-63 the accent DAC. */
void l909_trigger(l909_t *b, int voice, int code, float tag);

/* n frames of every lane (volts / 10). Per lane, hit_voice[l] is the voice (lane, or L909_OH) of
 * the last hit that started in these frames, -1 for none, and tag[l] its tag. */
void l909_render(l909_t *b, int n, float *const lanes[L909_LANES], int hit_voice[L909_LANES], float tag[L909_LANES]);

/* Blocks the render-ahead thread didn't finish in time (silence was played). */
long l909_underruns(const l909_t *b);

#ifdef __cplusplus
}
#endif

#endif
