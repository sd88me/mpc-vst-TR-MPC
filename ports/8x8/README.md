# 8X8

808 solved as a complete circuit from the service-note schematics (Lemus), part values fitted to a real unit. High quality, accurate, CPU heavy - Gen2 only.

The voices are Lemus's TR-808 voicing board (github.com/lemus-dev/lemus), solved sample by sample from the service notes'
netlists and played from 8W8's panel with its drive, distortion, sends and effects. 8W8 stays as it is for every MPC, Gen1 included, whose CPUs can't run the circuit solver.

- **CPU:** every voice is Lemus's circuit; to fit a Live III the solver runs at a lower precision than Lemus's own
  (`LEMUS_TOL`, `LEMUS_LINTOL` in `vst.json`).
- **Sources:** shared with 8W8 (`ports/8w8/src`), built with `SC808_ENGINE_LEMUS=1`: `src/dsp/l808_board.cpp` drives the board,
  `src/vendor/lemus` is Lemus's code with the local changes listed in its `VENDORED.md`.
- **Two boards:** the 808 shares its tom circuits with the congas, its rim shot with the claves and its clap with the maracas,
  switched on the panel. 8X8 runs a second board with those switches thrown, so all sixteen pads can ring at once. A quiet
  board costs nothing.
- **Latency (FX/MASTER):** 0-3 blocks of 128 frames, 2.9 ms each.
  - At 0 the boards render inside MPC's audio call.
  - Above 0, a thread of the plugin's own renders that many blocks ahead.
  - The default is 2. MPC's audio threads preempt ours on the fast cores, and at 1 a busy pattern missed blocks (Live III,
    2026-11-18).
  - The setting is saved with the project, so a project saved at 1 reopens at 1.
- **Controls an 808 doesn't have** have their scale drawn in orange on the panel. An orange triangle marks each one's default,
  where it leaves the 808 exactly as built.
  - **Part swaps:** kick and snare Tune; tom and conga Decay; rim shot, claves, clap and maracas Tune and Decay; cowbell
    and closed hat Decay. The hardware has no knob for these, so each one changes a part (a tuning capacitor, a decay
    resistor). Turning one builds a new board with the changed parts in the background. The next hits play on the new
    board while the old one plays its tails out, so nothing is cut.
  - **Cowbell Tune:** moves the cowbell's tuning trimmers.
  - **Closed hat, open hat and cymbal Tune:** these share one metal oscillator bank, as on the 808, so whichever was
    turned last sets the pitch for all three.
  - **Kick and maracas Attack:** a transient on the first few milliseconds of each hit.
- **Licence:** AGPL-3.0 (LICENSE), with Lemus. 8W8's own code is GPL-3.0, which allows this combination.

`python3 ports/8x8/gen_module.py` makes `module.mpc.json` from 8W8's; `python3 tools/drum_skin.py 8x8` draws the panel.
