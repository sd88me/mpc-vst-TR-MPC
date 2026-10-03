# TR-MPC (modular version), prototype

Sixteen slots; each slot picks any of 49 voices (6W6 8, 8W8 16, CW-78 14, 9W9 11) via `sNN_src`, with Level, Tune, Decay, Drive, Rev and Dly forwarded to that voice's own pots, plus a volume per kit. Pads: MIDI notes 36-51 -> slots 1-16.

How: `ports/trkit/src/trkit.cpp` links the four engines in one plugin. The engines are the vendored DSP plus a small per-voice tap (`ports/trkit/patches/<kit>.patch`, applied by `tools/apply_trkit_patches.sh` into the git-ignored `ports/trkit/engines/`; `ports/<kit>/src` stays byte-identical to upstream). With a tap set, an engine renders only its voices and hands each voice's post-fader dry sample to trkit, which applies the slot's pan and sends and runs ONE reverb, delay and master stage (8W8's own, `sc808_fx_stereo`) for the whole kit. 6W6 also gets an activity gate in tap mode (its metal lanes otherwise render when silent). Parameter list: `tools/gen_trkit_params.py` -> `ports/trkit/params.json` (159 params; append-only once released).

Per slot: Voice (49 options), Level, Tune, Decay, Drive (the voice's own pots), Pan, Rev, Dly (the slot's). Shared: Rev decay/tone/HPF/level, Dly time/feedback/tone/HPF/level, Master dist/drive, Comp, Volume. Host tempo reaches the synced delay.

Limits:
- Two slots on the same voice of the same kit share that voice (one engine voice, retriggered); they can differ in pan and sends.
- Only Level, Tune, Decay and Drive of a voice are VST parameters, so other pots (a kick's attack or tone) keep their defaults and are not saved.
- An engine or the FX stage that has been silent for 5 s is not rendered until the next trigger.
- Bench (Force): before the shared chain every stage was one FX chain per kit (worst p99 36.7%); see `bench.txt` once re-measured.

Still to do: install and listen on the Force, kit presets, release.

Tests: `bash ports/trkit/test/run.sh` applies the patches and triggers all 49 voices through the plugin under ASan/UBSan (each must be audible and decay), then checks that Pan moves a slot and Rev adds a tail (PASSED).
