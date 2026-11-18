#!/usr/bin/env python3
"""Compare a rebuilt wf.exe with the original.

usage: verify.py [rebuilt.exe] [-v]
Prints SHA-1 of both files, then per header/section the number of differing
bytes and the first few differing ranges (with the nearest known symbol).
Exit status 0 only if the files are identical.
"""
import bisect
import hashlib
import os
import struct
import sys

from gensyms import read_symbols
from wfcommon import BUILD, ORIG_EXE, PE


def ranges(diff):
    out = []
    for i in diff:
        if out and i <= out[-1][1] + 8:
            out[-1][1] = i
        else:
            out.append([i, i])
    return out


def header_fields(pe_off, i):
    """Name the PE header field containing file offset i (best effort)."""
    names = {pe_off + 8: "TimeDateStamp", pe_off + 24 + 64: "CheckSum",
             pe_off + 24 + 56: "SizeOfImage", pe_off + 24 + 4: "SizeOfCode",
             pe_off + 24 + 8: "SizeOfInitializedData", pe_off + 24 + 16: "AddressOfEntryPoint"}
    for o, n in names.items():
        if o <= i < o + 4:
            return n
    if 0x80 <= i < pe_off:
        return "DOS stub / Rich header"
    return ""


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    verbose = "-v" in sys.argv
    new_path = args[0] if args else os.path.join(BUILD, "wf.exe")
    a = open(ORIG_EXE, "rb").read()
    b = open(new_path, "rb").read()
    ha, hb = hashlib.sha1(a).hexdigest(), hashlib.sha1(b).hexdigest()
    print(f"original {ha}  {len(a)} bytes")
    print(f"rebuilt  {hb}  {len(b)} bytes")
    if a == b:
        print("MATCH: rebuilt wf.exe is byte-identical")
        return 0
    pa, pb = PE(ORIG_EXE), PE(new_path)
    syms = read_symbols()
    saddr = sorted(syms)

    def near(va):
        i = bisect.bisect_right(saddr, va) - 1
        if i < 0:
            return ""
        s = syms[saddr[i]]
        return f"{s['name']}+{va - saddr[i]:#x}"

    hdr_end = pa.sections[0]["raw"]
    d = [i for i in range(min(hdr_end, len(b))) if a[i] != b[i]]
    print(f"headers: {len(d)} bytes differ")
    for lo, hi in ranges(d)[:10]:
        print(f"   file {lo:#06x}-{hi:#06x} {header_fields(pa.pe, lo)}")
    for sa in pa.sections:
        sb = next((s for s in pb.sections if s["name"] == sa["name"]), None)
        if sb is None:
            print(f"{sa['name']}: missing in rebuilt exe")
            continue
        info = ""
        if (sa["va"], sa["vsize"], sa["rsize"]) != (sb["va"], sb["vsize"], sb["rsize"]):
            info = (f" (layout differs: va {sa['va']:#x}/{sb['va']:#x} vsize {sa['vsize']:#x}/"
                    f"{sb['vsize']:#x} raw {sa['rsize']:#x}/{sb['rsize']:#x})")
        da = a[sa["raw"]:sa["raw"] + sa["rsize"]]
        db = b[sb["raw"]:sb["raw"] + sb["rsize"]]
        n = max(len(da), len(db))
        diff = [i for i in range(n) if i >= len(da) or i >= len(db) or da[i] != db[i]]
        print(f"{sa['name']}: {len(diff)} of {len(da)} bytes differ{info}")
        rs = ranges(diff)
        for lo, hi in rs[:(50 if verbose else 8)]:
            va = 0x400000 + sa["va"] + lo
            print(f"   {va:#x}-{0x400000 + sa['va'] + hi:#x} ({hi - lo + 1} bytes) near {near(va)}")
        if len(rs) > (50 if verbose else 8):
            print(f"   ... {len(rs)} ranges")
    return 1


if __name__ == "__main__":
    sys.exit(main())
