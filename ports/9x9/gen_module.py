#!/usr/bin/env python3
"""9X9's module.mpc.json: 9W9's parameters and pages (ports/9w9/module.mpc.json) plus the Latency control, appended last
(params.lock binds saved projects by index), under 9X9's own id and name.
    python3 ports/9x9/gen_module.py"""
import json, os
HERE = os.path.dirname(os.path.abspath(__file__))
mod = json.load(open(os.path.join(HERE, "..", "9w9", "module.mpc.json"), encoding="utf-8"))
mod.update(id="9x9", name="9X9", abbrev="9X9", version="1.0.0", author="sd88me; voices: Lemus (lemus-dev); panel after 9W9 (athousanddetails)",
           description="TR-909 voicing board solved from its circuit (Lemus), with 9W9's panel, drive and effects")
# l909_board.h: the board renders this many 128-frame blocks ahead of the host, 2.9 ms each (1: a busy beat plays clean on the
# Live III; 2: never missed a block)
mod["chain_params"].append({"key": "latency", "name": "Latency", "type": "enum", "default": 2,
                            "options": ["0 blocks (0 ms)", "1 block (+2.9 ms)", "2 blocks (+5.8 ms)", "3 blocks (+8.7 ms)"]})
root = mod["ui_hierarchy"]["levels"]["root"]
root["params"].append({"key": "latency", "label": "Latency"})
json.dump(mod, open(os.path.join(HERE, "module.mpc.json"), "w", encoding="utf-8"), indent=1)
print("module.mpc.json: %d chain_params" % len(mod["chain_params"]))
