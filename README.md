# mpc-vst-TR-MPC

MPC/Force VST ports of the athousanddetails Schwung drum machines (6W6, 8W8, CW-78, 9W9), built with the wrapper and Skin Studio in [mpc-vst-plugins](https://github.com/sd88me/mpc-vst-plugins) via its Schwung adapter. License: GPL-3.0.

Plan: individual plugins first (6W6 -> 8W8 -> CW-78 -> 9W9), then a 16-voice any-module kit once CPU cost is measured on the Force.

Layout: `ports/<kit>/` holds each vendored upstream module; a `vst.json` and `layout.conf` per port come next. Build tools are not vendored; use a sibling checkout of mpc-vst-plugins.

## Offline test
tools/test_port.sh needs a path without spaces and a sibling mpc-vst-plugins checkout:
`bash ../mpc-vst-plugins/tools/test_port.sh ports/6w6/vst.json` (6W6 needs C++14, set in vst.json cflags).
`tools/extract_module.py` builds `module.mpc.json` (chain_params + ui_hierarchy) from a kit's generated `*_params.h`.

## Manufacturer names
The four ports (6W6, 8W8, CW-78, 9W9) keep their original author, athousanddetails, as manufacturer and Synths folder name (thousanddetails - VST - <name>); TR-Kit is sd88me's. 	ools/set_vendor.py sets this in each vst.json.

## Releases
Each port releases separately: Actions -> "Release <kit> (draft)" (`.github/workflows/release-<kit>.yml`, tags `<kit>-vst-v<version>`) uses mpc-vst-plugins' shared workflow, pinned to a commit. 9W9's zip carries `engine/samples/*.wav` next to the .so. Local build: `bash tools/build_ci.sh <kit>`, test: `bash tools/test_ci.sh <kit>`. 9W9 samples are ER-99 (GPL-3.0, same as the port).
