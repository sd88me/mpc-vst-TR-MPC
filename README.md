# mpc-vst-TR-MPC

MPC/Force VST ports of the athousanddetails Schwung drum machines (6W6, 8W8, CW-78, 9W9), built with the wrapper and Skin Studio in [mpc-vst-plugins](https://github.com/sd88me/mpc-vst-plugins) via its Schwung adapter. License: GPL-3.0.

Plan: individual plugins first (6W6 -> 8W8 -> CW-78 -> 9W9), then a 16-voice any-module kit once CPU cost is measured on the Force.

Layout: `ports/<kit>/` holds each vendored upstream module; a `vst.json` and `layout.conf` per port come next. Build tools are not vendored; use a sibling checkout of mpc-vst-plugins.
