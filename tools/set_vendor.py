#!/usr/bin/env python3
"""Set the plugin manufacturer ("vendor" in vst.json: the Synths folder name and the plugin list's manufacturer).

The four Schwung kit ports keep their original author; TR-Kit, which is new work, is sd88me's.
"""
import json
import os

ports = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "ports")
VENDOR = {"6w6": "athousanddetails", "8w8": "athousanddetails", "cw78": "athousanddetails", "9w9": "athousanddetails", "trkit": "sd88me"}
for kit, vendor in VENDOR.items():
    p = os.path.join(ports, kit, "vst.json")
    v = json.load(open(p))
    v["vendor"] = vendor
    json.dump(v, open(p, "w", newline="\n"), indent=2)
    print(kit, "->", vendor)
