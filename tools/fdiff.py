#!/usr/bin/env python3
"""Side-by-side disassembly of a compiled function (build/obj/src) and the
original. usage: fdiff.py [-n] <symbol name or address>   (run compare.py first)

-n aligns the instructions without their offsets and branch/call targets and
ends with the number of differing instructions (a finer score than bytes)."""
import re
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


def lines(data, base, md, norm=False):
    out = []
    for ins in md.disasm(bytes(data), base):
        if norm:
            op = ins.op_str
            if ins.mnemonic.startswith("j") or ins.mnemonic == "call":
                op = re.sub(r"0x[0-9a-f]+", "X", op)
            out.append(f"{ins.mnemonic} {op}")
        else:
            out.append(f"{ins.address - base:4x}  {ins.mnemonic} {ins.op_str}")
    return out


def main():
    syms = read_symbols()
    by_name = {s["name"]: a for a, s in syms.items()}
    norm = "-n" in sys.argv
    arg = [a for a in sys.argv[1:] if a != "-n"][0]
    addr = by_name.get(arg) or int(arg, 16)
    name = syms[addr]["name"]
    sec = find_compiled(name)
    if sec is None:
        raise SystemExit(f"{name} not compiled")
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    orig = PE(ORIG_EXE).read(addr, syms[addr]["size"])
    a = lines(orig, addr, md, norm)
    b = lines(sec["data"], addr, md, norm)
    w = max(len(x) for x in a + b) + 2
    ndiff = 0
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes():
        if tag != "equal":
            ndiff += max(i2 - i1, j2 - j1)
        for k in range(max(i2 - i1, j2 - j1)):
            left = a[i1 + k] if i1 + k < i2 else ""
            right = b[j1 + k] if j1 + k < j2 else ""
            mark = " " if tag == "equal" else "|"
            print(f"{left:<{w}}{mark} {right}")
    if norm:
        print(f"{ndiff} instructions differ")


if __name__ == "__main__":
    main()
