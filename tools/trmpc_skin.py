#!/usr/bin/env python3
"""TR-MPC's skin: a rack of 16 slot modules, each reskinning itself to the drum kit its voice comes from, drawn with
the kits' own realistic looks (ports/<kit>/skin/style.py, via tools/drum_skin.py).

    python3 tools/trmpc_skin.py      # -> ports/trkit/layout.conf, ports/trkit/art/*.png, ports/trkit/build/trmpc_skin/*.png

A module's faceplate is cut from its kit's panel (the kit's style.panel()), with the kit's heading behind the voice menu
(the menu doubles as the module's title), the kit's knobs and black screws in the corners. Each slot's derived "kit"
parameter (sNN_fam: 0 6W6, 1 8W8, 2 CW-78, 3 9W9, 4 the 9W9 kick, which has a third extra control and so six rows)
switches its faceplate and controls with `when=sNN_fam:<n>`; a voice's own extra controls (Attack, Snappy ...) are knobs
shown only while that voice is picked (`when=sNN_src:<flat voice index>`), with their scale and name drawn on their own
images, so a module only shows what its voice has.

Module rows (as the drum kits': the top 2x2 is the slot's Q-Link column on the page's overview):
    LEVEL  TUNE / DECAY DRIVE / the voice's extras / DISTORTION PAN / REVERB DELAY

Q-Links: a voice page's overview is Level, Tune, Decay, Drive for its four slots; its VOICES sub-page has the four
voice menus together; each SLOT page has Level/Tune/Decay/Drive, Distortion/Pan/Rev/Dly and the voice extras.
qlink_bounds=column outlines the column in use. The FX/MASTER and RANDOMISE pages are 8W8-style modules."""
import os
import shutil
import sys

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import drum_skin as ds                    # noqa: E402
import gen_trmpc_skin as base             # noqa: E402  (voice tables only: KITS, EXTRAS, KIND, LABEL)

ROOT = ds.ROOT
PORT = os.path.join(ROOT, "ports", "trkit")
SS, W, H, Y_OFF, FRAMES = ds.SS, ds.W, ds.H, ds.Y_OFF, ds.FRAMES
PX0, PW, PPITCH = 10, 310, 316            # module x, width, pitch: 4 modules across
MT, MB = 50, 622                          # module top and bottom (plugin y)
TITLE_Y = 84                              # the module heading's centre line, under the top screws
TITLE_CW78 = (104, 176)                    # CW-78's voice menu: centre and width, clear of its coloured keys on the right
XM = 330                                  # where a module is drawn on its kit's full panel before it is cut out
CX = (80, 230)                            # knob cell centres in a module
FAMILIES = ["6w6", "8w8", "cw78", "9w9", "9w9"]          # sNN_fam order; 4 = the 9W9 kick (six rows)
REG = dict(rows=(160, 252, 344, 436, 528), r=22, label=14, dy=-42)
DENSE = dict(rows=(150, 228, 306, 384, 462, 540), r=18, label=13, dy=-36)
CELLS = {"level": (0, 0), "tune": (1, 0), "decay": (0, 1), "drive": (1, 1), "dist": (0, 3), "pan": (1, 3), "rev": (0, 4),
         "dly": (1, 4)}
CELLS_DENSE = dict(CELLS, dist=(0, 4), pan=(1, 4), rev=(0, 5), dly=(1, 5))
EXTRA_CELLS = [(0, 2), (1, 2), (0, 3)]                  # a voice's 1st, 2nd and 3rd extra control
FIXED = [("level", "LEVEL"), ("tune", "TUNE"), ("decay", "DECAY"), ("drive", "DRIVE"), ("pan", "PAN"), ("rev", "REVERB"),
         ("dly", "DELAY")]
