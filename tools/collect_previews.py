#!/usr/bin/env python3
"""Replace docs/previews/<kit>/ with the distinct skin pages from _build/<kit>/preview (build_all.sh output)."""
import glob
import os
import re
import shutil

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
for kit in ("6w6", "8w8", "cw78", "9w9"):
    dst = os.path.join(root, "docs", "previews", kit)
    shutil.rmtree(dst, ignore_errors=True)
    os.makedirs(dst)
    pages = [p for p in glob.glob(os.path.join(root, "_build", kit, "preview", "page_*.png")) if "_open" not in p]
    pages.sort(key=lambda p: int(re.search(r"page_(\d+)", p).group(1)))
    for i, p in enumerate(pages, 1):
        shutil.copy(p, os.path.join(dst, "page%d.png" % i))
    print(kit, len(pages), "pages")
