# MPC OS drum-pad patch v2

Extends mpc-vst-machinedrum's `release/mpc_patch` (MPC OS 3.9.1.2 only): the plugin-name check is a table in `matcher.S`, so 6W6, 8W8, CW-78, 9W9, TR-Kit and Machinedrum Module all get the 16-pad drum layout. `helper.S`, `colours*.S` are that repo's sources, unchanged.

- `asm.sh` assembles the sources (arm32v7 gcc container); `make_patch.py <stock MPC>` writes `mpc-3.9.1.2.patch` (our bytes only) and checks the branch chain.
- `test_matcher.sh` runs the matcher under qemu-user at its real address with the stock targets stubbed: 18 name cases (6 matches, 12 fall-throughs to the original DrumSynth:Multi check, r4 preserved).
- Install: use install.sh from mpc-vst-machinedrum next to this patch file. A device with the old patch must run `install.sh uninstall` with the OLD patch file first.
- Patched md5 `730c959f317ea405c472f273342bc235` (stock `592eebc8e1ce0797dc8c98e7002143b8`).
