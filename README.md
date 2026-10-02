# mpc-vst-TR-MPC

Four classic drum machines as native VST2 instruments for Akai MPC OS standalone devices (Force, MPC Live / Live II, One, X, Key 61), each with its own MPC touchscreen skin and Q-Link mapping:

| Plugin | Models | Voices | Install |
|---|---|---|---|
| **6W6** | Roland TR-606 | 8 | [latest 6W6 release](../../releases?q=6w6-vst) |
| **8W8** | Roland TR-808 | 16 | [latest 8W8 release](../../releases?q=8w8-vst) |
| **CW-78** | Roland CR-78 CompuRhythm | 14, plus 17 preset rhythm buttons | [latest CW-78 release](../../releases?q=cw78-vst) |
| **9W9** | Roland TR-909 | 11 | [latest 9W9 release](../../releases?q=9w9-vst) |

## These are straight ports

All the sound design is by **athousanddetails**, who wrote the original [Schwung](https://github.com/charlesvestal/schwung) modules for Ableton Move: [schwung-6W6](https://github.com/athousanddetails/schwung-6W6), [schwung-8W8](https://github.com/athousanddetails/schwung-8W8), [schwung-cw-78](https://github.com/athousanddetails/schwung-cw-78) and [schwung-9W9](https://github.com/athousanddetails/schwung-9W9). Their DSP is vendored here **unmodified** (`ports/<kit>/src`, with the upstream commit recorded in each `VENDORED.md`), so the kits sound the way the originals do and the plugins keep athousanddetails as manufacturer.

What this repo adds is only the MPC side:
- the VST2 wrapper and parameter list, via the Schwung adapter in [mpc-vst-plugins](https://github.com/sd88me/mpc-vst-plugins);
- an MPC touchscreen skin and Q-Link pages per kit (`layout.conf`);
- a remap of the MPC drum-pad MIDI notes (0-15) onto each kit's own voice map, so the pads of an MPC drum program play the right drums;
- build, offline test and release automation.

Please support and credit the original author. Everything is GPL-3.0, as upstream.

## What the plugins do

All four share the same design. Every voice is synthesised (9W9 also uses three samples, below). Every continuous control is a 0-127 pot like the hardware, and every voice has Tune, Decay, Drive, a distortion type, Level and, except the kick, Reverb and Delay sends.

- **Seven distortion characters** per voice and on the master bus: Diode, Clip, SAT, BFZ, PDIST, Fold, Crush. Drive fully down is exactly dry.
- **Send FX**: a reverb (combs and allpasses with a 12-bit loop) and a tempo-synced delay (note divisions, slewed like tape). Both are silent at zero.
- **Master**: master distortion and drive, a one-knob bus compressor (hard bypass at zero), volume, velocity depth and hat choke options.
- **Velocity** plays: the loudest hit is always the top level, and the Velocity control sets how far softer hits fall below it.

### 6W6, TR-606
Eight voices, fitted against recordings of a real TR-606: bass drum, snare (with a Decay control), low and high tom, closed and open hat, cymbal, plus a hand clap on the spare pad. The hats and cymbal use banks of measured partials from hardware. Kick Attack goes from a softened onset to the original punch. Hat choke is a switch (Off, CH cuts OH, Mutual).

### 8W8, TR-808
Sixteen voices. Fifteen are circuit models built from the service notes: a bridged-T bass drum where Decay is loop gain, two-shell snare, low/mid/hi tom and conga, claves, maracas, hand clap, cowbell, hats and cymbal. The cowbell, both hats and the cymbal share one free-running metal oscillator bank like the hardware. The rim shot is a transcription of Sonic Pi's sc808. It fills all 16 pads.

### CW-78, CR-78 CompuRhythm
Fourteen voices modelled from the 1979 service notes and Roland's factory alignment figures: kick, snare, low/hi bongo, low conga, rim shot, claves, hi-hat, cymbal, maracas, cowbell, tambourine, guiro and the Metallic Beat. The machine's **preset rhythms** are included, transcribed from the service notes: 17 buttons with an A/B lever (34 rhythms), and a second button can be combined with the first as on the hardware. They play from the MPC transport on the Rhythm page.

### 9W9, TR-909
Circuit-modelled kick, snare, three toms, rim shot and hand clap; **sampled** hi-hats, ride and crash, the same split the real 909 used. The cymbal samples ship with the plugin (`engine/samples/` next to the `.so`) and come from [ER-99](https://github.com/matthewcieplak/er-99), GPL-3.0. Accent sets the full-velocity level; closed and open hat choke each other.

## Install

Needs a first-generation MPC OS standalone with root SSH (a modded unit). Take the zip from the plugin's release, unzip it, copy the folder to the device and run `install.sh` (it stops MPC, so save first):

```
scp -r 6W6-1.0.0 root@<device-ip>:/tmp/
ssh root@<device-ip> sh /tmp/6W6-1.0.0/install.sh
```

Then add the plugin to a track from the plugin browser. `INSTALL.md` in each zip has the manual steps and `uninstall.sh` removes it. Tested on a Force. Installing plugins this way is unofficial, so back up first.

## CPU (Force, one core, 44.1 kHz / 128 frames, 16 voices)

| Plugin | p99 | Worst block | Verdict |
|---|---|---|---|
| 6W6 | 30.9% | 31.6% | WARN: one or two instances per project |
| 8W8 | 13.1% | 13.6% | PASS |
| CW-78 | 7.7% | 9.0% | PASS |
| 9W9 | 21.2% | 22.9% | WARN: one or two instances per project |

## Status

- v1.0.0 of each port is released and listed in the mpc-vst-plugins catalog.
- The skins are auto-generated layouts (palette from the original web UIs). Knob caps, plates and the web UIs' sequencer and pad strip are not reproduced.
- Not yet checked by ear: the pad-to-voice mapping against every MPC drum-program layout.
- **TR-MPC is coming soon** (see below).

## Coming soon: TR-MPC

TR-MPC is intended to be one modular drum kit that combines all four machines: 16 pad slots, where each slot picks any of the 49 voices from 6W6, 8W8, CW-78 and 9W9 (a 606 kick next to an 808 snare and 909 hats, for example). Each slot gets its own Level, Tune, Decay, Drive and Reverb/Delay sends, mapped to the MPC pads (notes 36-51 or the drum-pad patch's notes 0-15), with one MPC skin and Q-Links for the whole kit. It reuses the same unmodified engines, so the sound stays the original author's.

A prototype exists (`ports/trkit`, `docs/TR-KIT.md`), but it has no skin yet, its CPU cost on the Force is unmeasured, and it isn't released. The four plugins above are the finished, standalone versions.

## Building

Needs Docker and a sibling checkout of [mpc-vst-plugins](https://github.com/sd88me/mpc-vst-plugins), in a path without spaces:

```
bash tools/build_ci.sh 9w9      # armhf .so + skin in ports/9w9/build
bash tools/test_ci.sh 9w9       # offline ASan/UBSan host test
```

`tools/extract_module.py` builds `module.mpc.json` (chain_params and ui_hierarchy) from a kit's generated `*_params.h`; `tools/build_all.sh` builds everything and renders skin previews. Releases: Actions, "Release <kit> (draft)" (`.github/workflows/release-<kit>.yml`, tags `<kit>-vst-v<version>`), then install the draft's zip on a device, smoke-test it and publish. `patch/` holds an experimental MPC OS drum-pad patch.

## Credits and licence

Sound design, DSP and the original modules: **athousanddetails**. 9W9's cymbals and early engine: Matthew Cieplak's ER-99. 8W8's rim shot: Sonic Pi's sc808. Schwung is by Charles Vestal. Not affiliated with or endorsed by Roland or Akai; TR-606, TR-808, CR-78 and TR-909 are Roland trademarks, used only to describe what is modelled. GPL-3.0, see `LICENSE`.

## Optional drum-pad firmware patch
Not part of any plugin release. A standalone script (`patch/mpc-drum-pad-patch.sh`) gives 6W6, 8W8, CW-78, 9W9, TR-MPC and Machinedrum Module the 16-pad drum layout on MPC OS 3.9.1.2. It modifies Akai's factory MPC program; read `patch/README.md` first.
