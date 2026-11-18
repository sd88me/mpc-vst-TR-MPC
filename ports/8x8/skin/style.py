"""8X8's look: 8W8's panel (ports/8w8/skin/style.py) with its own name."""
import importlib.util
import os

_spec = importlib.util.spec_from_file_location("style_8w8_base", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "8w8", "skin", "style.py"))
_base = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_base)

STYLE = dict(_base.STYLE)
STYLE["logo"] = "8X8"
for _name in dir(_base):   # panel(), knob_sprite() and whatever else drum_skin asks of a style
    if not _name.startswith("_") and _name not in globals():
        globals()[_name] = getattr(_base, _name)
