#!/usr/bin/env python3
"""Draws a drum kit's MPC skin for the 1280x628 plugin area in the manner of the machine it ports: a panel drawn from
scratch (no photo, no maker's name or mark), lit from the top left, with lettering baked into the PNGs.

    python3 tools/drum_skin.py 8w8 [6w6 ...]     # -> ports/<kit>/art/*.png, ports/<kit>/layout.conf, ports/<kit>/build/drum_skin/*.png

Each kit's look is its own ports/<kit>/skin/style.py: STYLE (ink, rules, tile style, label names, popup slot,
theme, tabs) plus panel(tab), which draws the plate and header, and knob_sprite(r, level, deg), which draws a knob at an
angle. This file does what must match across kits: placing the controls, the Q-Link columns, cutting the filmstrips
and writing layout.conf.

Layout (approved mockup, 2026-11-02): each voice is one column. Its top 2x2 is LEVEL, TUNE, DECAY, DRIVE: the voice's
Q-Link column on the tab's overview page and column 1 on its own page. Below: DISTORTION, the voice's own controls and
REVERB / DELAY (column 2, and 3 if needed, on its own page). A page where a voice has more than four lower controls uses
five rows instead of four. With qlink_bounds=column MPC outlines the column the Q-Links drive.

Each tab is drawn once at 2x; every knob is a 64-frame filmstrip cut from that tab with the knob turned, so a frame sits
exactly over its place in the background. The light is fixed: only the pointer turns.

Fonts: Helvetica from FONT_SRC (the mpc-vst-plugins Assets/fonts folder; baked, never shipped) for labels, URW Gothic
(tools/assets, AGPL-3.0 with a font exception, URW-LICENSE) and Michroma (tools/assets, SIL OFL 1.1) for header names."""
import json
import math
import os
import shutil
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
FONT_SRC = os.environ.get("FONT_SRC") or next(
    (p for p in (os.path.join(ROOT, "..", "mpc-vst", "Assets", "fonts"), os.path.join(ROOT, "..", "mpc-vst-plugins", "Assets", "fonts"))
     if os.path.isdir(os.path.join(p, "helvetica-255"))), "")
SS = 2
W, H = 1280, 628
Y_OFF = 86                 # layout y = plugin y + 86
FRAMES = 64
SWEEP = 300                # a pot's travel in degrees (-150 .. +150)
PITCH, COLW = 310, 298     # voice column pitch and width
X_START = (W - 4 * PITCH) // 2 + 6
HEAD = 46                  # header height (the rule under it)
LIGHT = -45                # degrees from the top: the top left

SMALLCAPS = {"bd": "BassDrum", "sd": "SnareDrum", "lt": "LowTom", "mt": "MidTom", "ht": "HiTom", "lc": "LowConga",
             "mc": "MidConga", "hc": "HiConga", "rs": "RimShot", "cl": "Claves", "ma": "Maracas", "cp": "HandClap",
             "cb": "CowBell", "ch": "ClosedHihat", "oh": "OpenHihat", "cy": "Cymbal",
             "rev": "Reverb", "dly": "Delay", "master": "Master", "accent": "Accent"}
TITLES = {"bd": "BASS DRUM", "sd": "SNARE", "lt": "LOW TOM", "mt": "MID TOM", "ht": "HI TOM", "lc": "LOW CONGA",
          "mc": "MID CONGA", "hc": "HI CONGA", "rs": "RIM SHOT", "cl": "CLAVES", "ma": "MARACAS", "cp": "HAND CLAP",
          "cb": "COWBELL", "ch": "CLOSED HAT", "oh": "OPEN HAT", "cy": "CYMBAL"}
NAMES = {"level": "LEVEL", "tune": "TUNE", "decay": "DECAY", "drive": "DRIVE", "attack": "ATTACK", "tone": "TONE",
         "snappy": "SNAPPY", "noise": "NOISE", "rate": "RATE", "sweep_depth": "SWEEP", "pitch_mod": "PITCH MOD", "rev": "REVERB", "dly": "DELAY", "dist_type": "DISTORTION", "choke": "CHOKE"}
