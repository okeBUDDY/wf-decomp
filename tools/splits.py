"""Loader for config/splits.txt.

Format (one directive per line, '#' starts a comment):

    unit <name> [key=value ...]
        <section> <start> <end>        # [start, end) virtual addresses
        ...

        extern <symbol> <key>          # undefined external without relocations

"extern" lines reproduce references made only from code that LINK discarded
(/OPT:REF); they still pull library members. The key (an address) places the
symbol in the blob's undefined-symbol order, which is the order the linker
searches libraries for them.

A unit is one object file of the original link, in link order. Keys:
    src=<path>     C++ source (relative to repo root) that replaces the unit
                   once it is decompiled; until then a blob object is built.
    comp=cpp|c     compiler id for the blob's @comp.id (Rich header).
    deflib=a.lib,b.lib    extra /DEFAULTLIB directives (#pragma comment(lib)).
Only game units are listed; library objects come from the rebuilt libraries.
"""
import os
import re

from wfcommon import CONFIG


class Unit:
    def __init__(self, name, opts):
        self.name = name
        self.opts = opts
        self.ranges = []  # (section, lo, hi)
        self.externs = []  # (symbol, order key): references from discarded code
        self.kind = "game"

    @property
    def src(self):
        return self.opts.get("src")

    def contains(self, a):
        return any(lo <= a < hi for _, lo, hi in self.ranges)

    def __repr__(self):
        return f"Unit({self.name})"


class Splits:
    def __init__(self, units):
        self.units = units

    def unit_of(self, a):
        for u in self.units:
            for sec, lo, hi in u.ranges:
                if lo <= a < hi:
                    return u, sec, lo, hi
        return None

    def unit_ending_at(self, a):
        for u in self.units:
            for sec, lo, hi in u.ranges:
                if hi == a:
                    return u, sec, lo, hi
        return None


def load_splits(path=None):
    path = path or os.path.join(CONFIG, "splits.txt")
    units = []
    cur = None
    for ln, line in enumerate(open(path), 1):
        line = line.split("#", 1)[0].rstrip()
        if not line.strip():
            continue
        if not line[0].isspace():
            f = line.split()
            if f[0] != "unit":
                raise ValueError(f"{path}:{ln}: expected 'unit'")
            opts = dict(kv.split("=", 1) for kv in f[2:])
            cur = Unit(f[1], opts)
            units.append(cur)
        else:
            f = line.split()
            if f[0] == "extern" and cur is not None and len(f) == 3:
                cur.externs.append((f[1], int(f[2], 16)))
                continue
            if cur is None or len(f) != 3:
                raise ValueError(f"{path}:{ln}: bad range line")
            lo, hi = int(f[1], 16), int(f[2], 16)
            if not re.match(r"^\.[A-Za-z$]+", f[0]) or hi < lo:
                raise ValueError(f"{path}:{ln}: bad range")
            cur.ranges.append((f[0], lo, hi))
    return Splits(units)
