"""CW-78's look for tools/drum_skin.py, in the manner of the CR-78 CompuRhythm (drawn from scratch: no photo, no maker's
name or mark): a black panel with thin light-grey rules, white lettering, section names in orange capitals between orange
rules under them and four little coloured keys beside them (as the earlier CW-78 skin), black knurled knobs with a red
pointer line and a walnut case all round.
"CompuRhythm" and "CW-78" in FilmotypeFord, as the earlier CW-78 skin set them (baked from FONT_SRC, not shipped).

The rhythm section (mode, A/B and the two preset rhythms) is a column on the GU - MB page, next to the voices.

The layout, Q-Links and filmstrips are drum_skin.py's; this file only draws."""
import math

from PIL import Image, ImageDraw, ImageFilter

import drum_skin as ds
from drum_skin import SS, W, H, HEAD, polar

INK = (236, 234, 226)
RULE = (150, 150, 146)
ORANGE = (240, 112, 40)
RED = (214, 40, 34)
SIDE = 22
TOP, BOTTOM = 5, 8             # the case's top and bottom rails
KEYS = [(214, 40, 34), (226, 220, 196), (236, 214, 60), (52, 92, 214)]   # the voice headings' four little keys

STYLE = dict(
    ink=INK, rule=RULE, tile="label", label_ink=ORANGE, names={},
    slot=((14, 14, 16), (0, 0, 0), (84, 84, 82)),
    theme="bg=1c1d21 ink=ecd5aa ink_dim=9c9378 accent=ff5a00 accent_hi=ff8a3d seg_active=ff5a00 seg_inactive=2a2b30 "
          "seg_active_tx=1c1d21 lcd=26272c line=7a735f btn_bg=2a2b30 btn_text=ecd5aa box=121316 display_ink=ff7a30",
    voices={"bd": ("BassDrum", "BASS DRUM"), "sd": ("SnareDrum", "SNARE"), "rs": ("RimShot", "RIM SHOT"),
            "hh": ("HiHat", "HI-HAT"), "cy": ("Cymbal", "CYMBAL"), "ma": ("Maracas", "MARACAS"), "cl": ("Claves", "CLAVES"),
            "hb": ("HiBongo", "HI BONGO"), "lb": ("LowBongo", "LOW BONGO"), "lc": ("LowConga", "LOW CONGA"),
            "cb": ("CowBell", "COWBELL"), "tb": ("Tambourine", "TAMBOURINE"), "gu": ("Guiro", "GUIRO"),
            "mb": ("MetalBeat", "METAL BEAT")},
    tabs=[("BD - HH", ["bd", "sd", "rs", "hh"]), ("CY - HB", ["cy", "ma", "cl", "hb"]),
          ("LB - TB", ["lb", "lc", "cb", "tb"]),
          ("GU - MB", ["gu", "mb", ("Rhythm", ["rhy_mode", "rhy_ab", "rhy_style", "rhy_style2"], [], "RHYTHM")])],
    fx=[("rev", ["rev_level", "rev_decay", "rev_tone", "rev_hpf"], []),
        ("dly", ["dly_level", "dly_time", "dly_fdbk", "dly_tone"], ["dly_hpf"]),
        ("master", ["volume", "master_drive", "master_dist", "comp"], []),
        ("accent", ["vel_depth", "hat_choke"], [])],
    fx_names={"rhy_mode": "MODE", "rhy_ab": "A / B", "rhy_style": "RHYTHM 1", "rhy_style2": "RHYTHM 2", "hat_choke": "CHOKE"})


def tile(t, x0, x1, y0, y1, name):
    """A voice heading as the earlier CW-78 skin had it: the name in orange capitals at the left, four little coloured
    keys (red, cream, yellow, blue, as the CR-78's rhythm buttons) at the right, an orange rule under both."""
    d = t.d
    words = "".join((" " + c if c.isupper() and i else c) for i, c in enumerate(name.replace("'", ""))).upper()
    cy = (y0 + y1) / 2 - 2
    t.text(x0 - 10, cy, words, 18, ORANGE, "bold", "lm")
    kw, kh, gap = 18, 9, 5
    kx = x1 + 10 - 4 * kw - 3 * gap
    for i, c in enumerate(KEYS):
        a = kx + i * (kw + gap)
        box = (a * SS, (cy - kh / 2) * SS, (a + kw) * SS, (cy + kh / 2) * SS)
        t.hi.alpha_composite(ds.soft_shadow(t.hi.size, box, 1, 170, (1, 1.5), False))
        d.rounded_rectangle(box, int(1.5 * SS), fill=c)
        hi = tuple(min(255, v + 50) for v in c)        # lit top edge
        d.line([((a + 1.5) * SS, (cy - kh / 2 + 1) * SS), ((a + kw - 1.5) * SS, (cy - kh / 2 + 1) * SS)], fill=hi, width=SS)
    d.line([((x0 - 10) * SS, (y1 + 1) * SS), ((x1 + 10) * SS, (y1 + 1) * SS)], fill=ORANGE, width=int(1.5 * SS))


