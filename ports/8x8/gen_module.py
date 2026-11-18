#!/usr/bin/env python3
"""8X8's module.mpc.json: 8W8's parameters and pages (ports/8w8/module.mpc.json) under 8X8's own id and name.
    python3 ports/8x8/gen_module.py"""
import json, os
HERE = os.path.dirname(os.path.abspath(__file__))
mod = json.load(open(os.path.join(HERE, "..", "8w8", "module.mpc.json"), encoding="utf-8"))
mod.update(id="8x8", name="8X8", abbrev="8X8", version="1.0.0", author="sd88me; voices: Lemus (lemus-dev); panel after 8W8 (athousanddetails)",
           description="TR-808 voicing board solved from its circuit (Lemus), with 8W8's panel, drive and effects")
json.dump(mod, open(os.path.join(HERE, "module.mpc.json"), "w", encoding="utf-8"), indent=1)
print("module.mpc.json: %d chain_params" % len(mod["chain_params"]))
