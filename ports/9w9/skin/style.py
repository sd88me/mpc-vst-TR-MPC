"""9W9's look for tools/drum_skin.py, in the manner of the TR-909 (drawn from scratch: no photo, no maker's name or mark):
an off-white panel, dark slate bars with the voice names in orange capitals, slate column rules, end cheeks, smooth charcoal
plastic knobs with an orange pointer dash. Header in Earth (baked from FONT_SRC, not shipped).

The layout, Q-Links and filmstrips are drum_skin.py's; this file only draws."""
import math

from PIL import Image, ImageDraw, ImageFilter

import drum_skin as ds
from drum_skin import SS, W, H, HEAD, polar

INK = (44, 46, 54)
SLATE = (78, 82, 94)
ORANGE = (240, 120, 40)

STYLE = dict(
    ink=INK, rule=SLATE, tile="bar", bar=(SLATE, ORANGE), names={},
    popup_w=140,      # "Diode (909)", "Wavefolder" need the room
    slot=((62, 66, 76), (30, 32, 38), (250, 249, 244)),
    theme="bg=e2e0d8 ink=2c2e36 ink_dim=5c5f68 accent=f07828 accent_hi=ff9a50 seg_active=4e525e seg_inactive=e8e6de "
          "seg_active_tx=f4f2ea lcd=e8e6de line=4e525e btn_bg=4e525e btn_text=f4f2ea box=3e424c display_ink=f07828",
    # voice prefix -> (small-caps name for the bar, Q-Link page title); 9W9's "hc" is the hand clap
    voices={"bd_c": ("BassDrum", "BASS DRUM"), "sd_c": ("SnareDrum", "SNARE"), "lt_c": ("LowTom", "LOW TOM"),
            "mt_c": ("MidTom", "MID TOM"), "ht_c": ("HiTom", "HI TOM"), "rs": ("RimShot", "RIM SHOT"),
            "hc": ("HandClap", "HAND CLAP"), "chh": ("ClosedHat", "CLOSED HAT"), "ohh": ("OpenHat", "OPEN HAT"),
            "rc": ("Ride", "RIDE"), "cr": ("Crash", "CRASH")},
    tabs=[("BD - MT", ["bd_c", "sd_c", "lt_c", "mt_c"]), ("HT - CH", ["ht_c", "rs", "hc", "chh"]),
          ("OH - CR", ["ohh", "rc", "cr"])],
    fx=[("rev", ["rev_level", "rev_decay", "rev_tone", "rev_hpf"], []),
        ("dly", ["dly_level", "dly_time", "dly_fdbk", "dly_tone"], ["dly_hpf"]),
        ("master", ["volume", "master_drive", "master_dist", "master_comp"], []),
        ("accent", ["accent", "vel_depth"], [])])


SIDE = 20


def panel(t):
    d = t.d
    g = Image.linear_gradient("L").resize((W * SS, H * SS)).point(lambda v: int(232 - 14 * v / 255))
    plate = Image.merge("RGB", (g, g.point(lambda v: v - 2), g.point(lambda v: v - 8)))
    noise = Image.effect_noise((W * SS, H * SS), 8).convert("L")
    t.hi.paste(Image.blend(plate, Image.merge("RGB", (noise,) * 3), 0.02), (0, 0))
    d.rectangle((0, 0, W * SS, 2 * SS), fill=(250, 249, 244))
    d.rectangle((0, (H - 2) * SS, W * SS, H * SS), fill=(150, 148, 140))
    # name and line in Earth, the face nearest the 909's lettering (baked, not shipped)
    t.text(ds.X_START + 24, 38, "9W9", 34, (70, 72, 80), "earth", "ls")
    t.text(W - ds.X_START - 24, 37, "RHYTHM COMPOSER", 20, (92, 94, 102), "earth", "rs")
    d.line([(SIDE * SS, HEAD * SS), ((W - SIDE) * SS, HEAD * SS)], fill=SLATE, width=2 * SS)
    ds.cheeks(t, SIDE, (122, 115, 95), 26)      # the 909's warm khaki grey (~146,139,117), nearly flat


_body = {}


def knob_body(r):
    """What doesn't turn, lit from the top left: a smooth charcoal plastic cylinder, its flat top a shade lighter
    toward the light, a bright edge where the top meets the side on the lit rim and a dark one opposite."""
    if r in _body:
        return _body[r]
    R = r * SS
    s = int(2 * R + 8 * SS)
    c = s / 2
    im = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    side = ds.brushed(int(2 * R), int(2 * R), 26, 64, 0.0)       # the cylinder wall seen from above: lit top left
    m = Image.new("L", side.size, 0)
    ImageDraw.Draw(m).ellipse((0, 0, side.width - 1, side.height - 1), fill=255)
    im.paste(Image.merge("RGBA", (side, side, side.point(lambda v: v + 3), m)), (int(c - R), int(c - R)), m)
    rc = R * 0.84
    top = ds.brushed(int(2 * rc), int(2 * rc), 40, 70, 0.0)      # the flat top
    m = Image.new("L", top.size, 0)
    ImageDraw.Draw(m).ellipse((0, 0, top.width - 1, top.height - 1), fill=255)
    im.paste(Image.merge("RGBA", (top, top, top.point(lambda v: v + 3), m)), (int(c - rc), int(c - rc)), m)
    edge = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    e = ImageDraw.Draw(edge)
    e.arc((c - rc, c - rc, c + rc, c + rc), 190, 300, fill=(150, 150, 156, 200), width=int(1.2 * SS))
    e.arc((c - rc, c - rc, c + rc, c + rc), 10, 120, fill=(8, 8, 10, 220), width=int(1.4 * SS))
    im.alpha_composite(edge.filter(ImageFilter.GaussianBlur(0.5 * SS)))
    ImageDraw.Draw(im).ellipse((c - R, c - R, c + R, c + R), outline=(14, 14, 16, 255), width=SS)
    _body[r] = (im, rc)
    return _body[r]


def knob_sprite(r, level, deg):
    """The knob at deg from the top: the fixed body with the orange pointer dash near the top's rim."""
    body, rc = knob_body(r)
    im = body.copy()
    c = im.width / 2
    x0, y0 = polar(c, c, rc * 0.50, deg)
    x1, y1 = polar(c, c, rc * 0.90, deg)
    ImageDraw.Draw(im).line([(x0, y0), (x1, y1)], fill=ORANGE + (255,), width=int(3.0 * SS))
    return im