ORANGE = (242, 86, 29)
# the voice menu's groups (heading fill / text, option fill / text per kit) and each kit's title and distortion ink
GROUPS = ("6W6 TR-606:8:b9bbb4:1a1d24:d9dad4:1a1d24,8W8 TR-808:16:141414:f1f827:202020:ffffff,"
          "CW-78 CR-78:14:121316:ff5a00:1c1d21:ecd5aa,9W9 TR-909:11:e6e2d6:4b5566:e6e2d6:ef6c1f")
TITLE_INK = {"6w6": "1a1c22", "8w8": "1a1a1a", "cw78": "f07028", "9w9": "f07828"}
DIST_INK = {"6w6": "ff3b30", "8w8": "f0aa3c", "cw78": "ff7a30", "9w9": "f07828"}
THEME = ("bg=1f1f1f ink=f1eee4 ink_dim=c9c4b3 accent=f0aa3c accent_hi=f8c060 seg_active=f0aa3c seg_inactive=2b2d31 "
         "seg_active_tx=14161a lcd=26272c line=585b64 btn_bg=2b2d31 btn_text=e8e9ec box=101214 display_ink=f0aa3c")


def geo(fam):
    return (DENSE, CELLS_DENSE) if fam == 4 else (REG, CELLS)


def cell_xy(g, cells, name):
    c, r = cells[name] if isinstance(name, str) else name
    return CX[c], g["rows"][r]


class Out:
    def __init__(self):
        self.art = os.path.join(PORT, "art")
        self.prev = os.path.join(PORT, "build", "trmpc_skin")
        shutil.rmtree(self.art, ignore_errors=True)
        os.makedirs(self.art)
        os.makedirs(self.prev, exist_ok=True)
        self.lines = []

    def save(self, im, name):
        im.save(os.path.join(self.art, name), optimize=True)
        return "art/" + name


# ---------------------------------------------------------------- modules

def kit_tab(kit, g):
    """A kit's full panel (its style's plate) to draw a module on, sized for the module's rows."""
    st = ds.load_style(kit)
    t = ds.Tab(st, "module")
    t.r, t.label_size, t.label_dy = g["r"], g["label"], g["dy"]
    t.tile_y = (TITLE_Y - 16, TITLE_Y + 16)
    return t


def module_edges(t, x0, title=True):
    """The module's edges (lit top left, shadowed bottom right), its four screws and the kit's heading."""
    d = t.d
    x1 = x0 + PW
    d.line([(x0 * SS, MT * SS), (x1 * SS, MT * SS)], fill=(150, 150, 150), width=SS)
    d.line([(x0 * SS, MT * SS), (x0 * SS, MB * SS)], fill=(130, 130, 130), width=SS)
    d.line([(x0 * SS, MB * SS), (x1 * SS, MB * SS)], fill=(0, 0, 0), width=2 * SS)
    d.line([(x1 * SS, MT * SS), (x1 * SS, MB * SS)], fill=(0, 0, 0), width=2 * SS)
    if title:
        t.tile(x0 + 6, "")
    for sx, sy in ((x0 + 13, MT + 12), (x1 - 13, MT + 12), (x0 + 13, MB - 12), (x1 - 13, MB - 12)):
        ds.screw(t, sx, sy, 6)


def cut(t, x0=XM):
    """The module region of a tab's hi-res image."""
    return t.hi.crop((x0 * SS, MT * SS, (x0 + PW) * SS, MB * SS))


