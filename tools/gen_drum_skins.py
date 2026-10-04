#!/usr/bin/env python3
"""Per-family skin styling for the four drum ports (6w6, 8w8, cw78, 9w9): palette, fonts, faceplate art.

    gen_drum_skins.py [kit ...]     writes ports/<kit>/skin.css, ports/<kit>/images/plate_<n>.svg and ports/<kit>/fonts/;
                                    then run tools/make_layout.py (it imports STYLES for theme, header and art lines).

Reference hardware (Roland TR-606 Drumatix, TR-808, CR-78 CompuRhythm, TR-909): palettes and typography per kit in STYLES.
Faceplate art is shapes only (SVG drawn as an <image>, so custom fonts do not apply inside it); all lettering is live
frame titles / text widgets set in the kit's fonts through skin.css (@font-face) and `fontfile=`.

Fonts come from the sibling mpc-vst checkout (Assets/fonts, FONT_SRC) and are copied into ports/<kit>/fonts/. They are
commercial/unknown-licence typefaces: ports/*/fonts/ is gitignored, so they never get published from this repo.
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
FONT_SRC = os.environ.get("FONT_SRC", os.path.join(ROOT, "..", "mpc-vst", "Assets", "fonts"))
W, H = 1280, 628
Y_OFF = 86
X0, PANEL_W, TOP, BOT = 8, 300, 136, 708      # must match make_layout.py
HEAD_Y0, HEAD_Y1 = 92, 136

FONTS = {   # family -> (source file under FONT_SRC, copied name)
    "Earth": ("Earth Normal.ttf", "Earth.ttf"),
    "Filmotype": ("FilmotypeFord.ttf", "FilmotypeFord.ttf"),
    "Univers": ("Univers 53 Extended Regular.otf", "UniversExt.otf"),
    "Helvetica": ("helvetica-255/Helvetica-Bold.ttf", "HelveticaBold.ttf"),
}

STYLES = {
    # TR-606: silver-grey aluminium, near-black printing, red LEDs/outline, cobalt accents; Eurostile titles
    "6w6": dict(
        theme="bg=b9bbb4 ink=1a1d24 ink_dim=4c4f55 accent=dd0000 accent_hi=ff3b30 seg_active=1a1d24 seg_inactive=d9dad4 "
              "seg_active_tx=ecd5aa lcd=1a1d24 line=6f7279 btn_bg=d9dad4 btn_text=1a1d24 box=cacbc4 display_ink=dd0000 "
              "knob_face=e4e5e1 knob_ring=7b7e84 knob_dot=1a1d24",
        knob_look="metal", logo=("DRUMATIX", "Univers"), logo_color="1a1d24", logo_size=3.0,
        tag="TR-606  COMPUTER CONTROLLED", tag_color="dd0000", fonts=("Helvetica", "Helvetica"),
        title_color="1a1d24", title_size="17px", title_spacing="0.1em"),
    # TR-808: charcoal panel, white lettering, red / orange / yellow stripes
    "8w8": dict(
        theme="bg=202020 ink=ffffff ink_dim=a8a49a accent=f8a125 accent_hi=f1f827 seg_active=f8a125 seg_inactive=3a3a3a "
              "seg_active_tx=202020 lcd=141414 line=6b6b6b btn_bg=3a3a3a btn_text=ffffff box=141414 display_ink=f8a125 "
              "knob_face=121212 knob_ring=5c5c5c knob_dot=f8a125",
        knob_look="cap", logo=("8W8", "Univers"), logo_color="e72e2e", logo_size=3.0,
        tag="RHYTHM COMPOSER  TR-808 STYLE", tag_color="f8a125", fonts=("Univers", "Helvetica"),
        title_color="ffffff", title_size="16px", title_spacing="0.12em"),
    # CR-78: black panel, orange headings, cream lettering, red / avocado / yellow / blue pattern buttons, wood cheeks
    "cw78": dict(
        theme="bg=1c1d21 ink=ecd5aa ink_dim=9c9378 accent=ff5a00 accent_hi=ff8a3d seg_active=ff5a00 seg_inactive=2a2b30 "
              "seg_active_tx=1c1d21 lcd=121316 line=7a735f btn_bg=2a2b30 btn_text=ecd5aa box=121316 display_ink=ff5a00 "
              "knob_face=101112 knob_ring=7a735f knob_dot=ff5a00",
        knob_look="cap", logo=("CompuRhythm", "Filmotype"), logo_color="ff5a00", logo_size=3.0,
        tag="CW-78  CR-78 STYLE", tag_color="ecd5aa", fonts=("Filmotype", "Helvetica"),
        title_color="ff5a00", title_size="17px", title_spacing="0.08em"),
    # TR-909: warm grey chassis, charcoal section bars with white lettering, red-orange logo, blue rules
    "9w9": dict(
        theme="bg=c1bdb3 ink=111111 ink_dim=55524b accent=e4002b accent_hi=ff3355 seg_active=2b2c30 seg_inactive=d1c7bd "
              "seg_active_tx=ffffff lcd=2b2c30 line=7d7a72 btn_bg=d1c7bd btn_text=111111 box=b4b0a6 display_ink=e4002b "
              "knob_face=1a1a1a knob_ring=8b8880 knob_dot=ff5a2a",
        knob_look="metal", logo=("9W9", "Earth"), logo_color="e4002b", logo_size=3.4,
        tag="RHYTHM COMPOSER", tag_color="111111", fonts=("Helvetica", "Helvetica"),
        title_color="ffffff", title_size="15px", title_spacing="0.14em"),
}


def panel_xs(n):
    return [X0 + i * PANEL_W + 2 for i in range(n)]


def stripes(x, y, w, colors, h=3, gap=2):
    return "".join('<rect x="%g" y="%g" width="%g" height="%d" fill="#%s"/>' % (x, y + i * (h + gap), w, h, c)
                   for i, c in enumerate(colors))


def plate_svg(kit, n):
    """Faceplate for one page of n panels (shapes only), 1280x628 in plugin-area coordinates."""
    y0, y1 = TOP - Y_OFF, BOT - Y_OFF
    hy0, hy1 = HEAD_Y0 - Y_OFF, HEAD_Y1 - Y_OFF
    pw = PANEL_W - 6
    xs = panel_xs(n)
    d = ['<defs>'
         '<linearGradient id="g606" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#c6c8c1"/><stop offset="1" stop-color="#a9aca3"/></linearGradient>'
         '<linearGradient id="g909" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#cbc7bd"/><stop offset="1" stop-color="#b3afa5"/></linearGradient>'
         '<linearGradient id="wood" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#5a3a22"/><stop offset="0.5" stop-color="#7a5232"/><stop offset="1" stop-color="#4a2f1b"/></linearGradient>'
         '<linearGradient id="woodh" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#7a5232"/><stop offset="1" stop-color="#4a2f1b"/></linearGradient>'
         '<pattern id="grain" width="4" height="4" patternUnits="userSpaceOnUse"><rect width="4" height="1" fill="#fff" fill-opacity="0.05"/><rect y="2" width="4" height="1" fill="#000" fill-opacity="0.06"/></pattern>'
         '</defs>']
    if kit == "6w6":
        d.append('<rect width="%d" height="%d" fill="url(#g606)"/><rect width="%d" height="%d" fill="url(#grain)"/>' % (W, H, W, H))
        d.append('<rect x="0" y="%d" width="%d" height="3" fill="#1a1d24"/>' % (hy1 - 2, W))
        for x in xs:    # recessed panel, thin black frame, red/cobalt tick under the title
            d.append('<rect x="%g" y="%d" width="%d" height="%d" rx="3" fill="#000" fill-opacity="0.06" stroke="#1a1d24" stroke-width="1.5"/>' % (x, y0, pw, y1 - y0))
            d.append('<rect x="%g" y="%d" width="%d" height="3" fill="#1a1d24"/>' % (x + 14, y0 + 36, pw - 28))
            d.append('<rect x="%g" y="%d" width="46" height="3" fill="#dd0000"/><rect x="%g" y="%d" width="22" height="3" fill="#2f3bff"/>' % (x + 14, y0 + 41, x + 66, y0 + 41))
        cy = (hy0 + hy1) / 2
        d.append('<circle cx="1232" cy="%g" r="5" fill="#dd0000"/><circle cx="1232" cy="%g" r="9" fill="none" stroke="#1a1d24" stroke-width="2"/>' % (cy, cy))
    elif kit == "8w8":
        d.append('<rect width="%d" height="%d" fill="#202020"/><rect width="%d" height="%d" fill="url(#grain)"/>' % (W, H, W, H))
        # the 808's four-colour rule across the header
        d.append(stripes(0, hy1 - 11, W, ["e72e2e", "f8a125", "f1f827", "ffffff"], 2, 1))
        for x in xs:
            d.append('<rect x="%g" y="%d" width="%d" height="%d" rx="2" fill="#262626" stroke="#4a4a4a" stroke-width="1.2"/>' % (x, y0, pw, y1 - y0))
            d.append(stripes(x + 14, y0 + 34, pw - 28, ["e72e2e", "f8a125", "f1f827"], 2, 2))
        # step-button colour blocks along the header, like the 808's sequencer row
        cols = ["e72e2e"] * 4 + ["f8a125"] * 4 + ["f1f827"] * 4 + ["ffffff"] * 4
        for i, c in enumerate(cols):
            d.append('<rect x="%d" y="%d" width="16" height="14" rx="2" fill="#%s"/>' % (1010 + i * 15 + (i // 4) * 4, hy0 + 2, c))
    elif kit == "cw78":
        d.append('<rect width="%d" height="%d" fill="#1c1d21"/><rect width="%d" height="%d" fill="url(#grain)"/>' % (W, H, W, H))
        d.append('<rect x="0" y="0" width="8" height="%d" fill="url(#wood)"/><rect x="%d" y="0" width="8" height="%d" fill="url(#wood)"/>' % (H, W - 8, H))
        d.append('<rect x="0" y="%d" width="%d" height="%d" fill="#ff5a00" fill-opacity="0.9"/>' % (hy1 - 2, W, 2))
        for x in xs:    # cream rounded outline like the CR-78 INSTRUMENTS box; four pattern-button chips under the title
            d.append('<rect x="%g" y="%d" width="%d" height="%d" rx="12" fill="#222328" stroke="#ecd5aa" stroke-width="2" stroke-opacity="0.7"/>' % (x, y0, pw, y1 - y0))
            for i, c in enumerate(["dd0000", "cacb9c", "f2ff2c", "2f3bff"]):
                d.append('<rect x="%g" y="%d" width="20" height="8" rx="2" fill="#%s"/>' % (x + pw - 14 - (4 - i) * 24, y0 + 20, c))
            d.append('<rect x="%g" y="%d" width="%d" height="1.5" fill="#ff5a00"/>' % (x + 14, y0 + 40, pw - 28))
    else:  # 9w9
        d.append('<rect width="%d" height="%d" fill="url(#g909)"/><rect width="%d" height="%d" fill="url(#grain)"/>' % (W, H, W, H))
        d.append('<rect x="0" y="%d" width="%d" height="3" fill="#005ca9"/>' % (hy1 - 2, W))
        for x in xs:    # charcoal title bar over a recessed section, blue rule below
            d.append('<rect x="%g" y="%d" width="%d" height="%d" fill="#000" fill-opacity="0.05" stroke="#7d7a72" stroke-width="1"/>' % (x, y0, pw, y1 - y0))
            d.append('<rect x="%g" y="%d" width="%d" height="30" fill="#2b2c30"/>' % (x, y0, pw))
            d.append('<rect x="%g" y="%d" width="%d" height="2" fill="#e4002b"/>' % (x, y0 + 30, pw))
    return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d">%s</svg>\n' % (W, H, W, H, "".join(d))


def css(kit):
    s = STYLES[kit]
    f_title, f_label = s["fonts"]
    ff = []
    for fam in dict.fromkeys(s["fonts"]):
        fn = FONTS[fam][1]
        ff.append('@font-face { font-family: "%s"; src: url("fonts/%s"); }' % (fam, fn))
    return ("/* %s skin: generated by tools/gen_drum_skins.py. Faceplate art is images/plate_*.svg. */\n%s\n"
            ":root { --title-font: \"%s\"; --label-size: 14px; --sheen: 0.06; }\n"
            "text { font-family: \"%s\"; }\n"
            ".frame-border { fill: none; stroke: none; }\n.frame-rule { stroke: none; }\n"
            ".frame-title { font-family: \"%s\"; font-size: %s; letter-spacing: %s; fill: #%s; }\n"
            ".box-label { font-family: \"%s\"; letter-spacing: 0.06em; }\n"
            % (kit, "\n".join(ff), f_title, f_label, f_title, s["title_size"], s["title_spacing"], s["title_color"], f_label))


def build(kit):
    port = os.path.join(ROOT, "ports", kit)
    os.makedirs(os.path.join(port, "images"), exist_ok=True)
    os.makedirs(os.path.join(port, "fonts"), exist_ok=True)
    for fam in dict.fromkeys(STYLES[kit]["fonts"] + (STYLES[kit]["logo"][1],)):
        src, dst = FONTS[fam]
        shutil.copyfile(os.path.join(FONT_SRC, src), os.path.join(port, "fonts", dst))
    for n in (4, 3, 2, 1):
        open(os.path.join(port, "images", "plate_%d.svg" % n), "w", newline="\n").write(plate_svg(kit, n))
    open(os.path.join(port, "skin.css"), "w", newline="\n").write(css(kit))
    print(kit, "skin.css, fonts/, images/plate_1-4.svg")


if __name__ == "__main__":
    for k in (sys.argv[1:] or STYLES):
        build(k)
