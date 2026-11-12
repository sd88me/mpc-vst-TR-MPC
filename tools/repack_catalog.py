#!/usr/bin/env python3
"""The catalog zips (<Name>-<ver>-mpc-armv7.zip / -mpc-aarch64.zip) for the folder-format releases: the previous release's catalog zips
(build/rel, downloaded by convert_family.py) with the version moved on. Binaries, skins and data are untouched; the manifest version, the
INSTALL.md text, the top folder name and SHA256SUMS are rewritten.   tools/repack_catalog.py [kit ...]   -> dist-catalog/"""
import hashlib, json, os, sys, zipfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from convert_family import KITS, NEW, REL, ROOT
OUT = os.path.join(ROOT, "dist-catalog")


def main():
    os.makedirs(OUT, exist_ok=True)
    for k in sys.argv[1:] or list(KITS):
        stem = KITS[k][0]; name, old = stem.rsplit("-", 1); new = NEW[k]
        for cpu in ("armv7", "aarch64"):
            zin = zipfile.ZipFile(os.path.join(REL, "%s-mpc-%s.zip" % (stem, cpu)))
            items = {}
            for i in zin.infolist():
                if i.is_dir():
                    continue
                data = zin.read(i)
                rel = i.filename.split("/", 1)[1]
                if rel.endswith("mpc-plugin.json"):
                    m = json.loads(data); m["version"] = new; data = (json.dumps(m, indent=2) + "\n").encode()
                elif rel == "INSTALL.md":
                    data = data.decode().replace(old, new).replace(stem, "%s-%s" % (name, new)).encode()
                items[rel] = (data, i.external_attr)
            sums = "".join("%s  %s\n" % (hashlib.sha256(d).hexdigest(), r) for r, (d, _) in sorted(items.items()) if r != "SHA256SUMS")
            if "SHA256SUMS" in items:
                items["SHA256SUMS"] = (sums.encode(), items["SHA256SUMS"][1])
            zout = os.path.join(OUT, "%s-%s-mpc-%s.zip" % (name, new, cpu))
            with zipfile.ZipFile(zout, "w", zipfile.ZIP_DEFLATED) as z:
                for r, (d, ea) in items.items():
                    zi = zipfile.ZipInfo("%s-%s/%s" % (name, new, r), (2026, 11, 12, 0, 0, 0)); zi.external_attr = ea; zi.compress_type = zipfile.ZIP_DEFLATED
                    z.writestr(zi, d)
            print(zout)


main()