def knob_strip(t, cx, cy, r, level, ticks=False, name=None):
    """A knob filmstrip cut from t at (cx, cy); with ticks and name (name = (text, dy, size)) drawn into each frame,
    for a knob that isn't on every voice. Such a frame is square and big enough to hold the name above the knob."""
    side = 2 * r + (34 if ticks else 8)
    if name:
        text, dy, size = name
        side = max(side, 2 * (-dy + size // 2 + 4), int(ds.font(size).getlength(text) / SS) + 12)
        t.text(cx, cy + dy, text, size)
    x0, y0 = int((cx - side / 2) * SS), int((cy - side / 2) * SS)
    if ticks:
        t.ticks(cx, cy, r)
        box = ((cx - r) * SS, (cy - r) * SS, (cx + r) * SS, (cy + r) * SS)
        t.hi.alpha_composite(ds.soft_shadow(t.hi.size, box, 4, 170, (3, 5)))
        t.hi.alpha_composite(ds.soft_shadow(t.hi.size, box, 1, 180, (1, 1)))
    base_im = t.hi.crop((x0, y0, x0 + side * SS, y0 + side * SS))
    strip = Image.new("RGB", (side, side * FRAMES))
    mod = t.st["module"]
    for i in range(FRAMES):
        deg = -ds.SWEEP / 2 + ds.SWEEP * i / (FRAMES - 1)
        f = base_im.copy()
        sp = mod.knob_sprite(r, level, deg)
        f.alpha_composite(sp, ((f.width - sp.width) // 2, (f.height - sp.height) // 2))
        strip.paste(ds.final(f, side, side), (0, i * side))
    return strip, side


def label_image(t, cx, cy, text, size):
    """A knob's name on a crop of its plate (for a knob that isn't on every voice)."""
    w, h = 120, size + 10
    x0, y0 = int((cx - w / 2) * SS), int((cy - h / 2) * SS)
    tt = ds.Tab.__new__(ds.Tab)
    tt.st, tt.hi = t.st, t.hi.crop((x0, y0, x0 + w * SS, y0 + h * SS))
    tt.d = ImageDraw.Draw(tt.hi)
    tt.text(w / 2, h / 2, text, size)
    return ds.final(tt.hi, w, h), w, h


# The kit's name running up the module's left edge, faint, in the kit's own lettering (as the earlier TR-MPC skin had it)
WATERMARK = {"6w6": ("6W6", "michroma", (26, 28, 34), 60), "8w8": ("8W8", "logo", (232, 98, 40), 88),
             "cw78": ("CW-78", "filmotype", (240, 112, 40), 80), "9w9": ("9W9", "earth", (78, 82, 94), 76)}


def watermark(t, kit):
    text, face, rgb, alpha = WATERMARK[kit]
    size = 42
    while size > 24 and ds.font(size, face).getlength(text) / SS > 118:   # stays beside the bottom row, under Distortion
        size -= 1
    f = ds.font(size, face)
    w = int(f.getlength(text)) + 8 * SS
    h = size * SS + 8 * SS
    layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(layer).text((4 * SS, h // 2), text, font=f, fill=rgb + (alpha,), anchor="lm")
    layer = layer.rotate(90, expand=True)                     # reads bottom to top
    x, y = (XM + 8) * SS, (MB - 26) * SS - layer.height      # left edge, above the bottom screw
    t.hi.alpha_composite(layer, (int(x), int(y)))


def family_module(out, fam):
    """Module art for sNN_fam = fam, the fixed knobs' strips, and the strips and name images of every voice extra a
    voice of this kit can have. Returns {name: strip file}, {(n, kind): (strip, label image)} and the art file."""
    kit = FAMILIES[fam]
    g, cells = geo(fam)
    t = kit_tab(kit, g)
    module_edges(t, XM)
    watermark(t, kit)
    ax = XM + (TITLE_CW78[0] + TITLE_CW78[1] // 2 - 4 if kit == "cw78" else PW - 44)   # the voice menu's arrow (CW-78: clear of its keys)
    ink = tuple(int(TITLE_INK[kit][i:i + 2], 16) for i in (0, 2, 4))
    t.d.polygon([((ax - 7) * SS, (TITLE_Y - 4) * SS), ((ax + 7) * SS, (TITLE_Y - 4) * SS), (ax * SS, (TITLE_Y + 5) * SS)], fill=ink)
    for name, label in FIXED:
        cx, cy = cell_xy(g, cells, name)
        t.knob("x", XM + cx, cy, label, name == "level")
    cx, cy = cell_xy(g, cells, "dist")
    t.popup("x", XM + cx, cy, "DISTORTION")
    clean = t.hi.copy()                   # before any extras: their scale and name come with their own images
    strips = {}
    for name, _ in FIXED:
        cx, cy = cell_xy(g, cells, name)
        im, side = knob_strip(t, XM + cx, cy, g["r"], name == "level")
        strips[name] = (out.save(im, "m%d_%s.png" % (fam, name)), side)
    art = out.save(ds.final(cut(t), PW, MB - MT), "module_%d.png" % fam)
    # A voice's own extra controls: knobs whose strips carry their scale and name, cut from the plate with the fixed
    # controls already on it, so a frame matches whatever it overlaps. (Separate name images with `when=` were merged by
    # MPC's skin builder into one picture per mode and came out with black gaps and edges.)
    extras = {}
    for n, items in base.extras_by_slot_knob().items():
        for flat, kind, label in items:
            if base.KIT_OF[flat] != kit or (flat == NINE_BD) != (fam == 4) or n >= (3 if fam == 4 else 2):
                continue
            if (n, kind) in extras:
                continue
            cx, cy = cell_xy(g, cells, EXTRA_CELLS[n])
            tt = kit_tab(kit, g)
            tt.hi = clean.copy()
            tt.d = ImageDraw.Draw(tt.hi)
            im, side = knob_strip(tt, XM + cx, cy, g["r"], False, ticks=True, name=(label, g["dy"], g["label"]))
            extras[(n, kind)] = (out.save(im, "x%d_%d_%s.png" % (fam, n, kind)), side)
    return dict(art=art, strips=strips, extras=extras)


NINE_BD = 38    # the 9W9 kick's flat voice index (gen_trkit_params' voice order)


def slot_lines(out, mods, s, px):
    k = "s%02d_" % s
    dx = px
    for fam in range(5):
        kit = FAMILIES[fam]
        g, cells = geo(fam)
        m = mods[fam]
        w = "when=%sfam:%d" % (k, fam)
        out.lines.append("art file=%s x=%d y=%d w=%d h=%d %s" % (m["art"], px, MT + Y_OFF, PW, MB - MT, w))
        tcx, tw = TITLE_CW78 if kit == "cw78" else (PW // 2, 250)
        out.lines.append('popup cx=%d cy=%d w=%d cw=118 h=34 label="" key=%ssrc groups="%s" accent=%s field=none %s' % (
            px + tcx, TITLE_Y + Y_OFF, tw, k, GROUPS, TITLE_INK[kit], w))
        for name, _ in FIXED:
            cx, cy = cell_xy(g, cells, name)
            f, side = m["strips"][name]
            out.lines.append('knob cx=%d cy=%d r=%d label="" key=%s%s strip=%s frames=%d ns=0 vs=0 bw=96 %s' % (
                dx + cx, cy + Y_OFF, (side - 8) // 2, k, name, f, FRAMES, w))
        cx, cy = cell_xy(g, cells, "dist")
        out.lines.append('popup cx=%d cy=%d w=112 h=36 label="" key=%sdist accent=%s %s' % (dx + cx, cy + Y_OFF, k, DIST_INK[kit], w))
    for n, items in base.extras_by_slot_knob().items():
        for flat, kind, label in items:
            fam = 4 if flat == NINE_BD else FAMILIES.index(base.KIT_OF[flat])
            e = mods[fam]["extras"].get((n, kind))
            if not e:
                continue
            g, cells = geo(fam)
            cx, cy = cell_xy(g, cells, EXTRA_CELLS[n])
            f, side = e
            w = "when=%ssrc:%d" % (k, flat)
            out.lines.append('knob cx=%d cy=%d r=%d label="" key=%s%s strip=%s frames=%d ns=0 vs=0 bw=96 %s' % (
                dx + cx, cy + Y_OFF, (side - 8) // 2, k, kind, f, FRAMES, w))


def chassis(out, name, sub):
    """The rack every page sits in: dark, with rails top and bottom, the name and a line in the header."""
    t = ds.Tab.__new__(ds.Tab)
    t.st = dict(ink=(200, 200, 196))
    t.hi = Image.new("RGBA", (W * SS, H * SS), (0, 0, 0, 255))
    t.d = ImageDraw.Draw(t.hi)
    g = Image.linear_gradient("L").resize((W * SS, H * SS)).point(lambda v: int(30 - 10 * v / 255))
    t.hi.paste(Image.merge("RGB", (g, g, g.point(lambda v: v + 2))), (0, 0))
    for y0, y1 in ((MT - 4, MT), (MB, MB + 4)):                # rails behind the modules' top and bottom edges
        t.d.rectangle((0, y0 * SS, W * SS, y1 * SS), fill=(70, 70, 74))
    t.text(22, 36, "TR-MPC", 30, ORANGE, "michroma", "ls", 1.0)
    t.text(22 + ds.font(30, "michroma").getlength("TR-MPC") / SS + 24, 34, sub, 14, (170, 170, 166), "bold", "ls")
    return t


def module_on(t, kit, px, title):
    """A whole module (an FX / utility one) drawn straight onto page t at px, in kit's look, with a fixed title."""
    g = REG
    kt = kit_tab(kit, g)
    module_edges(kt, XM)
    kt.text(XM + PW / 2, 75, title, 17, None, "bold")
    t.hi.paste(cut(kt), (px * SS, MT * SS))
    return kt


# ---------------------------------------------------------------- keys

def key_image(w, h, text, colour, lit, size):
    """A lit or unlit key, RGBA: a dark bezel, the key's face (glowing when lit), a soft top sheen, its lettering."""
    s = SS
    im = Image.new("RGBA", (w * s, h * s), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((2 * s, 2 * s, (w - 2) * s, (h - 2) * s), 10 * s, fill=(10, 10, 12, 255))
    face = colour if lit else tuple(int(c * 0.32) for c in colour)
    d.rounded_rectangle((6 * s, 5 * s, (w - 6) * s, (h - 7) * s), 8 * s, fill=face + (255,))
    sheen = Image.new("RGBA", im.size, (0, 0, 0, 0))
    ImageDraw.Draw(sheen).rounded_rectangle((6 * s, 5 * s, (w - 6) * s, (h // 2) * s), 8 * s, fill=(255, 255, 255, 80 if lit else 24))
    im.alpha_composite(sheen)
    d.text((w / 2 * s, (h / 2 - 1) * s), text, font=ds.font(size, "bold"), fill=(26, 26, 26) if lit else (140, 136, 126), anchor="mm")
    return im.resize((w, h), Image.LANCZOS)


def key_attrs(out, name, w, h, text, colour, size):
    for lit in (0, 1):
        out.save(key_image(w, h, text, colour, lit, size), "%s_%s.png" % (name, "on" if lit else "off"))
    return "img=art/%s_off.png img_on=art/%s_on.png w=%d h=%d" % (name, name, w, h)


# ---------------------------------------------------------------- pages

def page_image(out, t, slug):
    bg = ds.final(t.hi, W, H)
    return out.save(bg, "page_%s.png" % slug)


def voice_page(out, mods, page):
    slots = list(range(page * 4 + 1, page * 4 + 5))
    name = "S%d - S%d" % (slots[0], slots[-1])
    t = chassis(out, name, "16-SLOT MODULAR DRUM RACK  -  6W6 / 8W8 / CW-78 / 9W9")
    out.lines += ["", "[tab %s]" % name, "art file=%s fit=stretch" % page_image(out, t, "voices")]
    for i, s in enumerate(slots):
        slot_lines(out, mods, s, PX0 + i * PPITCH)
    ql = lambda s, keys: ",".join("s%02d_%s" % (s, kk) for kk in keys)   # noqa: E731
    out.lines.append('qlinks "%s" = %s' % (name, ",".join(ql(s, ("level", "tune", "decay", "drive")) for s in slots)))
    out.lines.append('qlinks "VOICES %d-%d" = %s' % (slots[0], slots[-1], ",".join("s%02d_src" % s for s in slots)))
    for s in slots:
        out.lines.append('qlinks "SLOT %d" = %s' % (s, ql(s, ("level", "tune", "decay", "drive", "attack", "tone", "snappy", "noise",
                                                             "rate", "sweep", "pmod", "ndecay", "dist", "pan", "rev", "dly"))))


def fx_page(out):
    t = chassis(out, "FX/MASTER", "SHARED FX AND MASTER  -  LIT = ON, DARK = SEND ONLY (TR-MPC TAP FX)")
    g, kit = REG, "8w8"
    mods = [("REVERB", "int_rev", [("fx_rev_level", "LEVEL", 1), ("fx_rev_decay", "DECAY", 0), ("fx_rev_tone", "TONE", 0),
                                   ("fx_rev_hpf", "HPF", 0)]),
            ("DELAY", "int_dly", [("fx_dly_level", "LEVEL", 1), ("fx_dly_time", "TIME", 0), ("fx_dly_fdbk", "FEEDBACK", 0),
                                  ("fx_dly_tone", "TONE", 0), ("fx_dly_hpf", "HPF", 0)]),
            ("MASTER", "int_comp", [("fx_volume", "VOLUME", 1), ("fx_master_drive", "DRIVE", 0), ("fx_master_dist", "DISTORTION", 0),
                                    ("fx_comp", "COMP", 0)])]
    lines = []
    for i, (title, ikey, ctrls) in enumerate(mods):
        px = PX0 + i * PPITCH
        kt = kit_tab(kit, g)
        module_edges(kt, XM, title=True)
        kt.text(XM + PW / 2 + 6, TITLE_Y, title, 17, (24, 24, 24), "bold")
        knobs = []
        for j, (key, label, lvl) in enumerate(ctrls):
            cx, cy = CX[j % 2], g["rows"][j // 2]
            if key in ("fx_dly_time", "fx_master_dist"):
                kt.popup("x", XM + cx, cy + 8, label)
                lines.append('popup cx=%d cy=%d w=112 h=36 label="" key=%s accent=%s' % (px + cx, cy + 8 + Y_OFF, key, DIST_INK[kit]))
            else:
                kt.knob("x", XM + cx, cy, label, lvl)
                knobs.append((key, cx, cy, lvl))
        for key, cx, cy, lvl in knobs:
            im, side = knob_strip(kt, XM + cx, cy, g["r"], lvl)
            lines.append('knob cx=%d cy=%d r=%d label="" key=%s strip=%s frames=%d ns=0 vs=0 bw=96' % (
                px + cx, cy + Y_OFF, (side - 8) // 2, key, out.save(im, "%s.png" % key), FRAMES))
        t.hi.paste(cut(kt), (px * SS, MT * SS))
        lines.append('toggle cx=%d cy=%d label="" key=%s bw=240 ns=0 %s' % (px + PW // 2, 540 + Y_OFF, ikey,
                                                               key_attrs(out, "key_" + ikey, 240, 70, title, ORANGE, 24)))
    kt = kit_tab(kit, g)                    # a blank module fills the fourth place
    module_edges(kt, XM, title=False)
    t.hi.paste(cut(kt), ((PX0 + 3 * PPITCH) * SS, MT * SS))
    out.lines += ["", "[tab FX/MASTER]", "art file=%s fit=stretch" % page_image(out, t, "fx")] + lines
    out.lines.append('qlinks "FX/MASTER" = fx_rev_level,fx_rev_decay,fx_rev_tone,fx_rev_hpf,fx_dly_level,fx_dly_time,fx_dly_fdbk,'
                     'fx_dly_tone,fx_volume,fx_master_drive,fx_master_dist,fx_comp,int_rev,int_dly,int_comp,fx_dly_hpf')


def rnd_page(out):
    t = chassis(out, "RANDOMISE", "RANDOMISE THE SELECTED SLOTS' PARAMETERS OR VOICES")
    kit, lines, grey = "8w8", [], (201, 196, 179)
    for i, title in enumerate(("SLOTS 1-8", "SLOTS 9-16")):
        px = PX0 + i * PPITCH
        kt = kit_tab(kit, REG)
        module_edges(kt, XM)
        kt.text(XM + PW / 2 + 6, TITLE_Y, title, 17, (24, 24, 24), "bold")
        t.hi.paste(cut(kt), (px * SS, MT * SS))
        for j in range(8):
            n = i * 8 + j + 1
            lines.append('toggle cx=%d cy=%d label="" key=rnd_s%02d bw=120 ns=0 %s' % (px + CX[j % 2], 160 + (j // 2) * 122 + Y_OFF, n,
                                                                          key_attrs(out, "key_rnd_s%02d" % n, 120, 96, str(n), grey, 40)))
    for i, (title, keys) in enumerate((("SELECT", [("rnd_all", "SELECT ALL", grey, 26), ("rnd_none", "CLEAR", grey, 30)]),
                                       ("RANDOMISE", [("rnd_go", "PARAMETERS", ORANGE, 26), ("rnd_go_voice", "VOICES", ORANGE, 30)]))):
        px = PX0 + (2 + i) * PPITCH
        kt = kit_tab(kit, REG)
        module_edges(kt, XM)
        kt.text(XM + PW / 2 + 6, TITLE_Y, title, 17, (24, 24, 24), "bold")
        ys = (250, 420) if i == 0 else (380, 530)
        if i == 1:
            kt.r, kt.label_dy = 30, -54
            kt.knob("x", XM + PW // 2, 200, "AMOUNT", True)
            im, side = knob_strip(kt, XM + PW // 2, 200, 30, True)
            lines.append('knob cx=%d cy=%d r=%d label="" key=rnd_amount strip=%s frames=%d ns=0 vs=0 bw=120' % (
                px + PW // 2, 200 + Y_OFF, (side - 8) // 2, out.save(im, "rnd_amount.png"), FRAMES))
        t.hi.paste(cut(kt), (px * SS, MT * SS))
        for (key, text, colour, size), y in zip(keys, ys):
            lines.append('button cx=%d cy=%d label="" key=%s %s' % (px + PW // 2, y + Y_OFF, key,
                                                                   key_attrs(out, "key_" + key, 250, 110, text, colour, size)))
    out.lines += ["", "[tab RANDOMISE]", "art file=%s fit=stretch" % page_image(out, t, "rnd")] + lines
    out.lines.append('qlinks "RANDOMISE" = rnd_amount,rnd_go,rnd_go_voice,rnd_all,rnd_none')


def preview(out):
    """A rough offline look at a voice page: slot n shows kit n % 4, every knob at the start of its sweep."""
    bg = Image.open(os.path.join(PORT, out.lines[out.lines.index("[tab S1 - S4]") + 1].split("=")[1].split()[0])).convert("RGB")
    for i in range(4):
        m = Image.open(os.path.join(PORT, "art", "module_%d.png" % [1, 2, 0, 3][i])).convert("RGB")
        bg.paste(m, (PX0 + i * PPITCH, MT))
    bg.save(os.path.join(out.prev, "voices.png"))


def main():
    out = Out()
    out.lines = ["# generated by tools/trmpc_skin.py: the slot modules are the drum kits' own looks; edit that, not this file."]
    out.lines += ["theme_%s" % kv for kv in THEME.split()] + ["scale_names=1", "qlink_bounds=column"]
    mods = [family_module(out, fam) for fam in range(5)]
    for page in range(4):
        voice_page(out, mods, page)
    fx_page(out)
    rnd_page(out)
    open(os.path.join(PORT, "layout.conf"), "w", newline="\n").write("\n".join(out.lines) + "\n")
    preview(out)
    print("layout.conf + art/ (%d files), preview in %s" % (len(os.listdir(out.art)), out.prev))


if __name__ == "__main__":
    main()
