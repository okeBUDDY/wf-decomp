#!/usr/bin/env python3
"""Collect evidence for translation-unit (object file) boundaries of the game
code and write config/tu-evidence.txt.

1. EH funclets (.text$x). Each object contributes one .text$x section,
   aligned to 16 and padded with int3. A run of 0xCC that ends on a 16-byte
   boundary therefore ends an object. (Two objects can merge when a chunk
   happens to end on a boundary.) The __ehhandler$ stubs in a chunk
   (`mov eax, FuncInfo; jmp ___CxxFrameHandler`) are pushed by the prologues
   of their parent functions, which ties each chunk to .text functions.
   The chunks come in link order, so each object's EH functions lie between
   the neighbouring chunks' functions.
2. Dynamic initializers (.CRT$XCU -> .text$yc). Each one constructs a global
   object; the object's .bss/.data address and its ctor anchor a TU, and
   their order is the link order.
"""
import bisect
import json
import os

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

from splits import load_splits
from wfcommon import BUILD, CONFIG, ORIG_EXE, PE


def main():
    pe = PE(ORIG_EXE)
    sp = load_splits()
    rng = {sec: (lo, hi) for u in sp.units for sec, lo, hi in u.ranges}
    an = json.load(open(os.path.join(BUILD, "analysis.json")))
    starts = sorted(a for a, _ in an["functions"])
    tlo, thi = rng[".text"]
    text = pe.read(tlo, thi - tlo)
    xlo, xhi = rng[".text$x"]
    x = pe.read(xlo, xhi - xlo)
    pushers = {}
    for k in range(len(text) - 5):
        if text[k] == 0x68:
            v = int.from_bytes(text[k + 1:k + 5], "little")
            if xlo <= v < xhi:
                pushers.setdefault(v, set()).add(starts[bisect.bisect_right(starts, tlo + k) - 1])
    chunks, start, i = [], 0, 0
    while i < len(x):
        if x[i] == 0xCC:
            j = i
            while j < len(x) and x[j] == 0xCC:
                j += 1
            if (xlo + j) % 16 == 0:
                chunks.append((xlo + start, xlo + i))
                start = j
            i = j
        else:
            i += 1
    if start < len(x):
        chunks.append((xlo + start, xhi))
    out = ["# Evidence for game translation-unit boundaries (tools/tuevidence.py)", "",
           "# EH funclet chunks (.text$x, one per object): chunk -> EH functions in .text",
           "# chunk_start chunk_end  functions"]
    for c0, c1 in chunks:
        fs = set()
        for k in range(c1 - c0 - 9):
            o = c0 - xlo + k
            if x[o] == 0xB8 and x[o + 5] == 0xE9:
                fs |= pushers.get(c0 + k, set())
        out.append(f"{c0:08x} {c1:08x}  " + " ".join(f"{f:08x}" for f in sorted(fs)))
    out += ["", "# Dynamic initializers (.CRT$XCU order): initializer, global object, ctor, atexit dtor",
            "# initializer object ctor dtor"]
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    clo, chi = rng[".CRT$XCU"]
    for a in range(clo, chi, 4):
        init = pe.u32(a)
        obj = ctor = dtor = 0
        for ins in md.disasm(pe.read(init, 0x40), init):
            if ins.mnemonic == "mov" and ins.op_str.startswith("ecx, 0x") and not obj:
                obj = int(ins.op_str.split(", ")[1], 16)
            elif ins.mnemonic == "call" and not ctor and ins.op_str.startswith("0x"):
                ctor = int(ins.op_str, 16)
            elif ins.mnemonic == "push" and ins.op_str.startswith("0x"):
                dtor = int(ins.op_str, 16)
            if ins.mnemonic == "ret":
                break
        out.append(f"{init:08x} {obj:08x} {ctor:08x} {dtor:08x}")
    open(os.path.join(CONFIG, "tu-evidence.txt"), "w").write("\n".join(out) + "\n")
    print(f"{len(chunks)} EH chunks, {(chi - clo) // 4} initializers")


if __name__ == "__main__":
    main()
