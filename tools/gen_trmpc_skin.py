#!/usr/bin/env python3
"""Generate the TR-MPC skin variants: ports/trkit/layouts/<style>.conf, the rack artwork in ports/trkit/images/ and
ports/trkit/skin.css. `bash tools/build_ci.sh trkit [style]` copies the chosen layout to ports/trkit/layout.conf.

Styles:
  restyle   the original 16 slot panels (voice popup + 7 knobs each), drawn as a modular-synth rack: faceplates with
            screws, rack rails, a jack and a pad label per module, blank panels filling the FX page.

Coordinates: the layout is in "shadow" coordinates, the artwork (art file=) in plugin-area coordinates (the layout's y
minus Y_OFF = 86).
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.join(HERE, "..", "ports", "trkit")
Y_OFF = 86
W, H = 1280, 628
EAR, MOD_W, GAP = 24, 304, 5
MOD_X = [EAR + i * (MOD_W + GAP) for i in range(4)]   # 24, 333, 642, 951
TOP, BOT = 136, 708                                      # module top and bottom, layout y
NOTE_NAMES = "C C# D D# E F F# G G# A A# B".split()
GROUPS = "6W6 TR-606:8:5b9bd5,8W8 TR-808:16:e8763a,CW-78 CR-78:14:d4c28a,9W9 TR-909:11:d9534f"

THEME = """theme_bg=17181b
theme_ink=e8e9ec
theme_ink_dim=9a9ea8
theme_accent=5ec2b7
theme_accent_hi=8fe0d6
theme_seg_active=5ec2b7
theme_seg_inactive=2b2d31
theme_seg_active_tx=14161a
theme_lcd=101214
theme_line=585b64
theme_btn_bg=2b2d31
theme_btn_text=e8e9ec
theme_box=101214
theme_display_ink=5ec2b7
theme_knob_face=34363c
theme_knob_ring=585b64
theme_knob_dot=5ec2b7
art_css=skin.css
knob_look=cap"""


def pad_name(slot):   # slot 1..16 -> MPC pad note C1.. (note 36 is C1 on MPC)
    n = 35 + slot
    return "%s%d" % (NOTE_NAMES[n % 12], n // 12 - 2)


def screw(cx, cy, seed):
    ang = (seed * 37) % 180
    return ('<g transform="translate(%g %g)"><circle r="7.5" fill="url(#screw)" stroke="#15161a" stroke-width="1"/>'
            '<circle r="7.5" fill="none" stroke="#ffffff" stroke-opacity="0.18" stroke-width="0.8" transform="translate(-0.4 -0.4)"/>'
            '<g transform="rotate(%d)"><rect x="-5.2" y="-0.9" width="10.4" height="1.8" rx="0.6" fill="#17181b"/>'
            '<rect x="-0.9" y="-5.2" width="1.8" height="10.4" rx="0.6" fill="#17181b"/></g></g>') % (cx, cy, ang)


def jack(cx, cy):
    return ('<g transform="translate(%g %g)"><circle r="12" fill="#0c0d0f" stroke="#6c707a" stroke-width="2.4"/>'
            '<circle r="8" fill="url(#hole)" stroke="#2b2d33" stroke-width="1.5"/><circle r="3.4" fill="#050506"/></g>') % (cx, cy)


def rails():
    out = []
    for x in (0, W - EAR):
        out.append('<rect x="%d" y="0" width="%d" height="%d" fill="url(#rail)"/>' % (x, EAR, H))
        out.append('<rect x="%d" y="0" width="1" height="%d" fill="#000" opacity="0.5"/>' % (x + (EAR - 1 if x == 0 else 0), H))
        for y in range(34, H, 78):
            out.append('<rect x="%g" y="%d" width="10" height="20" rx="5" fill="#0b0c0e" stroke="#44474f" stroke-width="1"/>' % (x + 7, y))
    return "".join(out)


def plate(x0, label, seed, blank=False, pad=None):
    y0, y1 = TOP - Y_OFF, BOT - Y_OFF
    h = y1 - y0
    o = ['<g>',
         '<rect x="%d" y="%d" width="%d" height="%d" rx="3" fill="url(#plate)" stroke="#07080a" stroke-width="1.5"/>' % (x0, y0, MOD_W, h),
         '<rect x="%g" y="%g" width="%d" height="%d" rx="2" fill="none" stroke="#ffffff" stroke-opacity="0.09" stroke-width="1"/>' % (x0 + 1.5, y0 + 1.5, MOD_W - 3, h - 3),
         # brushed-metal grain
         '<rect x="%d" y="%d" width="%d" height="%d" rx="3" fill="url(#grain)" opacity="0.5"/>' % (x0, y0, MOD_W, h)]
    for k, (sx, sy) in enumerate(((x0 + 14, y0 + 14), (x0 + MOD_W - 14, y0 + 14), (x0 + 14, y1 - 14), (x0 + MOD_W - 14, y1 - 14))):
        o.append(screw(sx, sy, seed * 4 + k))
    if blank:
        o.append('<text x="%g" y="%g" text-anchor="middle" font-family="Titillium Web" font-weight="700" font-size="30" '
                 'letter-spacing="6" fill="#5ec2b7" fill-opacity="0.55">TR-MPC</text>' % (x0 + MOD_W / 2, (y0 + y1) / 2))
        o.append('<text x="%g" y="%g" text-anchor="middle" font-family="Titillium Web" font-weight="600" font-size="13" '
                 'letter-spacing="3" fill="#9a9ea8" fill-opacity="0.6">%s</text>' % (x0 + MOD_W / 2, (y0 + y1) / 2 + 28, label))
    else:
        o.append('<rect x="%d" y="%d" width="%d" height="2" fill="#5ec2b7" fill-opacity="0.55"/>' % (x0 + 26, y0 + 40, MOD_W - 52))
        if pad is not None:
            o.append('<text x="%g" y="%g" font-family="Titillium Web" font-weight="600" font-size="13" letter-spacing="3" '
                     'fill="#9a9ea8">PAD %d  %s</text>' % (x0 + 30, y1 - 22, pad, pad_name(pad)))
            o.append('<text x="%g" y="%g" text-anchor="end" font-family="Titillium Web" font-weight="600" font-size="13" '
                     'letter-spacing="3" fill="#9a9ea8">OUT</text>' % (x0 + MOD_W - 66, y1 - 22))
            o.append(jack(x0 + MOD_W - 40, y1 - 26))
    o.append('</g>')
    return "".join(o)


DEFS = """<defs>
<linearGradient id="plate" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#34373e"/><stop offset="0.5" stop-color="#2a2d33"/><stop offset="1" stop-color="#232529"/></linearGradient>
<linearGradient id="rail" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#4a4d55"/><stop offset="0.5" stop-color="#6a6e78"/><stop offset="1" stop-color="#3d4047"/></linearGradient>
<radialGradient id="screw" cx="0.38" cy="0.35" r="0.8"><stop offset="0" stop-color="#d4d7de"/><stop offset="0.6" stop-color="#8d919b"/><stop offset="1" stop-color="#4f525a"/></radialGradient>
<radialGradient id="hole" cx="0.5" cy="0.5" r="0.5"><stop offset="0.5" stop-color="#151619"/><stop offset="1" stop-color="#3a3d44"/></radialGradient>
<pattern id="grain" width="6" height="6" patternUnits="userSpaceOnUse"><rect width="6" height="1" fill="#ffffff" fill-opacity="0.05"/><rect y="3" width="6" height="1" fill="#000000" fill-opacity="0.08"/></pattern>
<linearGradient id="bgg" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#0e0f11"/><stop offset="1" stop-color="#17181b"/></linearGradient>
</defs>"""


def page_svg(modules):
    """modules: list of 4 (kind, label, pad) for the 4 rack positions."""
    body = ['<rect width="%d" height="%d" fill="url(#bgg)"/>' % (W, H), rails()]
    for i, (kind, label, pad) in enumerate(modules):
        body.append(plate(MOD_X[i], label, i, blank=kind == "blank", pad=pad))
    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d">%s%s</svg>\n'
            % (W, H, W, H, DEFS, "".join(body)))


CSS = """/* TR-MPC rack skin: the faceplates are drawn in images/rack_*.svg; the frames only supply each module's title. */
.frame-border { fill: none; stroke: none; }
.frame-rule { stroke: #5ec2b7; stroke-opacity: 0.0; }
.frame-title { font-size: 21px; letter-spacing: 0.14em; fill: #e8e9ec; }
.text { fill: #9a9ea8; }
"""


def slot_module(lines, s, x0):
    """One slot panel: voice popup + 7 knobs, in the module at x0. Frame inset so the title clears the screws."""
    k = "s%02d_" % s
    fx = x0 + 26
    c1, c2 = x0 + 78, x0 + 226
    lines += [
        'frame x=%d y=%d w=%d h=%d title="SLOT %d"' % (fx, TOP, MOD_W - 52, BOT - TOP, s),
        'popup cx=%d cy=240 w=130 h=48 label="VOICE" key=%ssrc groups="%s"' % (c1, k, GROUPS),
        'knob cx=%d cy=212 r=26 label="LEVEL" key=%slevel' % (c2, k),
        'knob cx=%d cy=330 r=26 label="TUNE" key=%stune' % (c1, k),
        'knob cx=%d cy=330 r=26 label="DECAY" key=%sdecay' % (c2, k),
        'knob cx=%d cy=448 r=26 label="DRIVE" key=%sdrive' % (c1, k),
        'knob cx=%d cy=448 r=26 label="PAN" key=%span' % (c2, k),
        'knob cx=%d cy=566 r=26 label="REV" key=%srev' % (c1, k),
        'knob cx=%d cy=566 r=26 label="DLY" key=%sdly' % (c2, k),
    ]


FX_MODULES = [
    ("REVERB", [("knob", 78, 218, "DECAY", "fx_rev_decay"), ("knob", 226, 218, "TONE", "fx_rev_tone"),
                ("knob", 78, 348, "HPF", "fx_rev_hpf"), ("knob", 226, 348, "LEVEL", "fx_rev_level")]),
    ("DELAY", [("popup", 78, 246, "TIME", "fx_dly_time"), ("knob", 226, 218, "FDBK", "fx_dly_fdbk"),
               ("knob", 78, 348, "TONE", "fx_dly_tone"), ("knob", 226, 348, "HPF", "fx_dly_hpf"),
               ("knob", 78, 478, "LEVEL", "fx_dly_level")]),
    ("MASTER", [("popup", 78, 246, "MASTER DIST", "fx_master_dist"), ("knob", 226, 218, "DRIVE", "fx_master_drive"),
                ("knob", 78, 348, "COMP", "fx_comp"), ("knob", 226, 348, "VOLUME", "fx_volume")]),
]


def header(page, pages):
    return ['text cx=44 cy=116 label="TR-MPC" size=3 weight=700 spacing=3 color=5ec2b7 align=left',
            'text cx=314 cy=120 label="16-SLOT MODULAR DRUM RACK - ANY VOICE FROM 6W6 / 8W8 / CW-78 / 9W9" size=1.3 color=9a9ea8 align=left spacing=2',
            'text cx=1236 cy=120 label="PAGE %d/%d" size=1.2 color=9a9ea8 align=right spacing=2' % (page, pages)]


def build_restyle():
    images = os.path.join(PORT, "images")
    os.makedirs(images, exist_ok=True)
    out = ["# TR-MPC skin (restyle): the original slot panels drawn as a modular-synth rack. Generated by tools/gen_trmpc_skin.py.",
           THEME]
    slot_svg = {}
    for page in range(4):
        slots = list(range(page * 4 + 1, page * 4 + 5))
        name = "images/rack_slots_%d.svg" % (page + 1)
        open(os.path.join(PORT, name), "w", newline="\n").write(
            page_svg([("slot", "SLOT %d" % s, s) for s in slots]))
        out.append("")
        out.append("[tab S%d - S%d]" % (slots[0], slots[-1]))
        out.append("art file=%s fit=stretch" % name)
        out += header(page + 1, 5)
        for i, s in enumerate(slots):
            slot_module(out, s, MOD_X[i])
        out.append('qlinks "S%d - S%d" = %s' % (slots[0], slots[-1], ",".join(
            "s%02d_%s" % (s, k) for s in slots for k in ("src", "level", "tune", "decay"))))
        for s in slots:
            out.append('qlinks "SLOT %d" = %s' % (s, ",".join("s%02d_%s" % (s, k) for k in (
                "src", "level", "tune", "decay", "drive", "pan", "rev", "dly"))))
    name = "images/rack_fx.svg"
    open(os.path.join(PORT, name), "w", newline="\n").write(
        page_svg([("slot", t, None) for t, _ in FX_MODULES[:3]] + [("blank", "SHARED FX / MASTER", None)]))
    out += ["", "[tab FX - MASTER]", "art file=%s fit=stretch" % name] + header(5, 5)
    for i, (title, ctrls) in enumerate(FX_MODULES):
        x0 = MOD_X[i]
        out.append('frame x=%d y=%d w=%d h=%d title="%s"' % (x0 + 26, TOP, MOD_W - 52, BOT - TOP, title))
        for kind, dx, cy, label, key in ctrls:
            if kind == "popup":
                out.append('popup cx=%d cy=%d w=130 h=48 label="%s" key=%s' % (x0 + dx, cy, label, key))
            else:
                out.append('knob cx=%d cy=%d r=26 label="%s" key=%s' % (x0 + dx, cy, label, key))
    out += ['qlinks "FX - MASTER" = fx_rev_decay,fx_rev_tone,fx_rev_level,fx_dly_time,fx_dly_fdbk,fx_dly_level,fx_master_dist,fx_master_drive',
            'qlinks "REVERB" = fx_rev_decay,fx_rev_tone,fx_rev_hpf,fx_rev_level',
            'qlinks "DELAY" = fx_dly_time,fx_dly_fdbk,fx_dly_tone,fx_dly_hpf,fx_dly_level',
            'qlinks "MASTER" = fx_master_dist,fx_master_drive,fx_comp,fx_volume']
    return "\n".join(out) + "\n"


def main():
    os.makedirs(os.path.join(PORT, "layouts"), exist_ok=True)
    open(os.path.join(PORT, "layouts", "restyle.conf"), "w", newline="\n").write(build_restyle())
    open(os.path.join(PORT, "skin.css"), "w", newline="\n").write(CSS)
    print("wrote layouts/restyle.conf, skin.css, images/")


if __name__ == "__main__":
    main()
