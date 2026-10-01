#!/usr/bin/env python3
"""Draft layout.conf for a port: one tall panel per voice, controls stacked down the panel, as many panels per page as fit.

    make_layout.py            regenerate every port's ports/<kit>/layout.conf (overwrites hand edits)
    make_layout.py 6w6        one port

Needs a sibling checkout of mpc-vst-plugins (for the plugin area geometry in tools/studio.py). Panels come from the kit's
own page hierarchy (ports/<kit>/module.mpc.json, built by extract_module.py): one per voice, then the send effects, then a
MASTER panel with whatever no voice page lists. Palettes are the CSS variables at the top of each ports/<kit>/src/web_ui.html,
mapped onto the renderer's theme_<name> keys (tools/shadow_skin.py THEME_KEYS).
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

# plugin area, in the skin's own coordinates (the same grid studio.py's auto-layout uses)
PANEL_W = 158            # one slot column
X0 = 8
Y_TOP = 92               # top of the panels
PANEL_H = 616
PER_PAGE = 8
TITLE_H = 44             # frame title
SKIP = {"ui_focus", "mutes", "note_map"}   # engine-internal, not for the panel

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


def label(s, n=12):
    t = "".join(c if c.isalnum() or c in " .-/%+:#" else " " for c in s.upper())
    return " ".join(t.split())[:n]


def panels(mod):
    cp = {p["key"]: p for p in mod["chain_params"]}
    levels = mod["ui_hierarchy"]["levels"]
    cols, used = [], set()
    for name, lv in levels.items():
        if name == "root":
            continue
        keys = [p["key"] for p in lv.get("params", []) if isinstance(p, dict) and p.get("key") in cp and p["key"] not in SKIP]
        if keys:
            cols.append((label(lv.get("name") or name, 14), keys))
            used.update(keys)
    rest = [k for k in cp if k not in used and k not in SKIP and not k.endswith("__open")]
    if rest:
        cols.append(("MASTER", rest))
    return cp, cols


def control(p, cx, cy):
    name = label(p.get("name") or p["key"], 12)
    # drop the voice prefix the chain_param name carries ("BD TUNE" -> "TUNE")
    name = label(p["label"], 12) if p.get("label") else name
    if p.get("type") == "enum":
        # the popup draws its own label above the field, so sit it a little lower than a slider's centre
        return 'popup cx=%d cy=%d w=%d h=40 label="%s" key=%s' % (cx, cy + 12, PANEL_W - 24, name, p["key"])
    return 'slider_h cx=%d cy=%d w=%d h=36 label="%s" key=%s' % (cx, cy, PANEL_W - 24, name, p["key"])


def build(kit):
    port = os.path.join(ROOT, "ports", kit)
    mod = json.load(open(os.path.join(port, "module.mpc.json")))
    cp, cols = panels(mod)
    levels = mod["ui_hierarchy"]["levels"]
    labels = {}
    for lv in levels.values():
        for p in lv.get("params", []):
            if isinstance(p, dict) and "key" in p:
                labels[p["key"]] = p.get("label")
    head = ["# Draft skin. Palette from the Schwung web UI (src/web_ui.html); one tall panel per voice, controls stacked. "
            "Edit in Skin Studio."]
    head += ["theme_%s=%s" % tuple(kv.split("=")) for kv in THEMES[kit].split()]
    npages = (len(cols) + PER_PAGE - 1) // PER_PAGE
    per = (len(cols) + npages - 1) // npages            # balance the pages instead of 8 + 2
    body = []
    for pg in range(npages):
        group = cols[pg * per:(pg + 1) * per]
        title = " ".join(t for t, _ in group[:1]) + (" - " + group[-1][0] if len(group) > 1 else "")
        body += ["", "[tab %s]" % title[:28]]
        qsets = []
        pitch = (PANEL_H - TITLE_H - 16) // max(max(len(k) for _, k in group), 6)   # one row height across the page
        for i, (ptitle, keys) in enumerate(group):
            fx = X0 + i * PANEL_W
            body.append('frame x=%d y=%d w=%d h=%d title="%s"' % (fx + 2, Y_TOP, PANEL_W - 6, PANEL_H, ptitle))
            for j, k in enumerate(keys):
                p = dict(cp[k]); p["label"] = labels.get(k)
                body.append(control(p, fx + PANEL_W // 2 - 1, Y_TOP + TITLE_H + 24 + j * pitch + pitch // 2))
            qsets.append((ptitle, keys[:16]))
        for ptitle, keys in qsets:
            body.append('qlinks "%s" = %s' % (ptitle, ",".join(keys)))
    out = os.path.join(port, "layout.conf")
    open(out, "w", newline="\n").write("\n".join(head + body) + "\n")
    vp = os.path.join(port, "vst.json")
    v = json.load(open(vp))
    if v.get("layout") != "layout.conf":
        v["layout"] = "layout.conf"
        json.dump(v, open(vp, "w", newline="\n"), indent=2)
    print(kit, "panels:", len(cols), "pages:", npages, "max controls:", max(len(k) for _, k in cols))


for k in (sys.argv[1:] or THEMES):
    build(k)
