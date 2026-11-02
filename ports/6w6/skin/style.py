"""6W6's look for tools/drum_skin.py, in the manner of the TR-606 (drawn from scratch: no photo, no maker's name or mark):
brushed aluminium, black lettering and small-caps voice names straight on the metal, black column rules, aluminium
knurled knobs whose grey caps carry the 303/606 moulded facet. No end cheeks or screws, as on the 606. Header: URW
Gothic, with "Drumatix" in Michroma.

The layout, Q-Links and filmstrips are drum_skin.py's; this file only draws."""
import math

from PIL import Image, ImageChops, ImageDraw, ImageFilter

import drum_skin as ds
from drum_skin import SS, W, H, HEAD, polar

INK = (26, 28, 34)

STYLE = dict(
    ink=INK, rule=(40, 42, 48), tile="text", names={},
    slot=((38, 40, 46), (14, 14, 16), (236, 236, 232)),
    theme="bg=c4c5c0 ink=1a1c22 ink_dim=4c4f55 accent=dd0000 accent_hi=ff3b30 seg_active=1a1c22 seg_inactive=26282e "
          "seg_active_tx=e4e4e0 lcd=d9dad4 line=282a30 btn_bg=26282e btn_text=e4e4e0 box=26282e display_ink=ff3b30",
    tabs=[("BD - HT", ["bd", "sd", "lt", "ht"]), ("CH - CP", ["ch", "oh", "cy", "cp"])])


def panel(t):
    d = t.d
    g = ds.brushed(W * SS, H * SS, 178, 214)
    t.hi.paste(Image.merge("RGB", (g, g, g.point(lambda v: min(255, v + 3)))), (0, 0))
    d.rectangle((0, 0, W * SS, 2 * SS), fill=(246, 246, 242))       # top edge catches the light
    d.rectangle((0, (H - 2) * SS, W * SS, H * SS), fill=(110, 110, 108))
    x = ds.X_START + 24
    # "Drumatix" in Michroma, emboldened a little: the free face nearest the 606's wide geometric wordmark
    for s, size, face, stroke in (("6W6", 32, "logo", 0), ("Drumatix", 26, "michroma", 0.6), ("Computer Controlled", 17, "book", 0)):
        t.text(x, 36, s, size, INK, face, "ls", stroke)      # one baseline: the faces' boxes differ
        x += ds.font(size, face).getlength(s) / SS + 2 * stroke + 22
    d.line([(8 * SS, HEAD * SS), ((W - 8) * SS, HEAD * SS)], fill=STYLE["rule"], width=2 * SS)


_body = {}


def knob_body(r):
    """What doesn't turn, lit from the top left: the aluminium knurled skirt and the round grey cap."""
    if r in _body:
        return _body[r]
    R = r * SS
    s = int(2 * R + 8 * SS)
    c = s / 2
    im = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse((c - R, c - R, c + R, c + R), fill=(92, 94, 96, 255))
    for i in range(64):
        a = 360 * i / 64
        lit = max(0.0, math.cos(math.radians(a - ds.LIGHT)))
        x0, y0 = polar(c, c, R * 0.62, a)
        x1, y1 = polar(c, c, R * 0.97, a)
        v = int(118 + 110 * lit)
        d.line([(x0, y0), (x1, y1)], fill=(v, v, v + 2, 255), width=max(1, int(1.6 * SS)))
    rim = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    ImageDraw.Draw(rim).arc((c - R + SS, c - R + SS, c + R - SS, c + R - SS), 200, 290, fill=(255, 255, 255, 140), width=int(1.4 * SS))
    im.alpha_composite(rim.filter(ImageFilter.GaussianBlur(0.8 * SS)))
    rc = R * 0.60
    st = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    ImageDraw.Draw(st).ellipse((c - rc + SS, c - rc + 2 * SS, c + rc + SS, c + rc + 2 * SS), fill=(0, 0, 0, 160))
    im.alpha_composite(st.filter(ImageFilter.GaussianBlur(1.5 * SS)))
    top = ds.brushed(int(2 * rc), int(2 * rc), 120, 196, 0.0)     # the cap: a soft diagonal sheen
    m = Image.new("L", top.size, 0)
    ImageDraw.Draw(m).ellipse((0, 0, top.width - 1, top.height - 1), fill=255)
    im.paste(Image.merge("RGBA", (top, top, top.point(lambda v: min(255, v + 4)), m)), (int(c - rc), int(c - rc)), m)
    ImageDraw.Draw(im).ellipse((c - rc, c - rc, c + rc, c + rc), outline=(70, 72, 74, 255), width=SS)
    _body[r] = (im, rc)
    return _body[r]


def knob_sprite(r, level, deg):
    """The knob at deg from the top. The cap's moulded facet (the segment beyond a chord just past the centre, a plane
    sloping down toward the pointer) is shaded by how it faces the fixed light, as Model-303's knob_sprite does, so the
    cap reads as turning while the light stays put; then the index notch."""
    body, rc = knob_body(r)
    im = body.copy()
    c = im.width / 2
    facing = math.cos(math.radians(deg - ds.LIGHT))
    half = math.degrees(math.acos(-0.08))
    pts = [polar(c, c, rc - 0.5 * SS, deg - half + 2 * half * i / 40) for i in range(41)]
    mask = Image.new("L", im.size, 0)
    ImageDraw.Draw(mask).polygon(pts, fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(0.5 * SS))
    v = int(168 + 44 * facing)
    plane = Image.new("RGBA", im.size, (v, v, v + 3, 255))
    fall = Image.new("L", im.size, 0)       # a slope reads by a faint fall toward the rim
    fx, fy = polar(c, c, rc * 0.95, deg)
    ImageDraw.Draw(fall).ellipse((fx - rc * 0.55, fy - rc * 0.55, fx + rc * 0.55, fy + rc * 0.55), fill=50)
    fall = ImageChops.multiply(fall.filter(ImageFilter.GaussianBlur(rc * 0.35)), mask)
    facet = Image.new("RGBA", im.size, (0, 0, 0, 0))
    facet.paste(plane, (0, 0), mask)
    im.alpha_composite(facet)
    dark = Image.new("RGBA", im.size, (0, 0, 0, 0))
    dark.putalpha(fall)
    im.alpha_composite(dark)
    d = ImageDraw.Draw(im)
    ax, ay = polar(c, c, rc - SS, deg - half)       # the ridge along the chord
    bx, by = polar(c, c, rc - SS, deg + half)
    d.line([(ax, ay), (bx, by)], fill=(250, 250, 250, 220) if facing < 0 else (70, 70, 74, 230), width=max(1, int(1.2 * SS)))
    x0, y0 = polar(c, c, rc * 0.62, deg)            # the index notch at the facet's low edge
    x1, y1 = polar(c, c, rc * 0.98, deg)
    d.line([(x0, y0), (x1, y1)], fill=(24, 24, 26, 255), width=int(2.4 * SS))
    return im
