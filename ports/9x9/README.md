# 9X9

A TR-909 for MPC Gen2 (Live III and later, aarch64): the voices are Lemus's TR-909 voicing board, solved sample by sample from
the service notes' netlists (github.com/lemus-dev/lemus), played from 9W9's panel with its drive, distortion, sends and effects.
9W9 stays as it is, for every MPC including Gen1, whose CPUs can't run the circuit solver.

- **Sources:** shared with 9W9 (`ports/9w9/src`), built with `ER99_ENGINE_LEMUS=1`: `src/dsp/l909_board.cpp` drives the board,
  `src/vendor/lemus` is Lemus's code with the local changes listed in its `VENDORED.md`.
- **Hi-hat, crash and ride:** Lemus plays the 909's own sample ROMs, which aren't included. Put your copies of
  `hn61256p__c43.ic69` (hi-hat), `hn61256p__c42.ic62` (crash) and `hn61256p__c44.ic54` (ride) in the plugin folder's
  `engine/roms/`; they are checked by CRC. Without one, 9W9's own sample plays that voice.
- **Latency (FX/MASTER):** 0-3 blocks of 128 frames, 2.9 ms each. At 0 the board renders inside MPC's audio call; above 0 a
  thread of its own renders that many blocks ahead. Default 2: inside MPC its audio threads preempt ours on the fast cores, and
  at 1 a busy pattern missed blocks (Live III, 2026-11-18).
- **Controls a 909 doesn't have** are marked on the panel (their names on a pill). At its default each leaves the 909 as it
  is: kick Sweep (P.Depth: the pitch envelope's depth) and Pitch Mod (the whole pitch) on the kick's VCO model; tom Attack, a
  transient on the first few ms; clap, ride and crash Decay, which shorten the tail; closed and open hat Tune, the hi-hat
  ROM's clock (0.5x-2x; up, the hat gets thinner, as a clock mod does). Rim and clap Tune do nothing yet (they need the
  circuits' parts changed).
- **Licence:** AGPL-3.0 (LICENSE), with Lemus. 9W9's own code is GPL-3.0, which allows this combination.

`python3 ports/9x9/gen_module.py` makes `module.mpc.json` from 9W9's; `python3 tools/drum_skin.py 9x9` draws the panel.
