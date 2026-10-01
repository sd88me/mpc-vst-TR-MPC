#!/usr/bin/env python3
"""Draft layout.conf for a port: studio.py's auto-layout under a palette taken from the kit's Schwung web UI.

    make_layout.py            regenerate every port's ports/<kit>/layout.conf (overwrites hand edits)
    make_layout.py 6w6        one port

Needs a sibling checkout of mpc-vst-plugins. The palettes are the CSS variables at the top of each
ports/<kit>/src/web_ui.html, mapped onto the renderer's theme_<name> keys (tools/shadow_skin.py THEME_KEYS).
"""
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
STUDIO = os.path.join(ROOT, "..", "mpc-vst-plugins", "tools", "studio.py")

THEMES = {
    "6w6": "bg=d6d6d2 ink=111111 ink_dim=5a5a56 accent=e0261c accent_hi=ff5a4a seg_active=111111 seg_inactive=ebebe8 "
           "seg_active_tx=ffffff lcd=161616 line=8a8a86 btn_bg=cfcfca btn_text=111111 box=c4c4bf display_ink=e0261c",
    "8w8": "bg=4a4946 ink=f0eee8 ink_dim=9a978f accent=e8763a accent_hi=f09a4e seg_active=e8763a seg_inactive=5b5a56 "
           "seg_active_tx=1d1f24 lcd=1d1f24 line=9a978f btn_bg=5b5a56 btn_text=f0eee8 box=1d1f24 display_ink=e8763a",
    "cw78": "bg=202124 ink=e9e7e1 ink_dim=6e6c66 accent=e07a2f accent_hi=f09a4e seg_active=e07a2f seg_inactive=2a2b2e "
            "seg_active_tx=17181a lcd=17181a line=6e6c66 btn_bg=d9d4c5 btn_text=17181a box=17181a display_ink=e07a2f",
    "9w9": "bg=e8e0d0 ink=1a1a1a ink_dim=7a7466 accent=f7941d accent_hi=ffb04d seg_active=2b2b2b seg_inactive=efe9dc "
           "seg_active_tx=f7941d lcd=1a1a1a line=b8b0a0 btn_bg=2b2b2b btn_text=e8e0d0 box=efe9dc display_ink=f7941d",
}


def build(kit):
    port = os.path.join(ROOT, "ports", kit)
    out = os.path.join(port, "layout.conf")
    subprocess.run([sys.executable, STUDIO, "auto", os.path.join(port, "module.mpc.json"), "-o", out],
                   check=True, stdout=subprocess.DEVNULL)
    body = [l for l in open(out).read().splitlines() if not l.startswith("# first-pass")]
    head = ["# Draft skin. Palette from the Schwung web UI (src/web_ui.html); layout from studio.py auto. "
            "Edit in Skin Studio."]
    head += ["theme_%s=%s" % tuple(kv.split("=")) for kv in THEMES[kit].split()]
    open(out, "w", newline="\n").write("\n".join(head + body) + "\n")
    vp = os.path.join(port, "vst.json")
    v = json.load(open(vp))
    if v.get("layout") != "layout.conf":
        v["layout"] = "layout.conf"
        json.dump(v, open(vp, "w", newline="\n"), indent=2)
    print(kit, "tabs:", sum(1 for l in body if l.startswith("[tab")))


for k in (sys.argv[1:] or THEMES):
    build(k)
