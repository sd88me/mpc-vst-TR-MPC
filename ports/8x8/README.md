# 8X8

**An accurate, circuit-solved TR-808 for MPC Gen2.** Every voice is the 808's own circuit, solved from the service-note
schematics by [Lemus](https://github.com/lemus-dev/lemus) and calibrated to a real unit, for a sound faithful to the
original machine. On top of that, 8X8 adds what the hardware never had: drive and seven distortion types per voice, reverb
and delay sends, a master bus with distortion and compression, and extra controls on every voice.

High quality and CPU heavy: Gen2 only (MPC Live III and later). For every other MPC, 8W8 has the same panel with lighter,
hand-modelled voices.

## Playing it

- **All sixteen 808 voices**, congas, claves and maracas included, on all 16 pads, and they can all ring at once.
- **Extra controls** the 808 doesn't have are marked on the panel: their scale is orange, and an orange triangle shows the
  default, where the voice sounds exactly like the original.
  - **Tune and Decay** on voices the 808 left fixed (kick and snare Tune; tom and conga Decay; rim shot, claves, clap and
    maracas Tune and Decay; cowbell and closed hat Decay), made by changing the circuit's own parts. Notes already ringing
    finish as they were.
  - **Cowbell Tune**, and **closed hat, open hat and cymbal Tune**. The hats and cymbal share one set of oscillators, as on
    the 808, so the last of the three you turn sets the pitch for all of them.
  - **Kick and maracas Attack**, for extra click at the start of each hit.
- **Latency** (FX/MASTER), 0-3 blocks of 2.9 ms each: more blocks give the circuits more time to render. The default of 2
  plays busy patterns cleanly on a Live III; lower it only for light parts. It is saved with the project.

## Licence

AGPL-3.0 (`LICENSE`), with Lemus. 8W8's own code is GPL-3.0, which allows this combination.

## Building

Built from 8W8's sources (`ports/8w8/src`) with `SC808_ENGINE_LEMUS=1`; the Lemus code is in `src/vendor/lemus`, with its
local changes in `VENDORED.md`. `python3 ports/8x8/gen_module.py` makes `module.mpc.json` from 8W8's, and
`python3 tools/drum_skin.py 8x8` draws the panel.
