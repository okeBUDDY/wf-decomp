#!/usr/bin/env python3
"""Parse LINK .map files; compare symbol placement with config/symbols.txt.

usage: mapfile.py [build/wf.map]   -> lists library symbols whose address in the
rebuilt image differs from the original (first ones per section).
"""
import os
import re
import sys

from gensyms import read_symbols
from wfcommon import BUILD

LINE = re.compile(r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s+(f\s+)?(i\s+)?(\S+)")


def parse(path):
    """-> list of (va, name, object, static)"""
    out = []
    static = False
    for line in open(path, encoding="latin-1"):
        if "Static symbols" in line:
            static = True
        m = LINE.match(line)
        if m:
            out.append((int(m.group(4), 16), m.group(3), m.group(7), static))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(BUILD, "wf.map")
    syms = read_symbols()
    byname = {}
    for a, s in syms.items():
        byname.setdefault(s["name"], a)
    bad = 0
    last = None
    for va, name, obj, static in sorted(parse(path)):
        if name not in byname:
            continue
        want = byname[name]
        if want != va:
            delta = va - want
            if delta != last:
                print(f"{name:40} {obj:28} want {want:#x} got {va:#x} delta {delta:+#x}")
                last = delta
            bad += 1
        else:
            if last is not None:
                print(f"{name:40} {obj:28} OK at {va:#x}")
            last = None
    print(f"{bad} misplaced symbols")


if __name__ == "__main__":
    main()
