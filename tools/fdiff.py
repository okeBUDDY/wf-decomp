#!/usr/bin/env python3
"""Side-by-side disassembly of a compiled function (build/obj/src) and the
original. usage: fdiff.py <symbol name or address>   (run compare.py first)"""
import difflib
import glob
import os
import sys

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

from gensyms import read_symbols
from wfcommon import BUILD, ORIG_EXE, PE, Coff


def find_compiled(name):
    for path in glob.glob(os.path.join(BUILD, "obj", "src", "*.obj")):
        c = Coff(open(path, "rb").read(), path)
        for sec in c.sections:
            if sec["flags"] & 0x20:
                for y in sec["syms"]:
                    if y["name"] == name and y["value"] == 0:
                        return sec
    return None


def lines(data, base, md):
    out = []
    for ins in md.disasm(bytes(data), base):
        out.append(f"{ins.address - base:4x}  {ins.mnemonic} {ins.op_str}")
    return out


def main():
    syms = read_symbols()
    by_name = {s["name"]: a for a, s in syms.items()}
    arg = sys.argv[1]
    addr = by_name.get(arg) or int(arg, 16)
    name = syms[addr]["name"]
    sec = find_compiled(name)
    if sec is None:
        raise SystemExit(f"{name} not compiled")
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    orig = PE(ORIG_EXE).read(addr, syms[addr]["size"])
    a = lines(orig, addr, md)
    b = lines(sec["data"], addr, md)
    w = max(len(x) for x in a + b) + 2
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes():
        for k in range(max(i2 - i1, j2 - j1)):
            left = a[i1 + k] if i1 + k < i2 else ""
            right = b[j1 + k] if j1 + k < j2 else ""
            mark = " " if tag == "equal" else "|"
            print(f"{left:<{w}}{mark} {right}")


if __name__ == "__main__":
    main()