def wood(w, h):
    """Walnut, RGB: a warm brown with long vertical grain and a darker figure."""
    grain = Image.effect_noise((max(4, w // 6), max(8, h // 40)), 60).resize((w, h), Image.BICUBIC).filter(ImageFilter.GaussianBlur(1))
    fine = Image.effect_noise((max(4, w // 2), max(8, h // 12)), 40).resize((w, h), Image.BICUBIC)
    g = Image.blend(grain, fine, 0.4)
    r = g.point(lambda v: int(84 + 0.30 * (v - 128)))
    gg = g.point(lambda v: int(42 + 0.18 * (v - 128)))
    b = g.point(lambda v: int(24 + 0.10 * (v - 128)))
    return Image.merge("RGB", (r, gg, b))


def panel(t):
    d = t.d
    g = Image.linear_gradient("L").resize((W * SS, H * SS)).point(lambda v: int(30 - 8 * v / 255))
    plate = Image.merge("RGB", (g, g, g.point(lambda v: v + 2)))
    noise = Image.effect_noise((W * SS, H * SS), 8).convert("L")
    t.hi.paste(Image.blend(plate, Image.merge("RGB", (noise,) * 3), 0.025), (0, 0))
    d.rectangle((0, 0, W * SS, 2 * SS), fill=(70, 70, 72))
    # name and model in FilmotypeFord, orange, on one baseline
    x = SIDE + 28
    for s, size in (("CompuRhythm", 30), ("CW-78", 30)):
        t.text(x, 36, s, size, ORANGE, "filmotype", "ls")
        x += ds.font(size, "filmotype").getlength(s) / SS + 26
    d.line([(SIDE * SS, HEAD * SS), ((W - SIDE) * SS, HEAD * SS)], fill=RULE, width=2 * SS)
    # the walnut case all round: thin bands top and bottom, cheeks down the sides rounded by a soft shade
    for y0, y1 in ((0, TOP), (H - BOTTOM, H)):
        band = wood(W * SS, (y1 - y0) * SS).transpose(Image.Transpose.ROTATE_90).resize((W * SS, (y1 - y0) * SS))
        t.hi.paste(band, (0, y0 * SS))
        j = y1 if y0 == 0 else y0
        d.line([(0, j * SS), (W * SS, j * SS)], fill=(6, 6, 6), width=2 * SS)
    for xa in (0, W - SIDE):
        wd = wood(SIDE * SS, H * SS)
        shade = Image.new("L", wd.size)
        for xx in range(wd.width):
            f = xx / wd.width
            ImageDraw.Draw(shade).line([(xx, 0), (xx, wd.height)], fill=int(255 * (0.62 + 0.38 * math.sin(f * math.pi))))
        wd = Image.composite(wd, Image.new("RGB", wd.size, (0, 0, 0)), shade)
        t.hi.paste(wd, (xa * SS, 0))
        j = SIDE if xa == 0 else W - SIDE
        d.line([(j * SS, 0), (j * SS, H * SS)], fill=(6, 6, 6), width=2 * SS)


_body = {}


def knob_body(r):
    """What doesn't turn, lit from the top left: a black knurled skirt and a black cap with a soft sheen."""
    if r in _body:
        return _body[r]
    R = r * SS
    s = int(2 * R + 8 * SS)
    c = s / 2
    im = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse((c - R, c - R, c + R, c + R), fill=(10, 10, 11, 255))
    for i in range(56):
        a = 360 * i / 56
        lit = max(0.0, math.cos(math.radians(a - ds.LIGHT)))
        x0, y0 = polar(c, c, R * 0.66, a)
        x1, y1 = polar(c, c, R * 0.97, a)
        v = int(30 + 46 * lit)
        d.line([(x0, y0), (x1, y1)], fill=(v, v, v, 255), width=max(1, int(1.7 * SS)))
    rim = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    ImageDraw.Draw(rim).arc((c - R + SS, c - R + SS, c + R - SS, c + R - SS), 200, 290, fill=(210, 210, 214, 120), width=int(1.4 * SS))
    im.alpha_composite(rim.filter(ImageFilter.GaussianBlur(0.8 * SS)))
    rc = R * 0.62
    st = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    ImageDraw.Draw(st).ellipse((c - rc + SS, c - rc + 2 * SS, c + rc + SS, c + rc + 2 * SS), fill=(0, 0, 0, 200))
    im.alpha_composite(st.filter(ImageFilter.GaussianBlur(1.5 * SS)))
    top = ds.brushed(int(2 * rc), int(2 * rc), 14, 52, 0.0)
    m = Image.new("L", top.size, 0)
    ImageDraw.Draw(m).ellipse((0, 0, top.width - 1, top.height - 1), fill=255)
    im.paste(Image.merge("RGBA", (top, top, top.point(lambda v: v + 2), m)), (int(c - rc), int(c - rc)), m)
    ImageDraw.Draw(im).ellipse((c - rc, c - rc, c + rc, c + rc), outline=(4, 4, 4, 255), width=SS)
    _body[r] = (im, rc)
    return _body[r]


def knob_sprite(r, level, deg):
    """The knob at deg from the top: the fixed body with a red pointer line from the cap's centre across the skirt."""
    body, rc = knob_body(r)
    im = body.copy()
    c = im.width / 2
    x0, y0 = polar(c, c, rc * 0.15, deg)
    x1, y1 = polar(c, c, r * SS * 0.95, deg)
    ImageDraw.Draw(im).line([(x0, y0), (x1, y1)], fill=RED + (255,), width=int(2.6 * SS))
    return im
