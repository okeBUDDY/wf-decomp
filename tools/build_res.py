#!/usr/bin/env python3
"""Build build/wf.res from res/wf.rc with the toolchain's rc.exe.

usage: build_res.py [--check]
The icon is the only binary resource. It is not committed: it is extracted from
the original exe's RT_ICON/RT_GROUP_ICON resources into build/res/wf.ico (an
ordinary .ico file whose directory entry repeats the group-icon entry). The
script res/wf.rc is UTF-8; rc.exe gets a Shift-JIS copy in build/res/.
--check compares every resource rc wrote with the original's data.
"""
import os
import shutil
import struct
import sys

import pefile

from wfcommon import BUILD, ORIG_EXE, ROOT, VC71, run_wine, wine_env, winpath

RES_SRC = os.path.join(ROOT, "res")
RES_OUT = os.path.join(BUILD, "res")
RT_ICON, RT_GROUP_ICON = 3, 14


def resources(pe):
    """{(type, name): data} of the original exe."""
    out = {}
    for t in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        for n in t.directory.entries:
            for lang in n.directory.entries:
                d = lang.data.struct
                out[(t.id, n.id)] = pe.get_data(d.OffsetToData, d.Size)
    return out


def extract_icon(res, group_id, path):
    """Write the icon group `group_id` as an .ico file."""
    grp = res[(RT_GROUP_ICON, group_id)]
    _, typ, count = struct.unpack_from("<HHH", grp, 0)
    entries, images = [], []
    offset = 6 + 16 * count
    for i in range(count):
        w, h, colors, rsv, planes, bits, size, icon_id = struct.unpack_from(
            "<BBBBHHIH", grp, 6 + 14 * i)
        img = res[(RT_ICON, icon_id)]
        entries.append(struct.pack("<BBBBHHII", w, h, colors, rsv, planes, bits, len(img), offset))
        images.append(img)
        offset += len(img)
    with open(path, "wb") as fp:
        fp.write(struct.pack("<HHH", 0, typ, count) + b"".join(entries) + b"".join(images))


def read_res(path):
    """[(type, name, data)] of a .res file (numeric ids only)."""
    d = open(path, "rb").read()
    out, o = [], 0
    while o + 8 <= len(d):
        size, hsize = struct.unpack_from("<II", d, o)
        tff, typ, nff, name = struct.unpack_from("<HHHH", d, o + 8)
        assert tff == 0xFFFF and nff == 0xFFFF, "named resources are not supported"
        if size:
            out.append((typ, name, d[o + hsize:o + hsize + size]))
        o += hsize + size
        o = (o + 3) & ~3
    return out


def build(out=None):
    out = out or os.path.join(BUILD, "wf.res")
    os.makedirs(RES_OUT, exist_ok=True)
    pe = pefile.PE(ORIG_EXE)
    res = resources(pe)
    extract_icon(res, 103, os.path.join(RES_OUT, "wf.ico"))
    for f in os.listdir(RES_SRC):
        text = open(os.path.join(RES_SRC, f), encoding="utf-8").read()
        open(os.path.join(RES_OUT, f), "wb").write(text.encode("cp932"))
    env = wine_env([f"{VC71}/include", f"{VC71}/PlatformSDK/Include"], [])
    rc, txt = run_wine(f"{VC71}/bin/rc.exe",
                       ["/l", "0x411", "/fo", winpath(out), winpath(os.path.join(RES_OUT, "wf.rc"))],
                       env, cwd=RES_OUT, check=False)
    if rc != 0:
        raise SystemExit(f"rc failed ({rc}):\n{txt}")
    return out, res


def main():
    out, res = build()
    print("wrote", out)
    if "--check" in sys.argv:
        bad = 0
        for typ, name, data in read_res(out):
            same = res.get((typ, name)) == data
            bad += not same
            print(f"type {typ} name {name}: {len(data)} bytes {'match' if same else 'DIFFER'}")
        sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
