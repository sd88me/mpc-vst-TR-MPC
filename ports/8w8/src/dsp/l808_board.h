/*
 * l808_board.h — 8X8's voices: Lemus's TR-808 voicing board (circuit solved from the service
 * notes' netlists, src/vendor/lemus), driven from 8W8's sixteen lanes.
 *
 * The 808 has eleven voice circuits. Its congas share the toms' circuits, the claves the rim
 * shot's and the maracas the clap's, picked by switches. So that all sixteen pads can ring at once,
 * there are two boards: MAIN (the panel as it ships) and ALT (SW8-SW12 thrown: congas, claves,
 * maracas). A board's idle voices sleep and a quiet board is skipped, so ALT costs nothing until
 * its pads are played. The cymbal and hats share MAIN's metal source, as on the 808: their Tune
 * knobs move it together (the last one turned wins). Hosting the cymbal on ALT to tune it apart
 * woke a whole second board on every cymbal hit, 8-13 % of a core.
 *
 * Knobs come in two kinds:
 *   LIVE    a pot the hardware has (BD tone/decay, SD tone/snappy, the toms' tuning, OH and CY
 *           decay, accent) or a trimmer Lemus moves without a rebuild (the cowbell's TM1/TM2).
 *   REBUILD a part value (a tuning capacitor, a decay resistor): the hardware has no knob, so it is
 *           a part swap. A new pair of boards is built on a worker thread with the new parts and
 *           takes the next hits; the old pair keeps rendering until every tail on it has died, then
 *           is handed back to the worker to free. No crossfade is needed: nothing is cut.
 *           LEMUS_TUNE_REBUILD=0 compiles this out (the knobs then do nothing).
 *
 * Latency (lemus_ahead.h): 0 renders the boards on the audio thread inside render(); 1-3 on a thread of ours (fast
 * cores, below MPC's audio priority) that many 128-frame blocks ahead, render() then only copies. Hits and knobs
 * land in the block the latency puts them in; a hit's tag comes back with the block it sounds in (hit()).
 *
 * Realtime: trigger/render/setKnobs never allocate or lock on the audio thread.
 *
 * AGPL-3.0 (with Lemus).
 */
#ifndef L808_BOARD_H
#define L808_BOARD_H

#ifndef LEMUS_TUNE_REBUILD
#define LEMUS_TUNE_REBUILD 1
#endif

namespace l808 {

enum { kLanes = 16, kMaxBlock = 256 };

/* Per-lane knob values, all 0..1 pot positions (raw / 127), plus tune as a frequency ratio. */
struct LaneKnobs {
    float ratio = 1.0f;   /* tune: frequency ratio (1 = as built) */
    float tunePot = 0.5f; /* tune as a pot, for the toms' and congas' TUNING */
    float decay = 0.5f;   /* pot */
    float decayDef = 0.5f;/* the pot position where a REBUILD decay leaves the parts as built */
    float tone  = 0.5f;   /* BD tone */
    float snappy = 0.5f;  /* SD snappy */
};

class Engine {
public:
    /* Builds the first boards (not realtime) from the lanes' starting knobs (null: the defaults),
     * so a session's first knobs don't cost a rebuild. */
    Engine(double sampleRate, const LaneKnobs *initial = nullptr);
    ~Engine();

    /* A hit on a lane. accent01: 0 = unaccented trig; >0 = ACCENT data with VR3 at that rotation. tag: any value,
     * back from hit() after the render() whose frames the hit starts in. */
    void trigger(int lane, float accent01, float tag = 1.0f);

    /* Knob values; applied at the next render (LIVE) or after a short quiet period (REBUILD). */
    void setKnobs(int lane, const LaneKnobs &k);

    /* n <= kMaxBlock. lanes[l] receives lane l's jack (volts / 10). */
    void render(int n, float *const lanes[kLanes]);

    /* After render(): whether lane l's circuit was awake on any board pair in that block (its
     * buffer is zeros otherwise, and the lane's own processing can be skipped). */
    bool active(int lane) const;
    /* After render(): whether a hit on lane l started in it, and its tag. */
    bool hit(int lane, float *tag) const;

    /* 0..3 blocks of render-ahead (lemus_ahead.h); the latency new hits get now; blocks missed. */
    void setLatency(int blocks);
    int latency() const;
    long underruns() const;

    /* the number of board pairs rendering (1, or 2+ while an old pair's tails finish) */
    int pairs() const;

    struct Impl;
private:
    Impl *m;
};

} // namespace l808

#endif
