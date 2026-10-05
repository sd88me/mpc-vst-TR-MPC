#!/usr/bin/env python3
"""Generate the TR-MPC skin variants: ports/trkit/layouts/<style>.conf, the rack artwork in ports/trkit/images/ and
ports/trkit/skin.css. `bash tools/build_ci.sh trkit [style]` copies the chosen layout to ports/trkit/layout.conf.

Styles:
  restyle   the original 16 slot panels (voice popup + 7 knobs each), drawn as a modular-synth rack: faceplates with
            screws, rack rails, a jack and a pad label per module, blank panels filling the FX page.
  editor    rack overview + one editor: two mixer pages of 8 compact modules (voice, level, pan, rev), an EDITOR page
            (slot selector, voice, character with the voice's own extra controls labelled per voice, mix) and the FX page.
  full      every slot a full module with all its controls (voice, level, tune, decay, drive, dist, three extras labelled
            per voice, pan, rev, dly), 4 modules per page.

Coordinates: the layout is in "shadow" coordinates, the artwork (art file=) in plugin-area coordinates (the layout's y
minus Y_OFF = 86).
"""
import glob
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.join(HERE, "..", "ports", "trkit")
ROOT = os.path.join(HERE, "..")
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


def plate(x0, label, seed, blank=False, pad=None, w=None, head=True):
    w = w or MOD_W
    y0, y1 = TOP - Y_OFF, BOT - Y_OFF
    h = y1 - y0
    o = ['<g>',
         '<rect x="%d" y="%d" width="%d" height="%d" rx="3" fill="url(#plate)" stroke="#07080a" stroke-width="1.5"/>' % (x0, y0, w, h),
         '<rect x="%g" y="%g" width="%d" height="%d" rx="2" fill="none" stroke="#ffffff" stroke-opacity="0.09" stroke-width="1"/>' % (x0 + 1.5, y0 + 1.5, w - 3, h - 3),
         # brushed-metal grain
         '<rect x="%d" y="%d" width="%d" height="%d" rx="3" fill="url(#grain)" opacity="0.5"/>' % (x0, y0, w, h)]
    for k, (sx, sy) in enumerate(((x0 + 14, y0 + 14), (x0 + w - 14, y0 + 14), (x0 + 14, y1 - 14), (x0 + w - 14, y1 - 14))):
        o.append(screw(sx, sy, seed * 4 + k))
    if blank:
        o.append('<text x="%g" y="%g" text-anchor="middle" font-family="Titillium Web" font-weight="700" font-size="30" '
                 'letter-spacing="6" fill="#5ec2b7" fill-opacity="0.55">TR-MPC</text>' % (x0 + w / 2, (y0 + y1) / 2))
        o.append('<text x="%g" y="%g" text-anchor="middle" font-family="Titillium Web" font-weight="600" font-size="13" '
                 'letter-spacing="3" fill="#9a9ea8" fill-opacity="0.6">%s</text>' % (x0 + w / 2, (y0 + y1) / 2 + 28, label))
    else:
        if head: o.append('<rect x="%d" y="%d" width="%d" height="2" fill="#5ec2b7" fill-opacity="0.55"/>' % (x0 + 26 if w > 200 else x0 + 16, y0 + 40, w - 52 if w > 200 else w - 32))
        if pad is not None:
            o.append('<text x="%g" y="%g" text-anchor="middle" font-family="Titillium Web" font-weight="600" font-size="%d" '
                     'letter-spacing="2" fill="#9a9ea8">PAD %d  %s</text>' % (x0 + w / 2, y1 - 22, 13 if w > 200 else 12, pad, pad_name(pad)))
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


def page_svg(modules, xs=None, w=None, head=True):
    """modules: list of (kind, label, pad), one per rack position (xs: their x, default the 4 wide positions)."""
    body = ['<rect width="%d" height="%d" fill="url(#bgg)"/>' % (W, H), rails()]
    for i, (kind, label, pad) in enumerate(modules):
        body.append(plate((xs or MOD_X)[i], label, i, blank=kind == "blank", pad=pad, w=w, head=head))
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


