#!/usr/bin/env python3
"""Write ports/trkit/params.json: 16 slots (src, level, tune, decay, drive, pan, rev, dly) plus the shared FX and master stage.

The voice lists must match the engines' own voice order (sd606_voice_t, sc808_voice_t, cr78_voice_t, er99_trigger_t);
they are the flat index that trkit.cpp's `sNN_src` takes. Append only: saved projects store these by VST index.
"""
import json
import os

KITS = [
    ("606", "bd sd lt ht ch oh cy cp"),
    ("808", "bd sd lt mt ht lc mc hc rs cl ma cp cb ch oh cy"),
    ("CR78", "bd sd rs hh cy ma cl hb lb lc cb tb gu mb"),
    ("909", "bd sd lt mt ht rs hc ohh chh rc cr"),
]
SRC = ["%s %s" % (k, v.upper()) for k, ids in KITS for v in ids.split()]
DEFAULT_FLAT = []
off = [0]
for _, ids in KITS:
    off.append(off[-1] + len(ids.split()))
for b, v in [(1, 0), (1, 1), (1, 2), (1, 4), (0, 4), (0, 5), (3, 6), (3, 5),
             (1, 11), (1, 12), (2, 10), (2, 6), (0, 6), (3, 9), (2, 9), (1, 7)]:
    DEFAULT_FLAT.append(off[b] + v)

params, sections = [], []
for s in range(1, 17):
    keys = []
    sk = "s%02d_" % s
    params.append({"key": sk + "src", "name": "Slot %d voice" % s, "options": SRC, "default": DEFAULT_FLAT[s - 1]})
    keys.append(sk + "src")
    for k, label, dflt in (("level", "Level", 64), ("tune", "Tune", 64), ("decay", "Decay", 64), ("drive", "Drive", 64),
                           ("pan", "Pan", 64), ("rev", "Rev", 0), ("dly", "Dly", 0)):
        params.append({"key": sk + k, "name": "S%d %s" % (s, label), "min": 0, "max": 127, "default": dflt})
        keys.append(sk + k)
    sections.append({"label": "Slot %d" % s, "keys": keys})
# the shared FX and master stage: 8W8's own keys (defaults are 8W8's), prefixed fx_
DLY_TIME = ["1/32", "1/16T", "1/16", "1/8T", "1/16.", "1/8", "1/4T", "1/8.", "1/4", "1/2T", "1/4.", "1/2", "1/2."]
MASTER_DIST = ["Off", "Diode", "Clip", "SAT", "BFZ", "PDIST", "Fold", "Crush"]
fx = []
for key, name, dflt, opts in (
        ("rev_decay", "Rev Decay", 73, None), ("rev_tone", "Rev Tone", 57, None), ("rev_hpf", "Rev HPF", 62, None),
        ("rev_level", "Rev Level", 85, None), ("dly_time", "Dly Time", 7, DLY_TIME), ("dly_fdbk", "Dly Fdbk", 52, None),
        ("dly_tone", "Dly Tone", 51, None), ("dly_hpf", "Dly HPF", 62, None), ("dly_level", "Dly Level", 85, None),
        ("master_dist", "Master Dist", 0, MASTER_DIST), ("master_drive", "Master Drive", 0, None),
        ("comp", "Comp", 0, None), ("volume", "Volume", 100, None)):
    p = {"key": "fx_" + key, "name": name, "default": dflt}
    if opts: p["options"] = opts
    else: p.update({"min": 0, "max": 127})
    params.append(p)
    fx.append("fx_" + key)
sections.append({"label": "FX and master", "keys": fx})

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "ports", "trkit", "params.json")
json.dump({"name": "TR-MPC", "params": params, "sections": sections}, open(out, "w", newline="\n"), indent=1)
print(len(params), "params,", len(SRC), "voices")
