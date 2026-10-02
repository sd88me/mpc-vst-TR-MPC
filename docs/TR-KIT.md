# TR-MPC (modular version), prototype

Sixteen slots; each slot picks any of 49 voices (6W6 8, 8W8 16, CW-78 14, 9W9 11) via `sNN_src`, with Level, Tune, Decay, Drive, Rev and Dly forwarded to that voice's own pots, plus a volume per kit. Pads: MIDI notes 36-51 -> slots 1-16.

How: `ports/trkit/src/trkit.cpp` links the four unmodified engines in one plugin and routes triggers and keys. No Schwung adapter is involved. Parameter list: `tools/gen_trkit_params.py` -> `ports/trkit/params.json` (116 params, order is append-only).

Limits of this first version (engines expose one mono mixed bus each, no per-voice output):
- Two slots on the same voice of the same kit share that voice.
- Reverb, delay, compressor and master stage are per kit, not per slot. No per-slot pan.
- Only the six forwarded pots per slot are VST parameters, so only they are saved with a project; other pots of a voice (for example a kick's attack or tone) keep their defaults.
- Up to four engines render per block (an engine is created only when a slot uses it); CPU on the Force is not measured.
- Still to write: a skin (`layout.conf`; auto-layout of 49-option popups is not tried), kit-level FX controls, kit presets.

Next step that removes the main limits: add a per-voice render (or per-voice output bus) to each engine so a slot owns its voice, level and pan.

Tests: `bash tools/test_port.sh` (wrapper) passes; `ports/trkit/test/run.sh` triggers all 49 voices through the engine under ASan/UBSan and checks each is audible and decays (PASSED).
