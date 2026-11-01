"""8W8's look for tools/drum_skin.py, in the manner of the TR-808 (drawn from scratch: no photo, no maker's name or mark):
a charcoal plate with column rules, cream small-caps voice tiles, black knurled knobs with an orange LEVEL cap and white
caps, the four-colour bar, dark end cheeks and black Phillips screws. Header in URW Gothic.

The layout, Q-Links and filmstrips are drum_skin.py's; this file only draws."""
import math

from PIL import Image, ImageDraw, ImageFilter

import drum_skin as ds
from drum_skin import SS, W, H, HEAD, polar

SIDE = 22
ORANGE = (232, 98, 40)
CAP_LEVEL = (246, 170, 40)
CAP = (236, 236, 228)
BAR = [(231, 46, 46), (248, 161, 37), (241, 248, 39), (240, 240, 236)]

STYLE = dict(
    ink=(222, 222, 214), rule=(150, 150, 144), tile="tile", names={"tune": "TUNING"},
    slot=((30, 30, 29), (12, 12, 12), (96, 96, 92)),
    theme="bg=3a3a38 ink=dedcd6 ink_dim=9a9a94 accent=f0aa3c accent_hi=f8c060 seg_active=f0aa3c seg_inactive=1e1e1d "
          "seg_active_tx=1e1e1d lcd=1e1e1d line=96968f btn_bg=1e1e1d btn_text=dedcd6 box=1e1e1d display_ink=f0aa3c",
    tabs=[("BD - MT", ["bd", "sd", "lt", "mt"]), ("HT - HC", ["ht", "lc", "mc", "hc"]),
          ("RS - CP", ["rs", "cl", "ma", "cp"]), ("CB - CY", ["cb", "ch", "oh", "cy"])])


def panel(t):
    d = t.d
    # charcoal, a little darker toward the bottom, with a faint grain
    g = Image.linear_gradient("L").resize((W * SS, H * SS)).point(lambda v: int(60 - 10 * v / 255))
    plate = Image.merge("RGB", (g, g, g.point(lambda v: max(0, v - 2))))
    noise = Image.effect_noise((W * SS, H * SS), 9).convert("L")
    t.hi.paste(Image.blend(plate, Image.merge("RGB", (noise,) * 3), 0.035), (0, 0))
    d.rectangle((0, 0, W * SS, 2 * SS), fill=(96, 96, 94))
    d.rectangle((0, (H - 2) * SS, W * SS, H * SS), fill=(26, 26, 26))
    x = SIDE + 30
    for s, size, fill, face in (("8W8", 32, ORANGE, "logo"), ("Rhythm Composer", 26, ORANGE, "logo"),
                                ("Computer Controlled", 14, (205, 205, 200), "regular")):
        t.text(x, 25, s, size, fill, face, "lm")
        x += ds.font(size, face).getlength(s) / SS + 22
    for i, c in enumerate(BAR):
        d.rectangle(((950 + i * 66) * SS, 18 * SS, (1010 + i * 66) * SS, 30 * SS), fill=c)
    d.line([(SIDE * SS, HEAD * SS), ((W - SIDE) * SS, HEAD * SS)], fill=STYLE["rule"], width=2 * SS)
    for xa, xb in ((0, SIDE), (W - SIDE, W)):   # end cheeks: rounded, a navy cast, a dark joint against the plate
        for xx in range(xa * SS, xb * SS):
            f = (xx - xa * SS) / ((xb - xa) * SS)
            v = int(10 + 38 * math.sin(f * math.pi) ** 0.6)
            d.line([(xx, 0), (xx, H * SS)], fill=(v, v, v + 10))
        j = xb if xa == 0 else xa
        d.line([(j * SS, 0), (j * SS, H * SS)], fill=(8, 8, 10), width=2 * SS)
    for sx, sy in ((SIDE + 14, 12), (W - SIDE - 14, 12), (SIDE + 14, H - 15), (W - SIDE - 14, H - 15), (W // 2, 12)):
        ds.screw(t, sx, sy)


_body = {}


def knob_body(r, cap):
    """What doesn't turn, lit from the top left: the black knurled skirt and the domed coloured cap."""
    k = (r, cap)
    if k in _body:
        return _body[k]
    R = r * SS
    s = int(2 * R + 8 * SS)
    c = s / 2
    im = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse((c - R, c - R, c + R, c + R), fill=(12, 12, 12, 255))
    for i in range(48):
        a = 360 * i / 48
        lit = max(0.0, math.cos(math.radians(a - ds.LIGHT)))
        x0, y0 = polar(c, c, R * 0.70, a)
        x1, y1 = polar(c, c, R * 0.97, a)
        v = int(26 + 34 * lit)
        d.line([(x0, y0), (x1, y1)], fill=(v, v, v, 255), width=max(1, int(2.0 * SS)))
    rim = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    ImageDraw.Draw(rim).arc((c - R + SS, c - R + SS, c + R - SS, c + R - SS), 200, 290, fill=(200, 200, 200, 120), width=int(1.4 * SS))
    im.alpha_composite(rim.filter(ImageFilter.GaussianBlur(0.8 * SS)))
    rc = R * 0.56
    st = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    ImageDraw.Draw(st).ellipse((c - rc + SS, c - rc + 2 * SS, c + rc + SS, c + rc + 2 * SS), fill=(0, 0, 0, 200))
    im.alpha_composite(st.filter(ImageFilter.GaussianBlur(1.5 * SS)))
    d = ImageDraw.Draw(im)
    for i in range(int(rc), 0, -1):    # dome, brightest toward the light
        f = i / rc
        off = (1 - f) * rc * 0.3
        k2 = 0.74 + 0.34 * (1 - f)
        d.ellipse((c - i - off, c - i - off, c + i - off, c + i - off), fill=tuple(min(255, int(v * k2)) for v in cap) + (255,))
    d.ellipse((c - rc, c - rc, c + rc, c + rc), outline=tuple(int(v * 0.55) for v in cap) + (255,), width=SS)
    _body[k] = (im, rc)
    return _body[k]


def knob_sprite(r, level, deg):
    """The knob at deg from the top: the fixed body with the cap's pointer dot."""
    body, rc = knob_body(r, CAP_LEVEL if level else CAP)
    im = body.copy()
    c = im.width / 2
    x, y = polar(c, c, rc * 0.62, deg)
    ImageDraw.Draw(im).ellipse((x - 3.2 * SS, y - 3.2 * SS, x + 3.2 * SS, y + 3.2 * SS), fill=(34, 24, 18, 255))
    return im
