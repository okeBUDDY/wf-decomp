#!/usr/bin/env python3
"""Generate "blob" COFF objects for game units that are not (fully) decompiled.

Each blob reproduces the unit's original bytes, section by section, with
relocations rebuilt from build/analysis.json so that the linker resolves every
cross-unit, library and import reference by symbol. Symbols from
config/symbols.txt that fall inside the unit are defined (external), so other
units, decompiled code and libraries can refer to them by name.

usage: mkblobs.py [unit ...]     -> build/blobs/<unit>.obj
"""
import bisect
import json
import os
import sys

from coffwriter import (IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32, SCN_LNK_COMDAT, ObjectWriter,
                        Section, section_flags)
from gensyms import read_symbols
from splits import load_splits
from wfcommon import BUILD, CONFIG, ORIG_EXE, PE

COMP_IDS = {"cpp": 0x600C05, "c": 0x5F0C05}  # Utc1310_CPP / Utc1310_C build 3077
DEFAULT_DIRECTIVES = b'   /DEFAULTLIB:"LIBCMT" /DEFAULTLIB:"OLDNAMES" '


class Resolver:
    """Maps a target address to (symbol, addend) and records which unit has
    to define which label."""

    def __init__(self, splits, syms):
        self.splits = splits
        self.syms = syms
        self.addrs = sorted(syms)
        self.labels = {}  # unit name -> {addr: name}
        self.warnings = []
        self.external = {}  # library/import symbol -> lowest key (address)

    def owner(self, t):
        r = self.splits.unit_of(t)
        if r:
            return r
        return self.splits.unit_ending_at(t)

    def resolve(self, t):
        s = self.syms.get(t)
        if s and s["type"] in ("lib", "libdata", "import"):
            self.external[s["name"]] = min(self.external.get(s["name"], t), t)
            return s["name"], 0
        own = self.owner(t)
        if own:
            u, sec, lo, hi = own
            s = self.syms.get(t)
            name = s["name"] if s and s["type"] not in ("lib", "libdata", "import") \
                else f"lab_{t:08x}"
            self.labels.setdefault(u.name, {})[t] = name
            return name, 0
        s = self.syms.get(t)
        if s:
            return s["name"], 0
        i = bisect.bisect_right(self.addrs, t) - 1
        if i >= 0:
            a = self.addrs[i]
            s = self.syms[a]
            if s["type"] in ("lib", "libdata") and a <= t < a + max(s["size"], 1):
                self.external[s["name"]] = min(self.external.get(s["name"], a), a)
                return s["name"], t - a
        self.warnings.append(t)
        return f"abs_{t:08x}", None


def load_comdats():
    out = {}
    p = os.path.join(CONFIG, "comdats.txt")
    if os.path.exists(p):
        for line in open(p):
            line = line.split("#", 1)[0].split()
            if line:
                out.setdefault(int(line[0], 16), []).append((line[1], int(line[2], 16)))
    return out


IAT_KEY = 0x451000
# LINK aligns the start of a section group to the largest alignment of its
# contributions, so no piece may ask for more than the group start has.
GROUP_ALIGN = {}


def set_group_align(splits):
    starts = {}
    for u in splits.units:
        for sec, lo, hi in u.ranges:
            starts[sec] = min(starts.get(sec, lo), lo)
    for sec, lo in starts.items():
        a = 1
        while a < 16 and lo % (a * 2) == 0:
            a *= 2
        GROUP_ALIGN[sec] = a
FIRST_UNIT = [None]


def load_imports():
    p = os.path.join(CONFIG, "imports.txt")
    if not os.path.exists(p):
        return []
    return [line.split()[0] for line in open(p) if line.strip() and not line.startswith("#")]


def pieces(unit, comdats):
    """Split the unit's ranges so that each shared COMDAT constant is its own
    section: yields (section, lo, hi, comdat_name_or_None, duplicate).

    Several names at one address (e.g. __real@00000000 and the 4-byte wide
    string ??_C@_13NOLLCAOD@...) each get their own copy; /OPT:ICF folds the
    copies back into one, while the libraries' definitions are discarded by
    name. A single copy would instead be folded with a later library copy and
    move there."""
    for secname, lo, hi in unit.ranges:
        cur = lo
        for a in sorted(c for c in comdats if lo <= c < hi):
            if a > cur:
                yield secname, cur, a, None, False
            for i, (name, size) in enumerate(comdats[a]):
                yield secname, a, a + size, name, i > 0
            cur = a + comdats[a][0][1]
        if cur < hi:
            yield secname, cur, hi, None, False


