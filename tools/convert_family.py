#!/usr/bin/env python3
"""Packages the TR-MPC family for the plugin-folder format WITHOUT porting: Akai's SDK tools/legacy_convert.py merges each release's armv7 and
aarch64 zips into one "<Manufacturer> - VST - <Name>" folder (original .so files, original Plugin Skins, a generated plugin.json), then this script
verifies that every skin file, data file and binary is byte-identical to the release zips, and writes dist-sdk/<Name>-<ver>-VST2-Plugins.zip.
    tools/convert_family.py [kit ...]     (default: all six)   MPC_SDK: the mpc-plugin-builder-sdk checkout"""
import hashlib, os, shutil, subprocess, sys, zipfile
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SDK = os.environ.get("MPC_SDK", os.path.expanduser("~/mpc-310-private/mpc-plugin-builder-sdk"))
REL = os.path.join(ROOT, "build", "rel")
OUT = os.path.join(ROOT, "dist-sdk")
KITS = {"6w6": ("6W6-1.2.0", "6w6-vst-v1.2.0"), "8w8": ("8W8-1.2.0", "8w8-vst-v1.2.0"), "9w9": ("9W9-1.2.0", "9w9-vst-v1.2.0"), "cw78": ("CW-78-1.2.0", "cw78-vst-v1.2.0"),
        "trkit": ("TR-MPC-1.1.0", "trmpc-vst-v1.1.0"), "tapfx": ("TR-MPC-Tap-FX-1.1.0", "trmpc-tap-fx-vst-v1.1.0")}


def sha(b):
    return hashlib.sha256(b).hexdigest()


def release_files(zpath):
    """{path inside the plugin folder: sha256} of a release zip's portable folder."""
    z = zipfile.ZipFile(zpath)
    out, folder = {}, None
    for n in z.namelist():
        if "/portable/" in n and not n.endswith("/"):
            folder = n.split("/portable/")[1].split("/")[0]
            out[n.split("/portable/" + folder + "/")[1]] = sha(z.read(n))
    return folder, out


def main():
    kits = sys.argv[1:] or list(KITS)
    os.makedirs(OUT, exist_ok=True)
    for k in kits:
        stem, tag = KITS[k]
        arm, a64 = [os.path.join(REL, "%s-mpc-%s.zip" % (stem, c)) for c in ("armv7", "aarch64")]
        for z, c in ((arm, "armv7"), (a64, "aarch64")):
            if not os.path.exists(z):
                subprocess.check_call(["gh", "release", "download", tag, "-R", "sd88me/mpc-vst-TR-MPC", "-p", "*-mpc-%s.zip" % c, "--dir", REL, "--clobber"])
        tmp = os.path.join(ROOT, "build", "conv", k)
        shutil.rmtree(tmp, ignore_errors=True)
        subprocess.check_call([sys.executable, os.path.join(SDK, "tools", "legacy_convert.py"), arm, a64, "--out", tmp], stdout=subprocess.DEVNULL)
        folder = os.listdir(tmp)[0]
        fa, files_a = release_files(arm)
        _, files_b = release_files(a64)
        assert fa == folder, (fa, folder)
        # every released file that is not packaging must be in the converted folder, byte for byte (skins, samples, binaries, version.xml)
        skip = {"mpc-plugin.json", "plugin-meta.xml", "plugin.json"}
        bad = []
        for rel, h in list(files_a.items()) + list(files_b.items()):
            if rel in skip:
                continue
            if rel.endswith(".so"):   # the two builds share one folder: the converter renames the 64-bit one, so match binaries by content
                if h not in {sha(open(os.path.join(tmp, folder, f), "rb").read()) for f in os.listdir(os.path.join(tmp, folder)) if f.endswith(".so")}:
                    bad.append("binary not in the folder: " + rel)
                continue
            p = os.path.join(tmp, folder, rel)
            if not os.path.exists(p):
                bad.append("missing " + rel)
            elif sha(open(p, "rb").read()) != h:
                bad.append("changed " + rel)
        n = sum(1 for _ in os.walk(os.path.join(tmp, folder)))
        if bad:
            sys.exit("%s: not identical to the release: %s" % (k, bad[:5]))
        zout = os.path.join(OUT, "%s-VST2-Plugins.zip" % stem)
        with zipfile.ZipFile(zout, "w", zipfile.ZIP_DEFLATED) as z:
            for d, _, fs in os.walk(os.path.join(tmp, folder)):
                for f in sorted(fs):
                    full = os.path.join(d, f)
                    zi = zipfile.ZipInfo(os.path.relpath(full, tmp).replace(os.sep, "/"), (2026, 11, 9, 0, 0, 0))
                    zi.external_attr = (0o100755 if os.stat(full).st_mode & 0o100 else 0o100644) << 16
                    zi.compress_type = zipfile.ZIP_DEFLATED
                    z.writestr(zi, open(full, "rb").read())
        print("%-22s %d release files identical -> %s (%.1f MB)" % (folder, len(files_a) + len(files_b), os.path.relpath(zout, ROOT), os.path.getsize(zout) / 1e6))


if __name__ == "__main__":
    main()
