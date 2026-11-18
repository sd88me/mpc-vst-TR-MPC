"""9X9's look: 9W9's panel (ports/9w9/skin/style.py) with its own name, and the Latency control under ACCENT on FX/MASTER."""
import importlib.util
import os

_spec = importlib.util.spec_from_file_location("style_9w9_base", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "9w9", "skin", "style.py"))
_base = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_base)

STYLE = dict(_base.STYLE)
STYLE["logo"] = "9X9"
STYLE["fx"] = [f if f[0] != "accent" else ("accent", f[1], ["latency"]) for f in _base.STYLE["fx"]]
STYLE["fx_names"] = {"latency": "LATENCY"}
# Controls a TR-909 doesn't have (9X9 builds them on the circuit; see README): marked on the panel
STYLE["extended"] = {"bd_c_sweep_depth", "bd_c_pitch_mod", "lt_c_attack", "mt_c_attack", "ht_c_attack", "rs_tune", "hc_tune",
                     "hc_decay", "chh_pitch", "ohh_pitch", "rc_decay", "cr_decay"}
STYLE["extended_mark"] = "pill"
STYLE["fx_captions"] = {"latency": "EACH BLOCK ADDS 2.9 ms LATENCY\nBUT IMPROVES PERFORMANCE"}
panel = _base.panel
knob_sprite = _base.knob_sprite
for _name in dir(_base):   # whatever else drum_skin asks of a style (tile(), ...)
    if not _name.startswith("_") and _name not in globals():
        globals()[_name] = getattr(_base, _name)