def build_blob(unit, pe, an, res, outdir, comdats):
    w = ObjectWriter()
    comp = unit.opts.get("comp", "cpp")
    w.absolute("@comp.id", COMP_IDS[comp])
    drectve = DEFAULT_DIRECTIVES
    for lib in filter(None, unit.opts.get("deflib", "").split(",")):
        drectve += b'/DEFAULTLIB:"' + lib.encode() + b'" '
    w.add_section(Section(".drectve", drectve, section_flags(".drectve", 1)))
    relocs = an["relocs"]
    at_list = [r[0] for r in relocs]
    secs = []
    for secname, lo, hi, comdat, dup in pieces(unit, comdats):
        align = 1
        while align < GROUP_ALIGN.get(secname, 16) and lo % (align * 2) == 0:
            align *= 2
        if secname == ".bss":
            sec = Section(".bss", None, section_flags(".bss", align), size=hi - lo)
        elif comdat:
            sec = Section(secname, bytearray(pe.read(lo, hi - lo)),
                          section_flags(secname, align) | SCN_LNK_COMDAT, comdat=2)
            sec.comdat_name = comdat
            w.defined[comdat] = True
        else:
            sec = Section(secname, bytearray(pe.read(lo, hi - lo)), section_flags(secname, align))
        w.add_section(sec)
        if dup:
            continue
        secs.append((sec, lo, hi))
        i = bisect.bisect_left(at_list, lo)
        while i < len(relocs) and relocs[i][0] < hi:
            at, kind, t = relocs[i]
            i += 1
            if at + 4 > hi:
                continue
            name, addend = res.resolve(t)
            off = at - lo
            if addend is None:  # absolute fallback
                w_abs = name
                if w_abs not in w.defined:
                    w.absolute(w_abs, t)
                addend = 0
            sec.data[off:off + 4] = (addend & 0xFFFFFFFF).to_bytes(4, "little") \
                if kind == "dir32" else (addend & 0xFFFFFFFF).to_bytes(4, "little")
            w.undef_key.setdefault(name, t - addend)
            sec.relocs.append((off, name, IMAGE_REL_I386_DIR32 if kind == "dir32"
                               else IMAGE_REL_I386_REL32))
    for name, key in unit.externs:
        w.undef_key[name] = key
        w.phantoms.append(name)
    if unit is FIRST_UNIT[0]:
        # import references in the order that reproduces the IAT (iatorder.py)
        for i, name in enumerate(load_imports()):
            w.undef_key[name] = IAT_KEY + i
            w.phantoms.append(name)
    return w, secs


def main():
    pe = PE(ORIG_EXE)
    an = json.load(open(os.path.join(BUILD, "analysis.json")))
    sp = load_splits()
    syms = read_symbols()
    res = Resolver(sp, syms)
    comdats = load_comdats()
    outdir = os.path.join(BUILD, "blobs")
    os.makedirs(outdir, exist_ok=True)
    want = set(sys.argv[1:])
    units = [u for u in sp.units if not want or u.name in want]
    writers = []
    FIRST_UNIT[0] = sp.units[0]
    set_group_align(sp)
    for u in units:
        writers.append((u,) + build_blob(u, pe, an, res, outdir, comdats))
    # The first blob lists every library/import reference of all blobs, keyed
    # like the original single object, so that LINK's undefined-symbol list (and
    # with it the library search order) does not depend on the unit split.
    if writers and writers[0][0] is sp.units[0]:
        w0 = writers[0][1]
        for name, key in res.external.items():
            w0.undef_key.setdefault(name, key)
            w0.phantoms.append(name)
    # second pass: define symbols (named ones from symbols.txt + labels)
    for u, w, secs in writers:
        names = dict(res.labels.get(u.name, {}))
        for a, s in syms.items():
            if s["type"] in ("func", "data", "vtable") and u.contains(a):
                names[a] = s["name"]
        for a, name in sorted(names.items()):
            for sec, lo, hi in secs:
                if lo <= a < hi or (a == hi and not any(l2 <= a < h2 for _, l2, h2 in secs)):
                    if name not in w.defined:
                        w.define(name, sec, a - lo, func=sec.name.startswith(".text"))
                    break
        open(os.path.join(outdir, u.name + ".obj"), "wb").write(w.build())
    if res.warnings:
        ws = sorted(set(res.warnings))
        print(f"warning: {len(ws)} targets resolved as absolute: "
              + " ".join(f"{t:#x}" for t in ws[:20]) + (" ..." if len(ws) > 20 else ""))
    print(f"{len(writers)} blobs written to {outdir}")


if __name__ == "__main__":
    main()