FX = [("rev", ["rev_level", "rev_decay", "rev_tone", "rev_hpf"], []),
      ("dly", ["dly_level", "dly_time", "dly_fdbk", "dly_tone"], ["dly_hpf"]),
      ("master", ["volume", "master_drive", "master_dist", "comp"], []),
      ("accent", ["vel_depth"], [])]
FX_NAMES = {"rev_level": "LEVEL", "rev_decay": "DECAY", "rev_tone": "TONE", "rev_hpf": "HPF", "dly_level": "LEVEL",
            "dly_time": "TIME", "dly_fdbk": "FEEDBACK", "dly_tone": "TONE", "dly_hpf": "HPF", "volume": "VOLUME",
            "master_drive": "DRIVE", "master_dist": "DISTORTION", "comp": "COMP", "master_comp": "COMP",
            "vel_depth": "VELOCITY", "accent": "ACCENT"}
LEVELS = ("rev_level", "dly_level", "volume")


FONTS = {}


def font(size, face="regular"):
    k = (size, face)
    if k not in FONTS:
        if face in ("logo", "book", "michroma"):   # shipped in tools/assets with their licences
            f = os.path.join(HERE, "assets", {"logo": "URWGothic-Demi.otf", "book": "URWGothic-Book.otf",
                                              "michroma": "Michroma-Regular.ttf"}[face])
        elif face in ("earth", "filmotype"):   # baked from FONT_SRC, never shipped, like Helvetica
            f = os.path.join(FONT_SRC, {"earth": "Earth Normal.ttf", "filmotype": "FilmotypeFord.ttf"}[face])
        else:
            f = os.path.join(FONT_SRC, "helvetica-255", "Helvetica-Bold.ttf" if face == "bold" else "Helvetica.ttf")
            if not os.path.exists(f):
                sys.exit("drum_skin: Helvetica not found under FONT_SRC (%s)" % FONT_SRC)
        FONTS[k] = ImageFont.truetype(f, int(round(size * SS)))
    return FONTS[k]


def polar(cx, cy, r, deg):
    a = math.radians(deg)
    return cx + r * math.sin(a), cy - r * math.cos(a)


def soft_shadow(size, box, blur, alpha, off, ellipse=True):
    m = Image.new("L", size, 0)
    b = (box[0] + off[0] * SS, box[1] + off[1] * SS, box[2] + off[0] * SS, box[3] + off[1] * SS)
    (ImageDraw.Draw(m).ellipse if ellipse else ImageDraw.Draw(m).rectangle)(b, fill=alpha)
    sh = Image.new("RGBA", size, (0, 0, 0, 0))
    sh.putalpha(m.filter(ImageFilter.GaussianBlur(blur * SS)))
    return sh


