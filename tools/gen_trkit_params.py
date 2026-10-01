#!/usr/bin/env python3
"""Write ports/trkit/params.json: 16 slots (src, level, tune, decay, drive, rev, dly) plus a volume per kit.

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
    for k, label in (("level", "Level"), ("tune", "Tune"), ("decay", "Decay"), ("drive", "Drive"), ("rev", "Rev"), ("dly", "Dly")):
        params.append({"key": sk + k, "name": "S%d %s" % (s, label), "min": 0, "max": 127, "default": 64})
        keys.append(sk + k)
    sections.append({"label": "Slot %d" % s, "keys": keys})
kv = []
for b, (name, _) in enumerate(KITS):
    params.append({"key": "k%d_volume" % b, "name": "%s Volume" % name, "min": 0, "max": 127, "default": 100})
    kv.append("k%d_volume" % b)
sections.append({"label": "Kit volumes", "keys": kv})

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "ports", "trkit", "params.json")
json.dump({"name": "TR-Kit", "params": params, "sections": sections}, open(out, "w", newline="\n"), indent=1)
print(len(params), "params,", len(SRC), "voices")
