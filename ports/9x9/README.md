# 9X9

**An accurate, circuit-solved TR-909 for MPC Gen2.** The kick, snare, toms, rim shot and clap are the 909's own circuits,
solved from the service-note schematics by [Lemus](https://github.com/lemus-dev/lemus) and calibrated against recordings
of real units; the hats and cymbals play the 909's own sample ROMs, as the original does. On top of that, 9X9 adds what the
hardware never had: drive and seven distortion types per voice, reverb and delay sends, a master bus with distortion and
compression, and extra controls on several voices.

High quality and CPU heavy: Gen2 only (MPC Live III and later). For every other MPC, 9W9 has the same panel with lighter
voices modelled after ER-99.

## The hi-hat and cymbal ROMs

The 909's sample ROMs aren't included. Put your own copies of `hn61256p__c43.ic69` (hi-hat), `hn61256p__c42.ic62` (crash)
and `hn61256p__c44.ic54` (ride) in the plugin folder's `engine/roms/`; 9X9 checks each one. Without them, those voices
play 9W9's samples instead.

## Playing it

- **Extra controls** the 909 doesn't have are marked on the panel: their scale is orange, and an orange triangle shows the
  default, where the voice sounds exactly like the original.
  - **Kick Sweep** deepens the kick's pitch drop, and **Pitch Mod** moves its whole pitch.
  - **Tom Attack** adds click at the start of each hit.
  - **Clap, ride and crash Decay** shorten the tail; at the default, the full 909 tail plays.
  - **Closed and open hat Tune** speed up or slow down the hat ROM, 0.5x-2x, the way a clock mod does.
- **Latency** (FX/MASTER), 0-3 blocks of 2.9 ms each: more blocks give the circuits more time to render. The default of 2
  plays busy patterns cleanly on a Live III; lower it only for light parts. It is saved with the project.

## Licence

AGPL-3.0 (`LICENSE`), with Lemus. 9W9's own code is GPL-3.0, which allows this combination.

## Building

Built from 9W9's sources (`ports/9w9/src`) with `ER99_ENGINE_LEMUS=1`; the Lemus code is in `src/vendor/lemus`, with its
local changes (including those that make it fit a Live III's CPU) in `VENDORED.md`. `python3 ports/9x9/gen_module.py`
makes `module.mpc.json` from 9W9's, and `python3 tools/drum_skin.py 9x9` draws the panel.
