# Handoff: next phase (TR-MPC)

State as of 2026-10-02. Read README.md first; this is what to do next and what is known.

## Where things stand
- 6W6, 8W8, CW-78, 9W9 v1.0.0: released (public), installed and smoke-tested on a Force (192.168.1.44, MPC OS 3.9.1.2, root SSH), benched, and in the mpc-vst-plugins catalog via PR #94 (`catalog-tr-drums`; check it merged and the catalog rebuilt). Per-port workflows `.github/workflows/release-<kit>.yml` pin mpc-vst-plugins `28f5ffd`.
- Bench (Force, 16 voices, p99): 6W6 30.9% WARN, 8W8 13.1%, CW-78 7.7%, 9W9 21.2% WARN. Each port's `bench.txt` is the committed `-j` line.
- TR-Kit (`ports/trkit`, `docs/TR-KIT.md`) is a prototype: builds, wrapper and audio offline tests pass, never installed on the Force, no skin, CPU unmeasured.
- DSP in `ports/<kit>/src` is vendored unmodified from athousanddetails (see each VENDORED.md). Keep it that way; MPC-specific code goes in `mpc_pads_engine.c`, `ports/trkit`, `layout.conf`, tools.

## Goal of the next phase
Ship **TR-MPC** (the modular 16-slot kit; any of the 49 voices per slot) as one plugin with a usable skin, measured CPU, and a release, after which the README's "coming soon" section becomes real documentation. Name decision to confirm: plugin is currently `TR-Kit` (uid `TRKt`, vendor sd88me); README calls it TR-MPC. Pick one before the first release, since uid/name end up in saved projects.

## Work items, in order
1. **Decide name and UID.** Rename in `ports/trkit/vst.json`, `tools/*`, patch matcher table (`patch/matcher.S` matches plugin names for the drum-pad layout), README, docs.
2. **Install and listen first.** Build (`bash tools/build_ci.sh trkit`), install by hand on the Force, check all 16 slots trigger the right voices from MPC pads (notes 36-51, and 0-15 with the drum-pad patch), project save/reload keeps `sNN_src` and the other params.
3. **Bench.** `bash ../mpc-vst-plugins/tools/bench.sh ports/trkit/build/trkit.so 192.168.1.44 -j`. Worst case is 16 slots spread over four engines at once; 6W6 and 9W9 are already WARN alone. If it FAILs: render only engines/voices in use, lower per-engine cost, or cap polyphony. 9W9 needs `engine/samples/` next to the .so (copy to /tmp/engine/samples for the bench).
4. **Fix known limits** (docs/TR-KIT.md): two slots on the same voice share it; FX and master stage are per kit, not per slot (no per-slot pan); only six pots per slot are VST params, so other voice pots (kick attack, tone...) cannot be edited or saved. Decide which matter for v1: probably expose more pots per slot, or accept and document. Params are append-only (saved projects store VST index), so settle the list before releasing.
5. **Skin.** No `layout.conf` yet. 49-option popups for `sNN_src` are untried in the auto-layout; likely a 16-slot grid page plus per-slot pages, kit FX and volume page. Use Skin Studio from mpc-vst-plugins (`docs/SKIN_STUDIO.md`, `tools/studio.py`), then `tools/make_layout.py` conventions. Also consider kit presets.
6. **Release.** Copy a `release-*.yml`, set `vst_dir: ports/trkit`, plugin_id, `extra` for 9W9 samples (`ports/9w9/src/samples:engine/samples`), license GPL-3.0-only. Run dry_run, install the draft zip, smoke-test, publish, add a catalog entry (new PR in mpc-vst-plugins, own `asset_pattern`, as for the four ports). Commit `bench.txt`.
7. **Docs.** Replace the README "Coming soon" section; update REVIEW.md.

## Open items carried over
- Skins of the four ports are auto-layouts: no knob caps/plates, CW-78 knobs are black on hardware but cream in the preview. Optional polish pass.
- Pad-to-voice map not verified against every MPC drum-program layout.
- Drum-pad patch (`patch/`) is MPC OS 3.9.1.2 only; needs the install.sh from mpc-vst-machinedrum.
- CW-78 shows as GitHub "Latest" release in this repo (cosmetic).

## How to work here
- Build and test: `bash tools/build_ci.sh <kit>` / `bash tools/test_ci.sh <kit>` (Docker, sibling `../mpc-vst-plugins`, path without spaces). `tools/build_all.sh` also renders previews.
- Device: `ssh root@192.168.1.44`; MPC.settings backup `MPC.settings.pre-tr` is in `/media/az01-internal/Settings/MPC/`. Each plugin's `uninstall.sh` removes it. The four ports are currently installed on that Force.
- Skills in this setup: `mpc-vst-plugin` (build, skin, register, test) and `force-device-workflow` (deploy and restart pitfalls).
- Commits end with the Co-Authored-By / Claude-Session lines; releases come from the draft workflow, never by hand.
