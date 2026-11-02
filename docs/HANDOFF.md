# Handoff: TR-MPC (ports/trkit)

State as of 2026-10-07. Read README.md first. **Released:** TR-MPC 1.0.0 (`trmpc-vst-v1.0.0`) and TR-MPC Tap FX 1.0.0
(`trmpc-tap-fx-vst-v1.0.0`), both built 2026-10-06 from `ba52516` with mpc-vst-plugins `e15ce12`; the four drum ports are at 1.1.1
(MPC OS 2.x skin shape; tested on a Force only). The sections below keep the design notes; the "Next steps" are what remains after release.

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
- **Parameters.** `tools/gen_trkit_params.py` writes `params.json` (361 params; the list is append-only now that 1.0.0 is released). Per slot:
  src, kit (derived, read-only: 0-3 the kit, 4 = the 9W9 kick), level, tune, decay, drive, dist, attack/tone/snappy/noise/rate/sweep/
  pmod/ndecay/sat (the voice's own pots, ignored by voices without them), pan, rev, dly. Plus editor proxies (`edit_slot`,
  `edit_voice`, `e_*`, used by the editor/restyle skins), FX (`fx_*` = 8W8's keys), and the randomise module (`rnd_*`).
- **Project recall.** The wrapper saves whatever the engine returns for the `state` key. trkit serialises every slot value, the FX
  values, edit slot and the randomise settings as `trmpc1;key=value;...` (about 3.8 KB of the 8 KB chunk).
- **Randomise.** `rnd_s01..16` pick slots, `rnd_amount` how far, `rnd_voice` also swaps to another voice of the same kit,
  `rnd_go` (momentary) does it, `rnd_all`/`rnd_none` select. Level is never touched.
- Two slots on the same voice share one engine voice, but only the slot that was last hit gets its sound (so a tapped slot's twin cannot leak it into the main mix); only Level/Tune/Decay/Drive/Dist plus the nine
  extras of a voice are saved, other engine pots keep defaults.

### Skins (`tools/trmpc_skin.py`, from 2026-11; drum kits: `tools/drum_skin.py`)
- The drum kits and TR-MPC are drawn after the hardware by Pillow scripts that write `layout.conf` and `art/*.png` (both committed):
  `python3 tools/drum_skin.py <kit>` and `python3 tools/trmpc_skin.py`. Each kit's look (plate, header, heading, knob) is its own
  `ports/<kit>/skin/style.py`; drum_skin.py does what must match across kits (control placement, Q-Link columns, filmstrips).
  Fonts: URW Gothic and Michroma ship in `tools/assets` with their licences; Helvetica, Earth and FilmotypeFord are baked from
  `../mpc-vst-plugins/Assets/fonts` (FONT_SRC) and never shipped. Pillow's noise texture is random per run, so regenerating rewrites
  every PNG: commit the art you tested.
- **TR-MPC modules:** each slot module is cut from its kit's panel (`style.panel()`), with the kit's heading behind the voice menu (plus a
  menu arrow), its knobs and corner screws; `sNN_fam` + `when=` lines switch it per kit (`fam` 4: the 9W9 kick, six rows). A voice's own
  extra controls are knobs whose filmstrips carry their scale and name: separate `when=` name images were merged by MPC's skin builder
  into one picture per mode and showed black gaps/edges on the device.
- **Q-Links:** a page's overview is Level/Tune/Decay/Drive per slot (`qlink_bounds=column` outlines each slot's top 2x2), `VOICES n-m`
  has the four voice menus, `SLOT n` all of one slot. Defaults mix the four kits per page (`default_flat()` in trkit.cpp and
  `DEFAULT_FLAT` in gen_trkit_params.py must match).
- Earlier styles (`tools/gen_trmpc_skin.py`, `tools/gen_trmpc_full.py`, `layouts/*.conf`) are kept for reference; `build_ci.sh trkit
  <style>` would copy one of them over `layout.conf`, so build releases with plain `build_ci.sh trkit`.
- Global page (`FX - RANDOMISE`): REVERB / DELAY, MASTER, SELECT SLOTS (16 LED toggles), RANDOMISE. Lettering there is orange.
- **Needs mpc-vst-plugins** features (merged and pushed to its main; `release-trmpc.yml` pins e15ce126d3a5): per-control `ink=`/`ink_dim=`
  (knob label colours), popup `accent=<hex|none>`, `field=none`, `cw=` (list cell width) and
  `groups="Title:count[:headFill[:headInk[:optFill[:optInk]]]]"`.
- Cost: `TUI.json` is ~20 MB (about 1,100 conditional components); the zip is ~20 MB. Pages load a little slowly; watch memory (the
  Force had ~50 MB free with the add-ons running). Possible savings: fewer conditional pieces, or the editor style.

## Verified on the Force (192.168.1.44, MPC OS 3.9.1.2, root SSH)
- Full skin installed and checked through the 1.0.0 build (kit reskinning, voice menu, FX and RANDOMISE pages, project recall, sound OK after the limiter).
- Install route that survives a flaky network: build a package with `../mpc-vst-plugins/tools/release.py` (see the commands in the git
  log / below), `scp` the single zip to the device, `unzip` it there, check `sha256sum -c SHA256SUMS`, run `sh install.sh -y`
  (stops/restarts MPC; backs up MPC.settings). Do not scp the 2,000-file folder.

```
B=ports/trkit/build
bash tools/build_ci.sh trkit               # patches + skin + .so from the committed layout.conf (python3 tools/trmpc_skin.py redraws it)
python3 ../mpc-vst-plugins/tools/release.py --so $B/trmpc.so --skin "$B/skin/sd88me - VST - TR-MPC" --entry $B/pluginlist-entry.xml \
  --version <version> --extra ports/9w9/src/samples:engine/samples --bench ports/trkit/bench.txt --license GPL-3.0-only -o <dist dir>
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

## Release state and what is left
Done in 1.0.0: full skin (kit reskinning, voice menu, global page as FX and RANDOMISE tabs with `int_rev`/`int_dly`/`int_comp` keys, bigger
knob names, jog-wheel `nudge_gain: 3` on every continuous knob), randomise (`rnd_go`, `rnd_go_voice`), output soft limiter, and Tap FX.
On-device screenshots are in `docs/img`. Bench (Force): TR-MPC p99 24.8%, worst block 27.4% (`ports/trkit/bench.txt`), WARN: one instance
per project. Tap FX has no bench number.

**Tap FX** (`ports/trkit/tapfx`, uid `TRTf`, `trmpc_tapfx.so`; the instrument tap was dropped by decision), modelled on Machinemodule's taps.
`src/trkit_tap.h` is the shared state; `trmpc.so` exports `trmpc_tap_shared()` and the tap finds it with `dlopen("trmpc.so", RTLD_NOLOAD)`
(so both must be in the same project/process). Planes: per slot post-pan stereo (int16) plus mono reverb and delay send buses; the primary
publishes a block and outputs the previous one (one block, 2.9 ms latency; same hostRead/hostCallUs scheme as MD), filled only while a tap is
registered. A tapped slot leaves the main dry mix; a tapped send stops feeding the kit's reverb/delay. Params `src1..16`, `src_rev`, `src_del`,
`through`. Skin: `tools/gen_trmpc_tap.py` (3.x only). Package/install: `bash tools/package_trmpc.sh <version> <dist> [device-ip]`. Offline test:
"taps" in `test/audio_test.cpp`. Release: `release-trmpc.yml` (second job `tr_mpc_tap_fx`, its own tag and zip), pinned to mpc-vst-plugins `e15ce12`.

Remaining:
1. Audition Tap FX routing on the Force (slot to a track, sends to a return track, THRU); check alignment with `/tmp/trmpc-stats-on` ->
   `/tmp/trmpc-tap-stats.<pid>`.
2. Catalog entries for TR-MPC and Tap FX (separate PR in mpc-vst-plugins).
3. Baked per-voice titles (exact drum-port lettering instead of live text): about 800 art pieces; only if the skin has headroom (it is ~20 MB).
4. Optional: kit presets; check pan/sends on two slots sharing one voice behave as documented; MPC OS 2.x skin for TR-MPC.
5. Any change to the parameter list must append (project recall), and a new release needs a new version in `release-trmpc.yml`.

## Open items carried over
- Drum-pad patch (`patch/`) is MPC OS 3.9.1.2 only.
- Pad-to-voice map not verified against every MPC drum-program layout.
- CW-78 shows as GitHub "Latest" release in this repo (cosmetic).