def brushed(w, h, lo, hi, grain=0.10):
    """Brushed metal, L: a top-left to bottom-right fall from hi to lo, with horizontal streaks."""
    gy = Image.linear_gradient("L").resize((w, h))
    gx = Image.linear_gradient("L").transpose(Image.Transpose.TRANSPOSE).resize((w, h))
    g = Image.blend(gx, gy, 0.6).point(lambda v: int(hi + (lo - hi) * v / 255))
    streak = Image.effect_noise((max(8, w // 60), h), 40).resize((w, h), Image.BILINEAR).filter(ImageFilter.GaussianBlur(0.6))
    return Image.blend(g, streak, grain)


def screw(t, x, y, r=7):
    """A black domed Phillips screw, lit top left (for a style's panel())."""
    t.hi.alpha_composite(soft_shadow(t.hi.size, ((x - r) * SS, (y - r) * SS, (x + r) * SS, (y + r) * SS), 1.5, 200, (1, 2)))
    d = t.d
    X, Y, R = x * SS, y * SS, r * SS
    for i in range(R, 0, -1):
        f = i / R
        off = (1 - f) * R * 0.3
        v = int(18 + 46 * (1 - f))
        d.ellipse((X - i - off, Y - i - off, X + i - off, Y + i - off), fill=(v, v, v))
    for a in (40, 130):
        c, s = math.cos(math.radians(a)) * R * 0.6, math.sin(math.radians(a)) * R * 0.6
        d.line([(X - c, Y - s), (X + c, Y + s)], fill=(4, 4, 4), width=int(2.4 * SS))


def cheeks(t, side, base=(10, 10, 20), span=38):
    """Rounded end cheeks down both sides, lit across their curve, with a dark joint against the plate."""
    d = t.d
    for xa, xb in ((0, side), (W - side, W)):
        for xx in range(xa * SS, xb * SS):
            f = (xx - xa * SS) / ((xb - xa) * SS)
            v = int(span * math.sin(f * math.pi) ** 0.6)
            d.line([(xx, 0), (xx, H * SS)], fill=(base[0] + v, base[1] + v, base[2] + v))
        j = xb if xa == 0 else xa
        d.line([(j * SS, 0), (j * SS, H * SS)], fill=(8, 8, 10), width=2 * SS)


def load_style(kit):
    """ports/<kit>/skin/style.py: STYLE (colours, tabs, theme) plus panel(tab) and knob_sprite(r, level, deg)."""
    import importlib.util
    path = os.path.join(ROOT, "ports", kit, "skin", "style.py")
    spec = importlib.util.spec_from_file_location("style_" + kit, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return dict(mod.STYLE, module=mod)


# ---------------------------------------------------------------- the panel

class Tab:
    def __init__(self, st, name, compact=False):
        self.st, self.name = st, name
        self.rows = [160, 264, 368, 472, 576] if compact else [180, 304, 428, 552]   # spread evenly down to the panel's foot
        self.r = 24 if compact else 27              # knob radius and label size: as big as the row spacing allows
        self.label_size = 16 if compact else 17
        self.label_dy = -(self.r + 24) if compact else -(self.r + 30)
        self.hi = Image.new("RGBA", (W * SS, H * SS), (0, 0, 0, 255))
        self.d = ImageDraw.Draw(self.hi)
        self.knobs, self.lines, self.qlinks = [], [], []
        st["module"].panel(self)

    def text(self, x, y, s, size, fill=None, face="regular", anchor="mm", stroke=0):
        fill = fill or self.st["ink"]
        self.d.text((x * SS, y * SS), s, font=font(size, face), fill=fill, anchor=anchor,
                    stroke_width=int(stroke * SS), stroke_fill=fill)


    def column(self, i):
        x0 = X_START + i * PITCH
        if i:
            self.d.line([((x0 - 6) * SS, HEAD * SS), ((x0 - 6) * SS, 610 * SS)], fill=self.st["rule"], width=2 * SS)
        return x0, x0 + 80, x0 + 218

    def tile(self, x0, name):
        """The voice's name in small capitals: on a cream tile (808) or straight on the metal (606)."""
        d = self.d
        x1, y0, y1 = x0 + COLW - 20, 58, 90
        x0 += 20
        if hasattr(self.st["module"], "tile"):   # the kit draws its own headings
            self.st["module"].tile(self, x0, x1, y0, y1, name)
            return
        if self.st["tile"] == "label":   # the name in coloured capitals between rules of the same colour (CR-78)
            ink = self.st["label_ink"]
            t = name.replace("'", "")
            words = "".join((" " + c if c.isupper() and i else c) for i, c in enumerate(t)).upper()
            f = font(18, "bold")
            w = d.textlength(words, font=f) / SS
            cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
            for a, b in ((x0 - 10, cx - w / 2 - 12), (cx + w / 2 + 12, x1 + 10)):
                d.line([(a * SS, cy * SS), (b * SS, cy * SS)], fill=ink, width=int(2.5 * SS))
            d.text((cx * SS, cy * SS), words, font=f, fill=ink, anchor="mm")
            return
        if self.st["tile"] == "bar":   # a dark bar across the column, the name in coloured capitals
            bar, ink = self.st["bar"]
            d.rectangle(((x0 - 18) * SS, (y0 + 4) * SS, (x1 + 18) * SS, (y1 - 4) * SS), fill=bar)
            t = name.replace("'", "")
            words = "".join((" " + c if c.isupper() and i else c) for i, c in enumerate(t)).upper()
            d.text(((x0 + x1) / 2 * SS, (y0 + y1) / 2 * SS), words, font=font(17, "bold"), fill=ink, anchor="mm")
            return
        if self.st["tile"] == "tile":
            self.hi.alpha_composite(soft_shadow(self.hi.size, (x0 * SS, y0 * SS, x1 * SS, y1 * SS), 1.5, 150, (1, 2), False))
            d.rounded_rectangle((x0 * SS, y0 * SS, x1 * SS, y1 * SS), 3 * SS, fill=(238, 232, 196))
            d.line([(x0 * SS + 3 * SS, y0 * SS + SS), (x1 * SS - 3 * SS, y0 * SS + SS)], fill=(252, 250, 232), width=SS)
            ink, big, small = (24, 24, 24), 19, 14
        else:
            ink, big, small = self.st["ink"], 22, 16
        parts = []
        for ch in name:
            b = ch.isupper() or ch == "'"
            if parts and parts[-1][1] == b:
                parts[-1][0] += ch
            else:
                parts.append([ch, b])
        runs = [(s if b else s.upper(), font(big if b else small, "bold")) for s, b in parts]
        tw = sum(d.textlength(s, font=f) for s, f in runs)
        x, base = (x0 + x1) / 2 * SS - tw / 2, (y1 - 8) * SS
        for s, f in runs:
            d.text((x, base), s, font=f, fill=ink, anchor="ls")
            x += d.textlength(s, font=f)

    def ticks(self, cx, cy, r):
        for i in range(11):
            a = -150 + 30 * i
            x0, y0 = polar(cx * SS, cy * SS, (r + 6) * SS, a)
            x1, y1 = polar(cx * SS, cy * SS, (r + 12) * SS, a)
            self.d.line([(x0, y0), (x1, y1)], fill=self.st["ink"], width=2 * SS)

    def knob(self, key, cx, cy, name, level=False):
        r = self.r
        self.text(cx, cy + self.label_dy, name, self.label_size)
        self.ticks(cx, cy, r)
        box = ((cx - r) * SS, (cy - r) * SS, (cx + r) * SS, (cy + r) * SS)
        self.hi.alpha_composite(soft_shadow(self.hi.size, box, 4, 170, (3, 5)))
        self.hi.alpha_composite(soft_shadow(self.hi.size, box, 1, 180, (1, 1)))
        self.knobs.append(dict(key=key, cx=cx, cy=cy, level=level, r=r))

    def popup(self, key, cx, cy, name):
        self.text(cx, cy - 38, name, self.label_size)
        fill, top, bottom = self.st["slot"]
        pw = self.st.get("popup_w", 112)                         # wider for long option names
        x0, y0, x1, y1 = cx - pw // 2 - 4, cy - 20, cx + pw // 2 + 4, cy + 20     # a recessed slot the live field sits in
        self.d.rounded_rectangle((x0 * SS, y0 * SS, x1 * SS, y1 * SS), 4 * SS, fill=fill)
        self.d.line([(x0 * SS, y0 * SS), (x1 * SS, y0 * SS)], fill=top, width=2 * SS)
        self.d.line([(x0 * SS, y1 * SS), (x1 * SS, y1 * SS)], fill=bottom, width=SS)
        self.lines.append('popup cx=%d cy=%d w=%d h=36 label="" key=%s' % (cx, cy + Y_OFF, pw, key))


# ---------------------------------------------------------------- output

def final(im, w, h):
    return im.convert("RGB").resize((w, h), Image.LANCZOS)


def default_index(p, n):
    if "options" in p:
        return 0
    lo, hi = p.get("min", 0), p.get("max", 127)
    return int(round((p.get("default", lo) - lo) / ((hi - lo) or 1) * (n - 1)))


def render(t, art, prev_dir, params):
    bg = final(t.hi, W, H)
    prev = bg.convert("RGBA")
    for k in t.knobs:
        side = 2 * k["r"] + 8
        strip = Image.new("RGB", (side, side * FRAMES))
        x0, y0 = int((k["cx"] - side / 2) * SS), int((k["cy"] - side / 2) * SS)
        frames = []
        for i in range(FRAMES):
            deg = -SWEEP / 2 + SWEEP * i / (FRAMES - 1)
            bgc = t.hi.crop((x0, y0, x0 + side * SS, y0 + side * SS))
            sp = t.st["module"].knob_sprite(k["r"], k["level"], deg)
            bgc.alpha_composite(sp, ((bgc.width - sp.width) // 2, (bgc.height - sp.height) // 2))
            f = final(bgc, side, side)
            frames.append(f)
            strip.paste(f, (0, i * side))
        strip.save(os.path.join(art, "%s.png" % k["key"]), optimize=True)
        di = default_index(params.get(k["key"], {}), FRAMES)
        prev.alpha_composite(frames[di].convert("RGBA"), (int(k["cx"] - side / 2), int(k["cy"] - side / 2)))
        t.lines.append('knob cx=%d cy=%d r=%d label="" key=%s strip=art/%s.png frames=%d ns=0 vs=0 bw=100' %
                       (k["cx"], k["cy"] + Y_OFF, k["r"], k["key"], k["key"], FRAMES))
    slug = t.name.lower().replace(" ", "").replace("/", "_").replace("-", "_")
    bg.save(os.path.join(art, "tab_%s.png" % slug), optimize=True)
    prev.convert("RGB").save(os.path.join(prev_dir, "tab_%s.png" % slug))
    return "art/tab_%s.png" % slug


# The top 2x2's roles and the parameter suffixes that fill them, first found wins (9W9 calls a hat's tune "pitch",
# its level "volume", the rim shot's drive "saturation" and the snare's decay "noise_decay")
ROLES = [("level", ("level", "volume")), ("tune", ("tune", "pitch")), ("decay", ("decay", "noise_decay")),
         ("drive", ("drive", "saturation"))]


def voice_plan(v, params):
    """(top 2x2 keys, with "-" for a role the voice lacks; lower keys before the sends; the sends) for voice prefix v."""
    keys = [k for k in params if k.startswith(v + "_")] + (["hh_choke"] if v == "ch" and "hh_choke" in params else [])
    top = [next(("%s_%s" % (v, c) for c in cands if "%s_%s" % (v, c) in params), "-") for _, cands in ROLES]
    sends = [k for k in ("%s_rev" % v, "%s_dly" % v) if k in params]
    own = [k for k in keys if k not in top and k not in sends and k != "%s_dist_type" % v]
    return top, ["%s_dist_type" % v] + own, sends


def voice_tab(st, name, voices, params):
    """voices: voice prefixes, or (tile, top, lower, title) for a plain column (plain_column) on the same page."""
    plans = {v: voice_plan(v, params) for v in voices if isinstance(v, str)}
    compact = any(len(a) + len(s) > 4 for _, a, s in plans.values())
    t = Tab(st, name, compact)
    names = dict(NAMES, **st["names"])
    vnames = st.get("voices", {})
    overview = []
    for i, v in enumerate(voices):
        if not isinstance(v, str):
            tile, top, lower, title = v
            plain_column(t, i, tile, top, lower, params)
            overview += top + ["-"] * (4 - len(top))
            t.qlinks.append((title, top + lower))
            continue
        x0, ca, cb = t.column(i)
        t.tile(x0, vnames.get(v, (SMALLCAPS.get(v),))[0])
        top, lead, sends = plans[v]
        for (role, _), key, cx, cy in zip(ROLES, top, (ca, cb, ca, cb), (t.rows[0], t.rows[0], t.rows[1], t.rows[1])):
            if key != "-":
                t.knob(key, cx, cy, names[role], role == "level")
        # lower: DISTORTION top left, REVERB / DELAY on the bottom row, the voice's own controls in the cells left over
        cells = [(x, y) for y in t.rows[2:] for x in (ca, cb)]
        place = {}
        if sends:
            place.update({sends[0]: cells[-2], sends[-1]: cells[-1]})
        free = [c for c in cells if c not in place.values()]
        place.update(zip(lead, free))
        lower = sorted(place, key=lambda k: (place[k][1], place[k][0]))     # reading order: Q-Links follow the panel
        for key in lower:
            cx, cy = place[key]
            short = "choke" if key == "hh_choke" else key[len(v) + 1:]
            if params[key].get("type") == "enum":
                t.popup(key, cx, cy, names[short])
            else:
                t.knob(key, cx, cy, names[short])
        overview += top
        t.qlinks.append((vnames.get(v, (None, TITLES.get(v)))[1], top + lower))
    if len(voices) < 4:   # close the last column with a panel line
        t.column(len(voices))
    t.qlinks.insert(0, (name, overview))
    return t


def plain_column(t, i, tile, top, lower, params):
    """A column of named controls (FX, master, a kit's rhythm section): top is its Q-Link 2x2, lower goes under it."""
    x0, ca, cb = t.column(i)
    t.tile(x0, SMALLCAPS.get(tile, tile))
    r0, r1, r2 = t.rows[:3]
    names = dict(FX_NAMES, **t.st.get("fx_names", {}))
    for key, (cx, cy) in zip(top + lower, [(ca, r0), (cb, r0), (ca, r1), (cb, r1), (ca, r2), (cb, r2)]):
        if params[key].get("type") == "enum":
            t.popup(key, cx, cy + (8 if cy < r2 else 0), names[key])
        else:
            t.knob(key, cx, cy, names[key], key in LEVELS)


def fx_tab(st, params):
    t = Tab(st, "FX/MASTER")
    ql = []
    for i, (tile, top, lower) in enumerate(st.get("fx", FX)):
        plain_column(t, i, tile, top, lower, params)
        ql += top + ["-"] * (4 - len(top))
    while ql and ql[-1] == "-":
        ql.pop()
    t.qlinks.append(("FX/MASTER", ql))
    return t


def build(kit):
    st = load_style(kit)
    port = os.path.join(ROOT, "ports", kit)
    params = {p["key"]: p for p in json.load(open(os.path.join(port, "module.mpc.json")))["chain_params"]}
    art = os.path.join(port, "art")
    prev = os.path.join(port, "build", "drum_skin")
    shutil.rmtree(art, ignore_errors=True)
    os.makedirs(art)
    os.makedirs(prev, exist_ok=True)
    out = ["# generated by tools/drum_skin.py %s: the panel is drawn there; edit that, not this file." % kit]
    out += ["theme_%s" % kv for kv in st["theme"].split()] + ["scale_names=1", "qlink_bounds=column"]
    for make in [lambda n=n, v=v: voice_tab(st, n, v, params) for n, v in st["tabs"]] + [lambda: fx_tab(st, params)]:
        t = make()
        bgfile = render(t, art, prev, params)
        out += ["", "[tab %s]" % t.name, "art file=%s fit=stretch" % bgfile] + t.lines
        out += ['qlinks "%s" = %s' % (n, ",".join(ks)) for n, ks in t.qlinks]
    open(os.path.join(port, "layout.conf"), "w", newline="\n").write("\n".join(out) + "\n")
    print("%s: layout.conf + art/ (%d tabs), previews in %s" % (kit, len(st["tabs"]) + 1, prev))


if __name__ == "__main__":
    for kit in sys.argv[1:] or sorted(k for k in os.listdir(os.path.join(ROOT, "ports")) if os.path.exists(os.path.join(ROOT, "ports", k, "skin", "style.py"))):
        build(kit)
