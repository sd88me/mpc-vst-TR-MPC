# Morning review notes

State: all four ports build for armhf (glibc <= device 2.39), pass tools/test_port.sh (ASan/UBSan), and have a draft skin. Nothing was deployed to the Force and nothing was heard.

| Kit | uid | params (VST) | tabs | .so |
|---|---|---|---|---|
| 6W6 | T6W6 | 88 | 6 | tr6w6.so |
| 8W8 | T8W8 | 150 | 10 | tr8w8.so |
| CW-78 | TC78 | 138 | 9 | trcw78.so |
| 9W9 | T9W9 | 110 | 8 | tr9w9.so |

Open items
- 9W9 samples: now packaged by the release workflow (extra ports/9w9/src/samples:engine/samples). Licence checked: ER-99 samples are GPL-3.0 per upstream README.
- Skins are studio.py auto-layouts with a palette from each web_ui.html. Knob caps, plates and the sequencer/pad strip of the web UIs are not reproduced (CW-78's knobs are black on the hardware, the preview shows cream).
- Params are keyed per voice but pad-to-voice MIDI notes follow each kit's own map; not checked against the MPC pad layout.
- Not measured: CPU on the Force, audio output through the wrapper.
- Rebuild: tools/make_layout.py, then mpc-vst-plugins tools/build_port.sh from a path without spaces (WSL /tmp copy, tools converted to LF).
