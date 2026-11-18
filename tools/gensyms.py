#!/usr/bin/env python3
"""(Re)generate config/symbols.txt.

Columns: address  name  size  type  [section]
  type: func (game function), data (game data label), vtable, import (IAT
  slot), lib (library function), libdata (library data, from the link map).
Names already present in symbols.txt are kept (manual renames survive);
new entries get placeholder names (FUN_/DAT_/vtbl_).
"""
import csv
import json
import os

from wfcommon import BUILD, CONFIG, ORIG_EXE, PE, VC71, import_symbols
from splits import load_splits

SYMS = os.path.join(CONFIG, "symbols.txt")
IMPORT_LIBS = ["Kernel32.Lib", "User32.Lib", "Gdi32.Lib", "WinMM.Lib", "ddraw.lib", "dsound.lib",
               "dinput.lib"]


def read_symbols(path=SYMS):
    out = {}
    if not os.path.exists(path):
        return out
    for line in open(path):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        f = line.split()
        out[int(f[0], 16)] = dict(name=f[1], size=int(f[2], 16), type=f[3])
    return out


def write_symbols(syms, path=SYMS):
    with open(path, "w") as fp:
        fp.write("# address name size type   (see tools/gensyms.py)\n")
        for a in sorted(syms):
            s = syms[a]
            fp.write(f"{a:08x} {s['name']} {s['size']:x} {s['type']}\n")


def iat_symbols(pe):
    """IAT slot address -> __imp_ symbol name."""
    import pefile
    p = pefile.PE(ORIG_EXE, fast_load=True)
    p.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"]])
    table = {}
    for lib in IMPORT_LIBS:
        for sym, dll, imp, ntype in import_symbols(f"{VC71}/PlatformSDK/Lib/{lib}"):
            table[(dll.upper(), imp)] = sym
    out = {}
    for e in p.DIRECTORY_ENTRY_IMPORT:
        dll = e.dll.decode().upper()
        for i in e.imports:
            key = (dll, i.name.decode() if i.name else i.ordinal)
            out[i.address] = "__imp_" + table[key]
    return out


def main():
    pe = PE(ORIG_EXE)
    old = read_symbols()
    an = json.load(open(os.path.join(BUILD, "analysis.json")))
    sp = load_splits()
    syms = {}
    for a, e in an["functions"]:
        syms[a] = dict(name=f"FUN_{a:08x}", size=e - a, type="func")
    # referenced data in game units
    for at, kind, t in an["relocs"]:
        r = sp.unit_of(t)
        if r and not r[1].startswith(".text") and t not in syms:
            syms[t] = dict(name=f"DAT_{t:08x}", size=0, type="data")
    for r in csv.DictReader(open(os.path.join(CONFIG, "library-symbols.csv"))):
        a = int(r["address"], 16)
        if r["name"].startswith("<"):
            continue
        syms[a] = dict(name=r["name"], size=int(r["section_size"]), type="lib")
    for a, n in iat_symbols(pe).items():
        syms[a] = dict(name=n, size=4, type="import")
    libdata = os.path.join(CONFIG, "libdata.txt")
    if os.path.exists(libdata):
        for a, s in read_symbols(libdata).items():
            syms.setdefault(a, s)
    cpath = os.path.join(CONFIG, "comdats.txt")
    if os.path.exists(cpath):
        for line in open(cpath):
            f = line.split("#", 1)[0].split()
            if f:
                syms[int(f[0], 16)] = dict(name=f[1], size=int(f[2], 16), type="data")
    for a, s in syms.items():
        if s["type"] == "data" and not s["name"].startswith("DAT_"):
            continue
        if a in old and old[a]["type"] == s["type"]:
            s["name"] = old[a]["name"]
    write_symbols(syms)
    print(f"{len(syms)} symbols")


if __name__ == "__main__":
    main()