# ---- voice-specific controls (must match kExtras in ports/trkit/src/trkit.cpp) ----
KITS = [("606", "bd sd lt ht ch oh cy cp"), ("808", "bd sd lt mt ht lc mc hc rs cl ma cp cb ch oh cy"),
        ("CR78", "bd sd rs hh cy ma cl hb lb lc cb tb gu mb"), ("909", "bd sd lt mt ht rs hc ohh chh rc cr")]
LABEL = {"attack": "ATTACK", "tone": "TONE", "snappy": "SNAPPY", "noise": "NOISE", "rate": "RATE", "sweep_depth": "SWEEP",
         "pitch_mod": "PITCH MOD", "noise_decay": "NOISE DCY", "saturation": "SATURATE"}
EXTRAS = {(0, "bd"): ["attack"], (0, "sd"): ["snappy", "tone"], (0, "cp"): ["noise"],
          (1, "bd"): ["attack", "tone"], (1, "sd"): ["snappy"], (1, "ma"): ["attack"],
          (2, "sd"): ["snappy"], (2, "gu"): ["rate"],
          (3, "bd"): ["attack", "sweep_depth", "pitch_mod"], (3, "sd"): ["noise_decay", "snappy"],
          (3, "lt"): ["attack"], (3, "mt"): ["attack"], (3, "ht"): ["attack"], (3, "rs"): ["saturation"]}


KIND = {"attack": "attack", "tone": "tone", "snappy": "snappy", "noise": "noise", "rate": "rate", "sweep_depth": "sweep",
        "pitch_mod": "pmod", "noise_decay": "ndecay", "saturation": "sat"}   # engine key -> the plugin's control name


def extras_by_slot_knob():
    """{0: [(flat voice index, control name, label)], 1: [...], 2: [...]}: a voice's n-th extra control."""
    out = {0: [], 1: [], 2: []}
    flat = 0
    for b, (_, ids) in enumerate(KITS):
        for vid in ids.split():
            KIT_OF[flat] = ["6w6", "8w8", "cw78", "9w9"][b]
            for n, key in enumerate(EXTRAS.get((b, vid), [])):
                out[n].append((flat, KIND[key], LABEL[key]))
            flat += 1
    return out


KIT_OF = {}     # flat voice index -> kit key ("6w6"...), filled by extras_by_slot_knob()


def x_knobs(lines, prefix, n, cx, cy, r, when_key, img=False):
    """The n-th extra control position: a knob per voice that has one there, shown only while that voice is selected
    (named for what it does: the live name shown under the knob is the control's own name). img: in the kit's own knob."""
    for flat, kind, label in extras_by_slot_knob()[n]:
        if img:
            kit = KIT_OF[flat]
            f = FAM[kit]
            lines.append('knob cx=%d cy=%d r=%d label="%s" key=%s%s strip=images/knob_%s.png ink=%s ink_dim=%s when=%s:%d' % (
                cx, cy, r, label, prefix, kind, kit, f["ink"], f["dim"], when_key, flat))
        else:
            lines.append('knob cx=%d cy=%d r=%d label="%s" key=%s%s when=%s:%d' % (cx, cy, r, label, prefix, kind, when_key, flat))


def fx_tab(out, header_fn, svg_name):
    name = "images/%s" % svg_name
    open(os.path.join(PORT, name), "w", newline="\n").write(
        page_svg([("slot", t, None) for t, _ in FX_MODULES[:3]] + [("blank", "SHARED FX / MASTER", None)]))
    out += ["", "[tab FX - MASTER]", "art file=%s fit=stretch" % name] + header_fn
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


