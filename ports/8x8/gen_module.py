#!/usr/bin/env python3
"""8X8's module.mpc.json: 8W8's parameters and pages (ports/8w8/module.mpc.json) under 8X8's own id and name.
    python3 ports/8x8/gen_module.py"""
import json, os
HERE = os.path.dirname(os.path.abspath(__file__))
mod = json.load(open(os.path.join(HERE, "..", "8w8", "module.mpc.json"), encoding="utf-8"))
mod.update(id="8x8", name="8X8", abbrev="8X8", version="1.0.0", author="sd88me; voices: Lemus (lemus-dev); panel after 8W8 (athousanddetails)",
           description="TR-808 voicing board solved from its circuit (Lemus), with 8W8's panel, drive and effects")
# l808_board.h / lemus_ahead.h: the board renders this many 128-frame blocks ahead of the host, 2.9 ms each. Last in
# the list: params.lock binds saved projects by index.
mod["chain_params"].append({"key": "latency", "name": "Latency", "type": "enum", "default": 2,
                            "options": ["0 blocks (0 ms)", "1 block (+2.9 ms)", "2 blocks (+5.8 ms)", "3 blocks (+8.7 ms)"]})
mod["ui_hierarchy"]["levels"]["root"]["params"].append({"key": "latency", "label": "Latency"})
json.dump(mod, open(os.path.join(HERE, "module.mpc.json"), "w", encoding="utf-8"), indent=1)
print("module.mpc.json: %d chain_params" % len(mod["chain_params"]))
