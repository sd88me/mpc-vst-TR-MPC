# 9X9

A TR-909 for MPC Gen2 (Live III and later, aarch64): the voices are Lemus's TR-909 voicing board, solved sample by sample from
the service notes' netlists (github.com/lemus-dev/lemus), played from 9W9's panel with its drive, distortion, sends and effects.
9W9 stays as it is, for every MPC including Gen1, whose CPUs can't run the circuit solver.

- **Sources:** shared with 9W9 (`ports/9w9/src`), built with `ER99_ENGINE_LEMUS=1`: `src/dsp/l909_board.cpp` drives the board,
  `src/vendor/lemus` is Lemus's code with the local changes listed in its `VENDORED.md`.
- **Hi-hat, crash and ride:** Lemus plays the 909's own sample ROMs, which aren't included. Put your copies of
  `hn61256p__c43.ic69` (hi-hat), `hn61256p__c42.ic62` (crash) and `hn61256p__c44.ic54` (ride) in the plugin folder's
  `engine/roms/`; they are checked by CRC. Without one, 9W9's own sample plays that voice.
- **Latency (FX/MASTER):** 0-3 blocks of 128 frames, 2.9 ms each.
  - At 0 the board renders inside MPC's audio call.
  - Above 0, a thread of the plugin's own renders that many blocks ahead.
  - The default is 2. MPC's audio threads preempt ours on the fast cores, and at 1 a busy pattern missed blocks (Live III,
    2026-11-18).
  - The setting is saved with the project, so a project saved at 1 reopens at 1.
- **Controls a 909 doesn't have** have their scale drawn in orange on the panel. An orange triangle marks each one's default,
  where it leaves the 909 exactly as it is.
  - **Kick Sweep:** deepens the pitch envelope on the kick's VCO model. Its default is fully down, the 909's own depth.
  - **Kick Pitch Mod:** moves the kick's whole pitch, up or down from its default.
  - **Tom Attack:** above its default, a transient on the first few milliseconds of each hit; at or below it, none.
  - **Clap, ride and crash Decay:** below the default they shorten the tail; at the default and above, the 909's full tail
    plays. The defaults are 63 for the clap and 108 for the ride and crash.
  - **Closed and open hat Tune:** the hi-hat ROM's clock, 0.5x-2x. Turning it up makes the hat thinner, as a clock mod
    does.
  - Rim shot and clap have no Tune: 9W9's panel has them, but on the circuit they need the parts changed, which isn't done
    yet, so 9X9 leaves them off its panel. Their parameters remain, so projects and automation still load.
- **Licence:** AGPL-3.0 (LICENSE), with Lemus. 9W9's own code is GPL-3.0, which allows this combination.

`python3 ports/9x9/gen_module.py` makes `module.mpc.json` from 9W9's; `python3 tools/drum_skin.py 9x9` draws the panel.
