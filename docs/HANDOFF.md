# Handoff: TR-MPC (ports/trkit)

State as of 2026-10-05. Read README.md first. The four drum ports (6W6, 8W8, CW-78, 9W9) are released (v1.0.0, 1.1.0 skins) and
untouched by this work; **TR-MPC** (the modular 16-slot kit) is built, installed on the Force and working, but **not released**.

## What TR-MPC is now
One VST2 instrument (`ports/trkit`, uid `TRMP`, vendor sd88me, file `trmpc.so`). Sixteen slots; each slot picks any of the 49 voices
(6W6 8, 8W8 16, CW-78 14, 9W9 11) from the four vendored engines. Pads: MIDI notes 36-51 (or 0-15 with the drum-pad patch) = slots 1-16.

### Architecture (`ports/trkit/src/trkit.cpp`)
- **Engines + tap patches.** `ports/<kit>/src/dsp` stays byte-identical to upstream. `ports/trkit/patches/<kit>.patch` adds a per-voice
  tap (and 8W8 gets `sc808_fx_stereo`, its post-voice stage run once on summed buses); `tools/apply_trkit_patches.sh` builds the
  git-ignored `ports/trkit/engines/<kit>/` from them (`tools/make_trkit_patches.py` is how they were made). With a tap set an engine
  renders only its voices and hands each voice's post-fader dry sample to trkit.
- **Mix.** trkit applies each slot's pan (constant power) and its Rev/Dly sends, then runs ONE reverb, delay, master distortion, comp
  and volume (8W8's own, stereo) for the whole kit, then an output soft limiter (`soft_limit`, knee 0.5; without it 4+ voices
  hard-clipped and crackled; see the gain-staging note below). Engines and the FX stage sleep after 5 s of silence (CPU).
- **Parameters.** `tools/gen_trkit_params.py` writes `params.json` (391 params; the list is append-only once released). Per slot:
  src, kit (derived, read-only: 0-3 the kit, 4 = the 9W9 kick), level, tune, decay, drive, dist, attack/tone/snappy/noise/rate/sweep/
  pmod/ndecay/sat (the voice's own pots, ignored by voices without them), pan, rev, dly. Plus editor proxies (`edit_slot`,
  `edit_voice`, `e_*`, used by the editor/restyle skins), FX (`fx_*` = 8W8's keys), and the randomise module (`rnd_*`).
- **Project recall.** The wrapper saves whatever the engine returns for the `state` key. trkit serialises every slot value, the FX
  values, edit slot and the randomise settings as `trmpc1;key=value;...` (about 3.8 KB of the 8 KB chunk).
- **Randomise.** `rnd_s01..16` pick slots, `rnd_amount` how far, `rnd_voice` also swaps to another voice of the same kit,
  `rnd_go` (momentary) does it, `rnd_all`/`rnd_none` select. Level is never touched.
- Known limits: two slots on the same voice share it (pan and sends still differ); only Level/Tune/Decay/Drive/Dist plus the nine
  extras of a voice are saved, other engine pots keep defaults.

### Skins (`tools/gen_trmpc_skin.py`, `tools/gen_trmpc_full.py`)
- `layouts/restyle.conf`, `layouts/editor.conf`: earlier experiments (rack look; rack overview + one editor). `layouts/full.conf` is the
  chosen one and is what `ports/trkit/layout.conf` is (build_ci.sh copies `layouts/<style>.conf` over it when you pass a style).
- **Full style:** each slot panel reskins itself to the kit of the selected voice (`sNN_fam` + `when=` lines). Everything visual comes
  from the drum ports: faceplates are `gen_drum_skins.plate_svg` cropped to one panel, knobs are the ports' built filmstrips (copied
  from `_build/<kit>/skin/*/Plugin Skins/sh_knob_r26_<look>.png` into `ports/trkit/images/knob_*.png`; run `tools/build_all.sh` to refresh),
  colours come from `gen_drum_skins.STYLES`, lettering is outlined (`text_path`, needs fontTools + `../mpc-vst/Assets/fonts`).
  Run the generators with `bash tools/gen_skins_docker.sh gen_trmpc_full.py` (and `gen_trmpc_skin.py`).
