# CW-78 for MPC / Force

MPC OS VST plugin port of [CW-78](https://github.com/athousanddetails/schwung-cw-78) by athousanddetails (CompuRhythm CR-78 style drum machine). Manufacturer: athousanddetails. GPL-3.0, see `LICENSE` and `THIRD_PARTY.md` (if present) and `VENDORED.md`. The DSP is the upstream's, unmodified; this folder adds the MPC wrapper settings (`vst.json`), the skin (`layout.conf`) and a MIDI pad-note remap (`src/mpc_pads_engine.c`).

- **Voices:** 14 voices (pads 1-14). Pad n plays voice n in the drum layout (MIDI notes 0-15 from the drum-pad layout, or 36 and up from the keyboard layout).
- **Skin:** a header with the plugin name, then voice panels (two columns of knobs per voice), four panels per page.

## Optional: 16-pad drum layout (firmware patch)

Out of the box MPC shows this plugin with the keyboard (melodic) pad layout. There is an **optional, advanced** patch for MPC OS 3.9.1.2 that gives it the 16-pad drum layout. It modifies Akai's factory `/usr/bin/MPC`, is not part of the normal plugin install or release, and is a separate standalone script with warnings, backups and an uninstall: see [patch/](../../patch/README.md). Read the warnings there before using it.
