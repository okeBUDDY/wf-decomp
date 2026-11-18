#!/usr/bin/env python3
"""Disassemble original functions with symbol names.

usage: disasm.py <addr|name|unit> ...
  addr: hex address of a function; name: symbol from symbols.txt;
  unit: a unit name from splits.txt (all functions of its .text).
Operands that point at known symbols are annotated; strings and float
constants are shown.
"""
import bisect
import json
import os
import re
import struct
import sys

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

from gensyms import read_symbols
from splits import load_splits
from wfcommon import BUILD, ORIG_EXE, PE


class Disasm:
    def __init__(self):
        self.pe = PE(ORIG_EXE)
        self.syms = read_symbols()
        self.addrs = sorted(self.syms)
        an = json.load(open(os.path.join(BUILD, "analysis.json")))
        self.funcs = {a: e for a, e in an["functions"]}
        self.md = Cs(CS_ARCH_X86, CS_MODE_32)

    def name(self, v):
        s = self.syms.get(v)
        if s:
            return s["name"]
        i = bisect.bisect_right(self.addrs, v) - 1
        if i >= 0:
            a = self.addrs[i]
            s = self.syms[a]
            if a < v < a + s["size"]:
                return f"{s['name']}+{v - a:#x}"
        return None

    def describe(self, v):
        n = self.name(v)
        extra = ""
        sec = self.pe.section_of(v) if hasattr(self.pe, "section_of") else None
        if 0x451000 <= v < 0x45c000:
            raw = self.pe.read(v, 64)
            s = raw.split(b"\0")[0]
            if len(s) >= 2 and all(32 <= c < 127 or c >= 0x80 for c in s):
                try:
                    extra = ' "' + s.decode("cp932") + '"'
                except UnicodeDecodeError:
                    extra = ' "' + s.decode("latin-1") + '"'
            else:
                f = struct.unpack("<f", raw[:4])[0]
                dbl = struct.unpack("<d", raw[:8])[0]
                if n and n.startswith("__real@") and len(n) == 15:
                    extra = f" ={f!r}f"
                elif n and n.startswith("__real@"):
                    extra = f" ={dbl!r}"
                elif 1e-6 < abs(f) < 1e7:
                    extra = f" (f {f:g})"
        if n or extra:
            return f"{n or ''}{extra}"
        return None

    def function(self, a):
        e = self.funcs.get(a)
        if e is None:
            raise SystemExit(f"no function at {a:#x}")
        s = self.syms.get(a)
        if s and s["size"] > e - a:
            e = a + s["size"]  # symbols.txt merges analysis mis-splits
        head = self.name(a) or f"{a:08x}"
        print(f"; ---- {head}  [{a:08x}, {e:08x})  size {e - a:#x}")
        for ins in self.md.disasm(self.pe.read(a, e - a), a):
            notes = []
            for m in re.findall(r"0x[0-9a-f]+", ins.op_str):
                v = int(m, 16)
                if 0x401000 <= v < 0x470000:
                    d = self.describe(v)
                    if d:
                        notes.append(d)
            note = ("    ; " + ", ".join(notes)) if notes else ""
            print(f"{ins.address:08x}  {ins.bytes.hex():<20} {ins.mnemonic} {ins.op_str}{note}")
        print()


def main():
    d = Disasm()
    units = {u.name: u for u in load_splits().units}
    by_name = {s["name"]: a for a, s in d.syms.items()}
    for arg in sys.argv[1:]:
        if arg in units:
            for sec, lo, hi in units[arg].ranges:
                if sec == ".text":
                    for a in sorted(f for f in d.funcs if lo <= f < hi):
                        d.function(a)
        elif arg in by_name:
            d.function(by_name[arg])
        else:
            d.function(int(arg, 16))


if __name__ == "__main__":
    main()
