#!/usr/bin/env python3
"""Check rebuilt library functions against wf.exe.

For every function in config/library-symbols.csv that belongs to one of the
given libraries, find the function in the built .lib and compare its bytes with
the exe at the known address (relocation targets masked).
usage: libcheck.py [lib ...]   (default: all built libs)
"""
import csv
import os
import sys

from wfcommon import BUILD, CONFIG, ORIG_EXE, PE, Coff

LIBMAP = {"libogg": "ogg", "libvorbis": ("vorbis", "vorbisfile"), "zlib": "zlib", "libpng": "png"}


def lib_functions(libname):
    """name -> (bytes, maskset, objname) for each function in build/lib/<libname>/*.obj."""
    out = {}
    d = os.path.join(BUILD, "lib", libname)
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".obj"):
            continue
        c = Coff(open(os.path.join(d, fn), "rb").read(), fn)
        objname = fn
        for s in c.sections:
            if not s["flags"] & 0x20 or not s["data"]:
                continue
            syms = sorted((y for y in s["syms"] if y["cls"] in (2, 3) and y["type"] & 0x20),
                          key=lambda y: y["value"])
            for k, y in enumerate(syms):
                end = syms[k + 1]["value"] if k + 1 < len(syms) else len(s["data"])
                data = s["data"][y["value"]:end]
                mask = set()
                for off, _, _ in s["rels"]:
                    for j in range(off, off + 4):
                        if y["value"] <= j < end:
                            mask.add(j - y["value"])
                out[y["name"]] = (data, mask, objname)
    return out


def main():
    pe = PE(ORIG_EXE)
    rows = list(csv.DictReader(open(os.path.join(CONFIG, "library-symbols.csv"))))
    want = sys.argv[1:] or list(LIBMAP)
    total = {"ok": 0, "bad": 0, "missing": 0, "okbytes": 0, "bytes": 0}
    for lib in want:
        names = LIBMAP[lib]
        names = names if isinstance(names, tuple) else (names,)
        funcs = {}
        for n in names:
            funcs.update(lib_functions(n))
        stats = {"ok": 0, "bad": 0, "missing": 0}
        for r in rows:
            if r["library"] != lib:
                continue
            nm = r["name"]
            va = int(r["address"], 16)
            if nm not in funcs:
                stats["missing"] += 1
                print(f"  MISSING {va:#x} {nm}")
                continue
            data, mask, obj = funcs[nm]
            # strip trailing int3/nop alignment padding
            n = len(data)
            tgt = pe.read(va, n)
            bad = [i for i in range(n) if i not in mask and data[i] != tgt[i]]
            total["bytes"] += n
            if bad:
                stats["bad"] += 1
                print(f"  DIFF    {va:#x} {nm} ({obj}, {len(bad)}/{n} bytes differ, first +{bad[0]:#x})")
            else:
                stats["ok"] += 1
                total["okbytes"] += n
        print(f"{lib}: {stats['ok']} match, {stats['bad']} differ, {stats['missing']} missing")
        for k in stats:
            total[k] += stats[k]
    print(f"TOTAL: {total['ok']} match, {total['bad']} differ, {total['missing']} missing; "
          f"{total['okbytes']}/{total['bytes']} bytes")


if __name__ == "__main__":
    main()