def build_editor():
    images = os.path.join(PORT, "images")
    out = ["# TR-MPC skin (editor): rack overview + one editor. Generated by tools/gen_trmpc_skin.py.", THEME]
    # two mixer pages: 8 compact modules each
    cw = 152
    xs = [EAR + i * (cw + 2) for i in range(8)]
    for page in range(2):
        slots = list(range(page * 8 + 1, page * 8 + 9))
        name = "images/rack_mix_%d.svg" % (page + 1)
        open(os.path.join(PORT, name), "w", newline="\n").write(
            page_svg([("slot", "S%d" % s, s) for s in slots], xs=xs, w=cw))
        out += ["", "[tab S%d - S%d]" % (slots[0], slots[-1]), "art file=%s fit=stretch" % name] + header(page + 1, 4)
        for i, sl in enumerate(slots):
            k, x0 = "s%02d_" % sl, xs[i]
            out.append('frame x=%d y=%d w=%d h=%d title="%d"' % (x0 + 30, TOP, cw - 60, BOT - TOP, sl))
            out.append('popup cx=%d cy=232 w=130 h=48 label="VOICE" key=%ssrc groups="%s"' % (x0 + cw // 2, k, GROUPS))
            out.append('knob cx=%d cy=306 r=26 label="LEVEL" key=%slevel' % (x0 + cw // 2, k))
            out.append('knob cx=%d cy=424 r=26 label="PAN" key=%span' % (x0 + cw // 2, k))
            out.append('knob cx=%d cy=542 r=26 label="REV" key=%srev' % (x0 + cw // 2, k))
        out.append('qlinks "S%d - S%d LEVEL" = %s' % (slots[0], slots[-1], ",".join("s%02d_level" % s for s in slots)))
        out.append('qlinks "S%d - S%d PAN" = %s' % (slots[0], slots[-1], ",".join("s%02d_pan" % s for s in slots)))
        out.append('qlinks "S%d - S%d REV" = %s' % (slots[0], slots[-1], ",".join("s%02d_rev" % s for s in slots)))
    # the editor page: select | voice | character | mix
    name = "images/rack_editor.svg"
    open(os.path.join(PORT, name), "w", newline="\n").write(
        page_svg([("slot", "SELECT", None), ("slot", "VOICE", None), ("slot", "CHARACTER", None), ("slot", "MIX", None)]))
    out += ["", "[tab EDITOR]", "art file=%s fit=stretch" % name] + header(3, 4)
    titles = ["SELECT SLOT", "VOICE", "CHARACTER", "MIX"]
    for i, t in enumerate(titles):
        out.append('frame x=%d y=%d w=%d h=%d title="%s"' % (MOD_X[i] + 26, TOP, MOD_W - 52, BOT - TOP, t))
    x1, x2, x3, x4 = MOD_X
    c = lambda x0: (x0 + 78, x0 + 226)
    out.append('enum_h cx=%d cy=330 label="PAD / SLOT" key=edit_slot sw=66 rows=4' % (x1 + MOD_W // 2))
    out.append('popup cx=%d cy=240 w=210 h=48 label="VOICE" key=edit_voice groups="%s"' % (x2 + MOD_W // 2, GROUPS))
    out.append('knob cx=%d cy=330 r=26 label="LEVEL" key=e_level' % c(x2)[0])
    out.append('knob cx=%d cy=330 r=26 label="TUNE" key=e_tune' % c(x2)[1])
    out.append('knob cx=%d cy=448 r=26 label="DECAY" key=e_decay' % c(x2)[0])
    out.append('knob cx=%d cy=448 r=26 label="DRIVE" key=e_drive' % c(x2)[1])
    out.append('popup cx=%d cy=240 w=210 h=48 label="DISTORTION" key=e_dist' % (x3 + MOD_W // 2))
    x_knobs(out, "e_", 0, c(x3)[0], 330, 26, "edit_voice")
    x_knobs(out, "e_", 1, c(x3)[1], 330, 26, "edit_voice")
    x_knobs(out, "e_", 2, c(x3)[0], 448, 26, "edit_voice")
    out.append('readout cx=%d cy=240 w=210 h=48 label="EDITING SLOT" key=edit_slot' % (x4 + MOD_W // 2))
    out.append('knob cx=%d cy=330 r=26 label="PAN" key=e_pan' % c(x4)[0])
    out.append('knob cx=%d cy=330 r=26 label="REV" key=e_rev' % c(x4)[1])
    out.append('knob cx=%d cy=448 r=26 label="DLY" key=e_dly' % c(x4)[0])
    out.append('qlinks "EDITOR" = edit_slot,edit_voice,e_level,e_tune,e_decay,e_drive,e_dist,e_pan,e_rev,e_dly')
    out.append('qlinks "VOICE" = e_level,e_tune,e_decay,e_drive,e_dist,e_pan,e_rev,e_dly')
    out.append('qlinks "CHARACTER" = e_attack,e_tone,e_snappy,e_noise,e_rate,e_sweep,e_pmod,e_ndecay')
    out.append('qlinks "MIX" = e_pan,e_rev,e_dly,edit_slot,edit_voice')
    fx_tab(out, header(4, 4), "rack_fx.svg")
    return "\n".join(out) + "\n"


# ---- full style: every slot a module whose faceplate follows the kit its voice belongs to ----
sys.path.insert(0, HERE)
import gen_drum_skins as drum   # per-family palettes, fonts and chassis textures of the four drum ports

FAMILIES = ["6w6", "8w8", "cw78", "9w9"]            # = the order of the kit param sNN_fam (606, 808, CR78, 909)
FAMILY_TITLE = ["TR-606", "TR-808", "CR-78", "TR-909"]
PX0, PW, PPITCH = 10, 310, 316                       # panel x of the first slot, panel width, pitch (4 panels fill 1280 px)
ROW0, ROWP, KR = 205, 86, 18                         # first knob row, row pitch, knob radius
CELL_X = (78, 232)                                   # knob cell centres, relative to the panel

FULL_THEME = THEME.replace("theme_ink=e8e9ec", "theme_ink=f1eee4").replace("theme_ink_dim=9a9ea8", "theme_ink_dim=c9c4b3") \
    .replace("theme_bg=17181b", "theme_bg=1f1f1f").replace("theme_knob_dot=5ec2b7", "theme_knob_dot=f1eee4") \
    .replace("theme_knob_face=34363c", "theme_knob_face=1b1b1d").replace("art_css=skin.css\nknob_look=cap", "art_css=skin_full.css\nknob_look=cap\nlabel_scale=0.85")

FULL_CSS = """/* TR-MPC skin (full): generated by tools/gen_trmpc_skin.py. Faceplates are the drum ports' own plate art (images/panel_*.svg);
 * lettering is outlined into the art, the knobs are the ports' filmstrips (images/knob_*.png). */
:root { --label-size: 14px; --sheen: 0.06; }
.frame-border { fill: none; stroke: none; }
.frame-rule { stroke: none; }
.frame-title { font-size: 17px; letter-spacing: 0.1em; fill: #f1eee4; }
.knob-face, .look-metal, .look-body, .look-cap-ring, .look-fader { filter: none; }
.look-cap-ring { fill: #0e0e0f; stroke: #000; stroke-opacity: 0.6; } .look-cap-top { fill: #232326; } .look-line { stroke: #f1eee4; }
"""


def _theme(kit):
    return dict(kv.split("=") for kv in drum.STYLES[kit]["theme"].split())


# per kit, straight from the drum ports' STYLES: label ink / value ink (the live text under each knob), the popup text and
# field colours, the title colour
FAM = {k: dict(ink=_theme(k)["ink"], dim=_theme(k)["ink_dim"], dist=_theme(k)["accent"], field=_theme(k)["lcd"],
               title=drum.STYLES[k]["title_color"]) for k in drum.STYLES}

# the ports' own knob filmstrips (built by their skin builds, tools/build_all.sh: _build/<kit>/skin/*/Plugin Skins/sh_knob_r26_<look>.png);
# <look> is the built-in look id: 77dc8a3f = metal, 61370ca5 = cap. Copied into ports/trkit/images/ so they are vendored.
KNOB_STRIPS = {"6w6": ("6w6", "77dc8a3f"), "8w8": ("8w8", "61370ca5"), "8w8_level": ("8w8", "77dc8a3f"),
               "cw78": ("cw78", "61370ca5"), "9w9": ("9w9", "61370ca5")}


def vendor_knobs():
    for name, (kit, look) in KNOB_STRIPS.items():
        dst = os.path.join(PORT, "images", "knob_%s.png" % name)
        hits = glob.glob(os.path.join(ROOT, "_build", kit, "skin", "*", "Plugin Skins", "sh_knob_r26_%s.png" % look))
        if hits:
            shutil.copyfile(hits[0], dst)
        elif not os.path.isfile(dst):
            raise SystemExit("no knob strip for %s: run tools/build_all.sh %s first" % (name, kit))


def crop_svg(svg, x, y, w, h):
    """The same drawing, showing only the box x,y,w,h (the svg's viewBox is moved; the drawing is untouched)."""
    return re.sub(r'viewBox="[^"]*" width="\d+" height="\d+"', 'viewBox="%g %g %g %g" width="%g" height="%g"' % (x, y, w, h, w, h), svg, count=1)


def family_panel_svg(fi, cells, blank=False):
    """One module's faceplate: the drum port's own panel art (gen_drum_skins.plate_svg, cropped to one panel, without a title),
    plus the Distortion field drawn in the kit's colours. cells: (cx, cy, kind, caption); only 'popup' cells draw anything."""
    kit = FAMILIES[fi]
    f = FAM[kit]
    svg = drum.plate_svg(kit, 1, [""], name="TR-MPC")
    svg = re.sub(r'<rect [^>]*height="26" rx="13" fill="#f3efdc"/>', '<rect x="%g" y="%d" width="206" height="26" rx="13" fill="#f3efdc"/>' % (
        drum.panel_xs(1)[0] + 10, TOP - Y_OFF + 7), svg) if kit == "8w8" else svg   # the 808's title pill, sized for a voice name
    svg = crop_svg(svg, drum.panel_xs(1)[0], TOP - Y_OFF, PW, BOT - TOP)
    add = []
    for cx, cy, kind, caption in cells:
        if kind != "popup":
            continue
        # the popup's own caption and field fill would sit under this art, so both are drawn here
        add.append('<text x="%g" y="%g" font-family="Titillium Web" font-weight="600" font-size="14" letter-spacing="1.2" fill="#%s">%s</text>'
                   % (cx - 65, cy - TOP - 28, f["ink"], caption))
        add.append('<rect x="%g" y="%g" width="130" height="44" fill="#%s"/>' % (cx - 65, cy - TOP - 22, f["field"]))
    # the title menu's arrow
    add.append('<path d="M%d 15 L%d 15 L%d 23 Z" fill="#%s"/>' % (PW - 34, PW - 18, PW - 26, f["title"]))
    return svg.replace("</svg>", "".join(add) + "</svg>")


def full_chassis_svg():
    """The page behind the modules: the 808's satin-black chassis, with the page header's lettering outlined into the art
    (gen_drum_skins.text_path: no font files are needed at build time or in CI)."""
    hy = drum.HEADER_CY - Y_OFF
    logo, _ = drum.text_path("Eurostile", "TR-MPC", 24, hy, 30, "5ec2b7")
    tag, _ = drum.text_path("Helvetica", "16-SLOT MODULAR DRUM RACK - ANY VOICE FROM 6W6 / 8W8 / CW-78 / 9W9", 24 + 3 * 15 * 6 + 40, hy + 4, 13,
                            "c9c4b3", spacing=2, bold=0.02)
    d = [drum.DEFS, drum.base("8w8"), logo, tag,
         '<rect x="0" y="%d" width="%d" height="2" fill="#5ec2b7" fill-opacity="0.7"/>' % (drum.HEAD_Y1 - Y_OFF - 3, W)]
    return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d">%s</svg>\n' % (W, H, W, H, "".join(d))


def full_header():
    return []   # the header lettering is part of images/chassis.svg


def full_cells():
    """Knob cells of a slot panel, 6 rows x 2: Distortion + Level on top, the voice's own controls and Drive after them, Rev + Dly last.
    Returns {name: (col, row)}."""
    return {"dist": (0, 0), "level": (1, 0), "tune": (0, 1), "decay": (1, 1), "x1": (0, 2), "x2": (1, 2), "x3": (0, 3),
            "drive": (1, 3), "pan": (0, 4), "rev": (0, 5), "dly": (1, 5)}


def build_full(pages=4):
    images = os.path.join(PORT, "images")
    cells = full_cells()
    out = ["# TR-MPC skin (full): every slot a module that reskins itself to the kit its voice comes from. Generated by tools/gen_trmpc_skin.py.",
           FULL_THEME]
    cell_xy = lambda px, col, row: (px + CELL_X[col], ROW0 + row * ROWP)
    open(os.path.join(images, "chassis.svg"), "w", newline="\n").write(full_chassis_svg())
    vendor_knobs()
    dist_c = cell_xy(0, *cells["dist"])
    for fi, kit in enumerate(FAMILIES):
        open(os.path.join(images, "panel_%s.svg" % kit), "w", newline="\n").write(
            family_panel_svg(fi, [(CELL_X[cells["dist"][0]], ROW0 + cells["dist"][1] * ROWP + 14, "popup", "DISTORTION")]))
    for page in range(pages):
        slots = list(range(page * 4 + 1, page * 4 + 5))
        out += ["", "[tab S%d - S%d]" % (slots[0], slots[-1]), "art file=images/chassis.svg fit=stretch"] + full_header()
        for i, sl in enumerate(slots):
            k, px = "s%02d_" % sl, PX0 + i * PPITCH
            for fi, kit in enumerate(FAMILIES):
                f, w = FAM[kit], "when=%sfam:%d" % (k, fi)
                out.append("art file=images/panel_%s.svg x=%d y=%d w=%d h=%d %s" % (kit, px, TOP, PW, BOT - TOP, w))
                # the voice menu doubles as the module's title: the voice's name, in the kit's colour
                out.append('popup cx=%d cy=%d w=200 h=34 label="" key=%ssrc groups="%s" accent=%s %s' % (px + 108, TOP + 19, k, GROUPS, f["title"], w))
                c = cell_xy(px, *cells["dist"])
                out.append('popup cx=%d cy=%d w=130 h=44 label="" key=%sdist accent=%s %s' % (c[0], c[1] + 14, k, f["dist"], w))
                for name, label in (("level", "LEVEL"), ("tune", "TUNE"), ("decay", "DECAY"), ("drive", "DRIVE"), ("pan", "PAN"),
                                    ("rev", "REV"), ("dly", "DLY")):
                    c = cell_xy(px, *cells[name])
                    img = "knob_8w8_level" if (kit == "8w8" and name == "level") else "knob_%s" % kit
                    out.append('knob cx=%d cy=%d r=%d label="%s" key=%s%s strip=images/%s.png ink=%s ink_dim=%s %s' % (
                        c[0], c[1], KR, label, k, name, img, f["ink"], f["dim"], w))
            for n, name in enumerate(("x1", "x2", "x3")):
                c = cell_xy(px, *cells[name])
                x_knobs(out, k, n, c[0], c[1], KR, k + "src", img=True)
        out.append('qlinks "S%d - S%d" = %s' % (slots[0], slots[-1], ",".join(
            "s%02d_%s" % (s, kk) for s in slots for kk in ("src", "level", "tune", "decay"))))
        for sl in slots:
            keys = ["src", "level", "tune", "decay", "drive", "dist", "pan", "rev", "dly"]
            out.append('qlinks "SLOT %d" = %s' % (sl, ",".join("s%02d_%s" % (sl, kk) for kk in keys[:8])))
            out.append('qlinks "SLOT %d CHARACTER" = %s' % (sl, ",".join("s%02d_%s" % (sl, kk) for kk in (
                "attack", "tone", "snappy", "noise", "rate", "sweep", "pmod", "ndecay"))))
    if pages in (0, 4):   # the FX page: neutral charcoal modules in the 808 finish
        out += ["", "[tab FX - MASTER]", "art file=images/chassis.svg fit=stretch"] + full_header()
        fx_cells = [(CELL_X[0], ROW0), (CELL_X[1], ROW0), (CELL_X[0], ROW0 + ROWP), (CELL_X[1], ROW0 + ROWP), (CELL_X[0], ROW0 + 2 * ROWP)]
        for i, (title, ctrls) in enumerate(FX_MODULES):
            px = PX0 + i * PPITCH
            fcells, lines = [], []
            for kind, dx, cy, label, key in ctrls:
                col = 0 if dx < 150 else 1
                row = {218: 0, 240: 0, 246: 0, 348: 1, 478: 2}[cy]
                x, y = cell_xy(px, col, row)
                if kind == "popup":
                    fcells.append((x - px, y, "popup", label))
                    lines.append('popup cx=%d cy=%d w=130 h=44 label="" key=%s' % (x, y + 14, key))
                else:
                    fcells.append((x - px, y, "knob", ""))
                    lines.append('knob cx=%d cy=%d r=%d label="%s" key=%s' % (x, y, KR, label, key))
            name = "images/panel_fx%d.svg" % (i + 1)
            open(os.path.join(PORT, name), "w", newline="\n").write(family_panel_svg(1, fcells))
            out.append("art file=%s x=%d y=%d w=%d h=%d" % (name, px, TOP, PW, BOT - TOP))
            out.append('frame x=%d y=%d w=%d h=%d title="%s"' % (px + 8, TOP, PW - 16, 44, title))
            out += lines
        px = PX0 + 3 * PPITCH
        open(os.path.join(PORT, "images/panel_blank.svg"), "w", newline="\n").write(family_panel_svg(1, []))
        nm, _ = drum.text_path("Eurostile", "TR-MPC", PW / 2, 30, 30, "5ec2b7", opacity=0.55, anchor="middle")
        open(os.path.join(PORT, "images/blank_name.svg"), "w", newline="\n").write(
            '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d 60" width="%d" height="60">%s</svg>\n' % (PW, PW, nm))
        out.append("art file=images/panel_blank.svg x=%d y=%d w=%d h=%d" % (px, TOP, PW, BOT - TOP))
        out.append("art file=images/blank_name.svg x=%d y=%d w=%d h=%d" % (px, (TOP + BOT) // 2 - 40, PW, 60))
        out += ['qlinks "FX - MASTER" = fx_rev_decay,fx_rev_tone,fx_rev_level,fx_dly_time,fx_dly_fdbk,fx_dly_level,fx_master_dist,fx_master_drive',
                'qlinks "REVERB" = fx_rev_decay,fx_rev_tone,fx_rev_hpf,fx_rev_level',
                'qlinks "DELAY" = fx_dly_time,fx_dly_fdbk,fx_dly_tone,fx_dly_hpf,fx_dly_level',
                'qlinks "MASTER" = fx_master_dist,fx_master_drive,fx_comp,fx_volume']
    return "\n".join(out) + "\n"


def main():
    os.makedirs(os.path.join(PORT, "layouts"), exist_ok=True)
    open(os.path.join(PORT, "layouts", "restyle.conf"), "w", newline="\n").write(build_restyle())
    open(os.path.join(PORT, "layouts", "editor.conf"), "w", newline="\n").write(build_editor())
    open(os.path.join(PORT, "layouts", "full.conf"), "w", newline="\n").write(build_full())
    open(os.path.join(PORT, "layouts", "full_preview.conf"), "w", newline="\n").write(build_full(pages=1))   # one page: quick previews
    open(os.path.join(PORT, "layouts", "full_fx_preview.conf"), "w", newline="\n").write(build_full(pages=0))   # the FX page only
    open(os.path.join(PORT, "skin.css"), "w", newline="\n").write(CSS)
    open(os.path.join(PORT, "skin_full.css"), "w", newline="\n").write(FULL_CSS)
    print("wrote layouts/{restyle,editor,full}.conf, skin.css, images/")


if __name__ == "__main__":
    main()