- Five-row layout (r=22, label scale 0.95); the 9W9 kick has three extra controls so it gets a six-row layout (`fam` = 4).
  The voice menu is the module title (live text in the kit's colour); its open list is grouped and coloured per kit.
- Global page (`FX - RANDOMISE`): REVERB / DELAY, MASTER, SELECT SLOTS (16 LED toggles), RANDOMISE. Lettering there is orange.
- **Needs mpc-vst-plugins** features (merged and pushed to its main; `release-trmpc.yml` pins 73d5dfd5912b): per-control `ink=`/`ink_dim=`
  (knob label colours), popup `accent=<hex|none>`, `field=none`, `cw=` (list cell width) and
  `groups="Title:count[:headFill[:headInk[:optFill[:optInk]]]]"`.
- Cost: `TUI.json` is ~20 MB (about 1,100 conditional components); the zip is ~20 MB. Pages load a little slowly; watch memory (the
  Force had ~50 MB free with the add-ons running). Possible savings: fewer conditional pieces, or the editor style.

## Verified on the Force (192.168.1.44, MPC OS 3.9.1.2, root SSH)
- Full skin installed (0.4.1): kit reskinning, voice menu, project recall, sound OK after the limiter. Not yet seen on the device:
  the latest tweaks (global page with randomise, 5-row geometry, watermark/screws). Last bench: p99 23.5% (WARN), 16 voices 16.6%.
- Install route that survives a flaky network: build a package with `../mpc-vst-plugins/tools/release.py` (see the commands in the git
  log / below), `scp` the single zip to the device, `unzip` it there, check `sha256sum -c SHA256SUMS`, run `sh install.sh -y`
  (stops/restarts MPC; backs up MPC.settings). Do not scp the 2,000-file folder.

```
B=ports/trkit/build
bash tools/build_ci.sh trkit full          # patches + generators + skin + .so   (copies layouts/full.conf over layout.conf)
python3 ../mpc-vst-plugins/tools/release.py --so $B/trmpc.so --skin "$B/skin/sd88me - VST - TR-MPC" --entry $B/pluginlist-entry.xml \
  --version 0.5.0 --extra ports/9w9/src/samples:engine/samples --bench ports/trkit/bench.txt --license GPL-3.0-only -o <dist dir>
bash ports/trkit/test/run.sh               # offline audio/wrapper tests (ASan); build_ci + bench.sh for the CPU number
bash ../mpc-vst-plugins/tools/bench.sh $B/trmpc.so 192.168.1.44 -j
```
- /tmp fills up (other sessions); previews of the full style take ~10 min (`prev.sh`-style: studio.py preview in Docker; keep only
  the pages you need; the mode images number in the thousands).

## Gain staging and clipping: what Machinemodule does (reference)
Machinemodule (mpc-vst-machinedrum `engine/Mixer.cpp`) translates the Machinedrum's DSP1 mixer to C++: per track a constant-power
pan from sin/cos tables times a VOL gain (word-length fixed point), sends taken after the track's L/R gains, sums in 48-bit
accumulators with the hardware's `<<3`/`<<4` shifts, then ONE saturating `lim()` at 24-bit full scale; silent tracks are skipped.
So it also hard-limits at the end, but its gains are the hardware's, so normal use stays under full scale. TR-MPC sums float (no
intermediate clipping) and ends in a soft limiter, which is gentler. Worth copying: sends after pan (stereo sends), skip silent
voices (done per engine here), and the Tap design below.

## Next steps
1. **Install and check the latest skin** on the Force (global page with randomise, bigger knobs, watermark bottom-left, screws on all
   panels, kit-coloured voice lists). Fix whatever looks off; send screenshots.
2. **Tap / insert modules (the user wants this).** Model it on Machinemodule's Tap plugins (`mpc-vst-machinedrum/vst/tap_shared.h`,
   `vst/tap/`, `vst/tapfx/`): the primary plugin owns the engine and publishes per-source audio planes into a shared struct that the tap
   plugins (in the same MPC process) find via an exported `md_tap_shared()` (`dlopen(RTLD_NOLOAD)`); each tap has a source mask, taps
   are sample-aligned, and a tapped track leaves the primary's main mix. For TR-MPC: publish 16 slot planes (mono post-voice, or stereo
   post-pan) plus reverb and delay send planes; add "TR-MPC Tap" (instrument) and "TR-MPC Tap FX" (effect, with THRU) so slots can be
   routed to their own MPC tracks, submixes and insert chains. Needs: shared header, exported symbol in `trmpc.so`, two small vst.json
   ports, pluginlist entries, installer bundling, and a bench of the extra cost.
3. **Baked per-voice titles** (exact drum-port lettering instead of live text): one small art line per voice per slot (~800 pieces);
   only if the skin has headroom.
4. **Release:** merge/push the plugins branch and re-pin the workflow; dry run `Release TR-MPC (draft)`; add the catalog entry
   (separate PR in mpc-vst-plugins); replace the README "Coming soon" section with real docs; commit `ports/trkit/bench.txt`.
5. Optional: kit presets; a quick check that pan/sends on two slots sharing one voice behave as documented.

## Open items carried over
- Drum-pad patch (`patch/`) is MPC OS 3.9.1.2 only.
- Pad-to-voice map not verified against every MPC drum-program layout.
- CW-78 shows as GitHub "Latest" release in this repo (cosmetic).
