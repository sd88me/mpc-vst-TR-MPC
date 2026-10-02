<!-- ccr-projects-attribution: {"github_login":"sd88me"} -->
_Requested by **Sam2**_

Before: the only drum-pad patch for MPC OS 3.9.1.2 lived in mpc-vst-machinedrum's release folder and matched exactly one plugin name ("Machinedrum Module"); other plugins stayed on the keyboard pad layout.

After: `tools/mpc_patch/` holds one shared, optional, standalone patch script. A table of plugin names (Machinedrum Module, 6W6, 8W8, CW-78, 9W9, TR-MPC) gets Akai's drum layout with 16 lit pads (pad n sends note n-1). It is not part of any plugin release or installer.

How: `matcher.S` is the name table, reached through the existing hook in the firmware; `mpc-drum-pad-patch.sh` is generated from it and has `status`, `install` and `uninstall`. It shows the warnings, refuses anything but the exact MPC OS 3.9.1.2 checksum, needs the user to type PATCH, saves the full original and the original bytes first, verifies the result by checksum and restores itself on a mismatch. It upgrades a device that has the earlier Machinedrum-only patch. Tested offline (matcher under qemu-user at its real address; the script in BusyBox 1.36 against copies of the binary, 21 cases) and on a Force (6W6, 8W8, CW-78, 9W9). Also adds a dated NOTES.md entry and LF line endings for these files.

Not tested: other firmware versions (the script refuses them), plugins other than the ones in the table.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
