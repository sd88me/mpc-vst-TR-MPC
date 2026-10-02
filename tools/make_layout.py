#!/usr/bin/env python3
"""Draft layout.conf for a port: a header with the plugin name, then tall voice panels, as many per page as fit.

    make_layout.py            regenerate every port's ports/<kit>/layout.conf (overwrites hand edits)
    make_layout.py 6w6        one port

Each voice panel is 2 knob columns x up to 4 rows (8 controls; an enum such as Distortion takes one cell as a popup).
On the device the first draft used one column of 8 horizontal sliders per voice; MPC draws each slider's name and value
around it, so 8 of them in 570 px overlapped and clipped. A knob row needs about 130 px (arc, name, value box), so 4 rows
fit under a 44 px header, 4 panels (4 x 300 px) per page.

Panels come from the kit's page hierarchy (ports/<kit>/module.mpc.json, built by extract_module.py): one per voice, then
the send effects, then a MASTER panel with whatever no voice page lists. Palettes are the CSS variables at the top of each
ports/<kit>/src/web_ui.html, mapped onto the renderer's theme_<name> keys (tools/shadow_skin.py THEME_KEYS).
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

# plugin area, in the skin's own coordinates (the grid studio.py's auto-layout uses): x 0-1280, y 92-708
X0 = 8
Y_TOP = 92
HEADER_H = 44
PANEL_W = 300            # two 150 px knob columns
PER_PAGE = 4
PANEL_H = 708 - Y_TOP - HEADER_H
TITLE_H = 44             # frame title inside the panel
ROWS = 4
KNOB_R = 26
SKIP = {"ui_focus", "mutes", "note_map"}   # engine-internal, not for the panel
FX_LEVELS = {"rev", "dly", "fxrev", "fxdly", "rhy"}   # page-hierarchy levels that are not a drum voice

THEMES = {
    "6w6": "bg=d6d6d2 ink=111111 ink_dim=5a5a56 accent=e0261c accent_hi=ff5a4a seg_active=111111 seg_inactive=ebebe8 "
           "seg_active_tx=ffffff lcd=161616 line=8a8a86 btn_bg=cfcfca btn_text=111111 box=c4c4bf display_ink=e0261c knob_face=1c1c1c knob_ring=8a8a86 knob_dot=e0261c",
    "8w8": "bg=4a4946 ink=f0eee8 ink_dim=9a978f accent=e8763a accent_hi=f09a4e seg_active=e8763a seg_inactive=5b5a56 "
           "seg_active_tx=1d1f24 lcd=1d1f24 line=9a978f btn_bg=5b5a56 btn_text=f0eee8 box=1d1f24 display_ink=e8763a knob_face=e2762c knob_ring=2a2a2a knob_dot=fffdf8",
    "cw78": "bg=202124 ink=e9e7e1 ink_dim=6e6c66 accent=e07a2f accent_hi=f09a4e seg_active=e07a2f seg_inactive=2a2b2e "
            "seg_active_tx=17181a lcd=17181a line=6e6c66 btn_bg=d9d4c5 btn_text=17181a box=17181a display_ink=e07a2f knob_face=191a1c knob_ring=6e6c66 knob_dot=e07a2f",
    "9w9": "bg=e8e0d0 ink=1a1a1a ink_dim=7a7466 accent=f7941d accent_hi=ffb04d seg_active=2b2b2b seg_inactive=efe9dc "
           "seg_active_tx=f7941d lcd=1a1a1a line=b8b0a0 btn_bg=2b2b2b btn_text=e8e0d0 box=efe9dc display_ink=f7941d knob_face=2b2b2b knob_ring=b8b0a0 knob_dot=f7941d",
}
TITLES = {   # (big name, tagline)
    "6w6": ("6W6", "DRUMATIX - TR-606 STYLE DRUM MACHINE"),
    "8w8": ("8W8", "TR-808 STYLE DRUM MACHINE - 16 VOICES"),
    "cw78": ("CW-78", "COMPURHYTHM - CR-78 STYLE DRUM MACHINE"),
    "9w9": ("9W9", "TR-909 STYLE DRUM MACHINE"),
}


def label(s, n=12):
    t = "".join(c if c.isalnum() or c in " .-/%+:#" else " " for c in s.upper())
    return " ".join(t.split())[:n]


def panels(mod):
    cp = {p["key"]: p for p in mod["chain_params"]}
    levels = mod["ui_hierarchy"]["levels"]
    cols, used, labels = [], set(), {}
    for name, lv in levels.items():
        if name == "root":
            continue
        keys = []
        for p in lv.get("params", []):
            if isinstance(p, dict) and p.get("key") in cp and p["key"] not in SKIP:
                keys.append(p["key"])
                labels[p["key"]] = p.get("label")
        if keys:
            fx = name in FX_LEVELS
            abbr = {"rev": "REV", "fxrev": "REV", "dly": "DLY", "fxdly": "DLY", "rhy": "RHY"}.get(name) or keys[0].split("_")[0].upper()
            cols.append((label(lv.get("name") or name, 14), keys, not fx, abbr))
            used.update(keys)
    rest = [k for k in cp if k not in used and k not in SKIP and not k.endswith("__open")]
    if rest:
        cols.append(("MASTER", rest, False, "MASTER"))
    return cp, cols, labels


def cells(keys, voice):
    """key -> cell 0-7 (row = cell // 2, column = cell % 2). In a voice panel Drive, Distortion, Level, Rev and Dly each
    keep one cell on every panel (3-7), and the voice's own controls fill 0-2 and then whatever cells are left."""
    if not voice:
        return {k: i for i, k in enumerate(keys[:ROWS * 2])}
    def tail(k):
        if k.endswith("_drive"):
            return 3
        if k.endswith("_dist_type") or k.endswith("_dist"):
            return 4
        if k.endswith("_level") or k.endswith("_volume"):
            return 5
        if k.endswith("_rev"):
            return 6
        if k.endswith("_dly"):
            return 7
        return None
    pos = {k: tail(k) for k in keys if tail(k) is not None}
    taken = set(pos.values())
    free = [c for c in range(ROWS * 2) if c not in taken]
    for k in keys:
        if k not in pos and free:
            pos[k] = free.pop(0)
    return pos


def build(kit):
    port = os.path.join(ROOT, "ports", kit)
    mod = json.load(open(os.path.join(port, "module.mpc.json")))
    cp, cols, labels = panels(mod)
    head = ["# Draft skin. Palette from the Schwung web UI (src/web_ui.html); header, then 2-column knob panels. "
            "Edit in Skin Studio."]
    head += ["theme_%s=%s" % tuple(kv.split("=")) for kv in THEMES[kit].split()]
    npages = (len(cols) + PER_PAGE - 1) // PER_PAGE
    per = (len(cols) + npages - 1) // npages            # balance the pages instead of 4 + 4 + 1
    name, tagline = TITLES[kit]
    th = dict(kv.split('=') for kv in THEMES[kit].split())
    acc, dim = th['accent'], th['ink_dim']
    body = []
    pitch = (PANEL_H - TITLE_H - 6) // ROWS
    for pg in range(npages):
        group = cols[pg * per:(pg + 1) * per]
        rng = group[0][3] + (" - " + group[-1][3] if len(group) > 1 else "")
        body += ["", "[tab %s]" % rng]
        hy = Y_TOP + HEADER_H // 2 + 2
        body.append('text cx=24 cy=%d label="%s" size=3 weight=700 spacing=3 color=%s align=left' % (hy, name, acc))
        body.append('text cx=%d cy=%d label="%s" size=1.3 color=%s align=left spacing=2' % (24 + 40 * len(name) + 30, hy + 4, tagline, dim))
        body.append('text cx=1256 cy=%d label="PAGE %d/%d" size=1.2 color=%s align=right spacing=2' % (hy + 4, pg + 1, npages, dim))
        qsets = []
        top = Y_TOP + HEADER_H
        overview = []
        for i, (ptitle, keys, voice, abbr) in enumerate(group):
            fx = X0 + i * PANEL_W
            body.append('frame x=%d y=%d w=%d h=%d title="%s"' % (fx + 2, top, PANEL_W - 6, PANEL_H, ptitle))
            pos = cells(keys, voice)
            for k in keys:
                if k not in pos:
                    continue
                row, col = divmod(pos[k], 2)
                cx = fx + 75 + col * 148
                ry = top + TITLE_H + row * pitch
                nm = label(labels.get(k) or cp[k].get("name") or k, 12)
                if cp[k].get("type") == "enum":
                    body.append('popup cx=%d cy=%d w=130 h=48 label="%s" key=%s' % (cx, ry + 66, nm, k))
                else:
                    body.append('knob cx=%d cy=%d r=%d label="%s" key=%s' % (cx, ry + 38, KNOB_R, nm, k))
            qsets.append((ptitle, keys[:16]))
            # the page's first Q-Link set (it names the tab): this panel's first two controls, drive and level
            pick = [k for k in keys[:2]] + [k for k in keys if k.endswith(("_drive", "_level", "_volume"))][:2]
            overview += list(dict.fromkeys(pick))[:4]
        body.append('qlinks "%s" = %s' % (rng, ",".join(overview[:16])))
        for ptitle, keys in qsets:
            body.append('qlinks "%s" = %s' % (ptitle, ",".join(keys)))
    out = os.path.join(port, "layout.conf")
    open(out, "w", newline="\n").write("\n".join(head + body) + "\n")
    vp = os.path.join(port, "vst.json")
    v = json.load(open(vp))
    if v.get("layout") != "layout.conf":
        v["layout"] = "layout.conf"
        json.dump(v, open(vp, "w", newline="\n"), indent=2)
    over = [c[0] for c in cols if len(c[1]) > ROWS * 2]
    print(kit, "panels:", len(cols), "pages:", npages, "max controls:", max(len(c[1]) for c in cols), "over 8:", over)


for k in (sys.argv[1:] or THEMES):
    build(k)
