# 8X8

A TR-808 for MPC Gen2 (Live III and later, aarch64): the voices are Lemus's TR-808 voicing board, solved sample by sample from
the service notes' netlists (github.com/lemus-dev/lemus), played from 8W8's panel with its drive, distortion, sends and effects.
8W8 stays as it is, for every MPC including Gen1, whose CPUs can't run the circuit solver.

- **Sources:** shared with 8W8 (`ports/8w8/src`), built with `SC808_ENGINE_LEMUS=1`: `src/dsp/l808_board.cpp` drives the board,
  `src/vendor/lemus` is Lemus's code with the local changes listed in its `VENDORED.md`.
- **Latency:** none; the board renders inside MPC's audio call with three solver workers.
- **Licence:** AGPL-3.0 (LICENSE), with Lemus. 8W8's own code is GPL-3.0, which allows this combination.

`python3 ports/8x8/gen_module.py` makes `module.mpc.json` from 8W8's; `python3 tools/drum_skin.py 8x8` draws the panel.
