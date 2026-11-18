#!/usr/bin/env python3
"""Static analysis of the game code in wf.exe.

Recursive-descent disassembly of the game code ranges (with switch jump tables),
function boundary recovery and reconstruction of relocations (absolute
addresses in code and data, rel32 branches that leave a function).

Writes build/analysis.json:
  functions: [[start, end], ...]   (end excludes trailing int3 padding)
  tables:    [[addr, size, kind], ...]  jump/index tables inside .text
  relocs:    [[at, kind, target], ...]  kind: "dir32" | "rel32"
The game ranges are read from config/splits.txt (sections with "game" units).
"""
import json
import os
import struct

import capstone
from capstone import x86

from wfcommon import BUILD, ORIG_EXE, PE

IMG_LO, IMG_HI = 0x401000, 0x46C000


class Analyzer:
    def __init__(self, pe, code_ranges, data_ranges):
        self.pe = pe
        self.code_ranges = code_ranges
        self.data_ranges = data_ranges
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
        self.insns = {}  # addr -> insn
        self.code = set()  # byte addresses that are code
        self.tables = {}  # addr -> (size, kind)
        self.func_starts = set()
        self.known_code = set()  # library function addresses (valid pointer targets)

    def in_code(self, a):
        return any(lo <= a < hi for lo, hi in self.code_ranges)

    def code_range(self, a):
        for lo, hi in self.code_ranges:
            if lo <= a < hi:
                return lo, hi
        return None

    def decode(self, a):
        if a in self.insns:
            return self.insns[a]
        r = self.code_range(a)
        buf = self.pe.read(a, min(16, r[1] - a))
        ins = next(self.md.disasm(buf, a, 1), None)
        self.insns[a] = ins
        return ins

    # ------------------------------------------------------------ seeds ---
    def seeds(self):
        pe = self.pe
        s = set()
        for lo, hi in self.code_ranges:
            s.add(lo)
            for a in range(lo + 16 - (lo % 16 or 16), hi, 16):
                if pe.read(a - 1, 1) == b"\xcc" and pe.read(a, 1) != b"\xcc":
                    s.add(a)
        for lo, hi in self.data_ranges:
            for a in range(lo, hi - 3, 4):
                v = pe.u32(a)
                if self.in_code(v):
                    s.add(v)
        return s

    # ---------------------------------------------------------- descent ---
    NOPS = (b"\x90", b"\x8b\xff", b"\x8b\xc0", b"\x8d\x49\x00", b"\x8d\x64\x24\x00", b"\x8d\xa4\x24\x00\x00\x00\x00",
            b"\x8d\x9b\x00\x00\x00\x00", b"\xeb\x03", b"\xcc")

    def run(self):
        """Descend from the seeds; then seed unreached non-padding code at
        16-byte boundaries (functions only reached through unknown pointers)."""
        self.func_starts = set(self.seeds())
        self.descend(sorted(self.func_starts))
        while True:
            new = [a for a in self.unreached() if a % 16 == 0 and a not in self.func_starts]
            if not new:
                break
            self.func_starts.update(new)
            self.descend(new)
        return self

    def table_bytes(self):
        tb = set()
        for a, (s, _) in self.tables.items():
            tb.update(range(a, a + s))
        return tb

    def unreached(self):
        """Start addresses of byte runs that are neither code, tables nor padding."""
        tb = self.table_bytes()
        out = []
        for lo, hi in self.code_ranges:
            a = lo
            while a < hi:
                if a in self.code or a in tb:
                    a += 1
                    continue
                for nop in self.NOPS:
                    if self.pe.read(a, len(nop)) == nop:
                        a += len(nop)
                        break
                else:
                    out.append(a)
                    while a < hi and a not in self.code and a not in tb:
                        a += 1
        return out

    def descend(self, work):
        work = list(work)
        seen = set(self.insns_seen) if hasattr(self, "insns_seen") else set()
        self.insns_seen = seen
        while work:
            a = work.pop()
            while a not in seen and self.in_code(a) and a not in self.tables:
                ins = self.decode(a)
                if ins is None:
                    break
                seen.add(a)
                for k in range(ins.size):
                    self.code.add(a + k)
                m = ins.mnemonic
                nxt = a + ins.size
                if ins.imm_size == 4 and m != "call" and not m.startswith("j"):
                    v = struct.unpack_from("<I", ins.bytes, ins.imm_offset)[0]
                    if self.in_code(v) and v not in self.func_starts:
                        self.func_starts.add(v)
                        work.append(v)
                if m == "call":
                    t = self.branch_target(ins)
                    if t is not None and self.in_code(t):
                        self.func_starts.add(t)
                        work.append(t)
                elif m == "jmp":
                    t = self.branch_target(ins)
                    if t is not None:
                        if self.in_code(t):
                            work.append(t)
                    else:
                        self.jump_table(ins, work)
                    break
                elif m.startswith("j") or m.startswith("loop"):
                    t = self.branch_target(ins)
                    if t is not None and self.in_code(t):
                        work.append(t)
                elif m in ("ret", "retf", "int3", "hlt", "ud2"):
                    break
                a = nxt

    @staticmethod
    def branch_target(ins):
        op = ins.operands[0] if ins.operands else None
        if op is not None and op.type == x86.X86_OP_IMM:
            return op.imm & 0xFFFFFFFF
        return None

    def jump_table(self, ins, work):
        op = ins.operands[0]
        if op.type != x86.X86_OP_MEM or op.mem.scale != 4 or op.mem.base != 0:
            return
        tab = op.mem.disp & 0xFFFFFFFF
        if not self.in_code(tab):
            return
        # find "cmp reg, imm / ja" bound and optional byte index table
        count = None
        btab = None
        a = ins.address
        back = self.prev_insns(a, 8)
        for p in reversed(back):
            if p.mnemonic == "movzx" and p.operands[1].type == x86.X86_OP_MEM:
                bt = p.operands[1].mem.disp & 0xFFFFFFFF
                if self.in_code(bt):
                    btab = bt
            if p.mnemonic == "cmp" and len(p.operands) == 2 and p.operands[1].type == x86.X86_OP_IMM:
                count = p.operands[1].imm + 1
                break
        if btab is not None:
            n = count if count else 1
            idx = self.pe.read(btab, n)
            self.tables[btab] = (n, "byte")
            for k in range(n):
                self.code.discard(btab + k)
            count = max(idx) + 1
        if count is None:
            count = 0
            while True:
                v = self.pe.u32(tab + 4 * count)
                if not self.in_code(v) or count > 512:
                    break
                count += 1
        self.tables[tab] = (4 * count, "dword")
        for k in range(count):
            t = self.pe.u32(tab + 4 * k)
            if self.in_code(t):
                work.append(t)

    def prev_insns(self, a, n):
        out = []
        # walk back over already-decoded linear predecessors
        cur = a
        for _ in range(n):
            found = None
            for sz in range(1, 16):
                p = self.insns.get(cur - sz)
                if p is not None and p.size == sz:
                    found = p
                    break
            if found is None:
                break
            out.insert(0, found)
            cur = found.address
        return out

    # ------------------------------------------------------- functions ---
    def functions(self):
        starts = sorted(a for a in self.func_starts if a in self.code)
        out = []
        for i, a in enumerate(starts):
            lo, hi = self.code_range(a)
            e = starts[i + 1] if i + 1 < len(starts) and starts[i + 1] < hi else hi
            while e > a and self.pe.read(e - 1, 1) == b"\xcc" and (e - 1) not in self.code and \
                    not any(t <= e - 1 < t + s for t, (s, _) in self.tables.items()):
                e -= 1
            out.append([a, e])
        return out

    # ---------------------------------------------------------- relocs ---
    def plausible_data_pointer(self, a, v):
        """Reject dwords inside text strings (e.g. "TE\\0" read as 0x00455449)."""
        b = self.pe.read(a - 1, 5)
        if all(_printable(c) for c in b[:4]):
            return False
        if self.in_code(v):
            return v in self.func_starts or v in self.code
        return any(lo <= v <= hi for lo, hi in self.data_ranges) or self.extra_ok(v)

    def extra_ok(self, v):
        sec = self.pe.section_of(v)
        if sec == ".text":
            return v in self.known_code
        return sec in (".rdata", ".data")

    def relocs(self, funcs):
        import bisect
        starts = [f[0] for f in funcs]

        def func_of(a):
            i = bisect.bisect_right(starts, a) - 1
            return funcs[i] if i >= 0 and a < funcs[i][1] else None

        rel = []
        for a, ins in sorted(self.insns.items()):
            if ins is None or a not in self.code:
                continue
            f = func_of(a)
            m = ins.mnemonic
            is_branch = m == "call" or m.startswith("j") or m.startswith("loop")
            if is_branch and ins.operands and ins.operands[0].type == x86.X86_OP_IMM:
                t = ins.operands[0].imm & 0xFFFFFFFF
                if ins.size >= 5 and (f is None or not (f[0] <= t < f[1])):
                    rel.append([a + ins.size - 4, "rel32", t])
                continue
            if ins.disp_size == 4:
                v = struct.unpack_from("<I", ins.bytes, ins.disp_offset)[0]
                if IMG_LO <= v < IMG_HI:
                    rel.append([a + ins.disp_offset, "dir32", v])
            if ins.imm_size == 4:
                v = struct.unpack_from("<I", ins.bytes, ins.imm_offset)[0]
                if IMG_LO <= v < IMG_HI:
                    rel.append([a + ins.imm_offset, "dir32", v])
        for t, (size, kind) in self.tables.items():
            if kind == "dword":
                for k in range(0, size, 4):
                    rel.append([t + k, "dir32", self.pe.u32(t + k)])
        for lo, hi in self.data_ranges:
            for a in range(lo, hi - 3, 4):
                v = self.pe.u32(a)
                if IMG_LO <= v < IMG_HI and self.plausible_data_pointer(a, v):
                    rel.append([a, "dir32", v])
        rel.sort()
        return rel


def _printable(b):
    return 0x20 <= b < 0x7F


def load_ranges():
    from splits import load_splits
    sp = load_splits()
    code, data = [], []
    for u in sp.units:
        if u.kind != "game":
            continue
        for sec, lo, hi in u.ranges:
            (code if sec.startswith(".text") else data).append((lo, hi))
    return merge(code), merge(data)


def merge(rs):
    rs = sorted(rs)
    out = []
    for lo, hi in rs:
        if out and lo <= out[-1][1]:
            out[-1][1] = max(out[-1][1], hi)
        else:
            out.append([lo, hi])
    return [tuple(r) for r in out]


def main():
    pe = PE(ORIG_EXE)
    code, data = load_ranges()
    an = Analyzer(pe, code, data)
    import csv
    from wfcommon import CONFIG
    for r in csv.DictReader(open(os.path.join(CONFIG, "library-symbols.csv"))):
        an.known_code.add(int(r["address"], 16))
    an.run()
    funcs = an.functions()
    rel = an.relocs(funcs)
    os.makedirs(BUILD, exist_ok=True)
    json.dump(dict(functions=funcs, tables=[[a, s, k] for a, (s, k) in sorted(an.tables.items())],
                   relocs=rel), open(os.path.join(BUILD, "analysis.json"), "w"))
    ncode = len(an.code)
    tot = sum(hi - lo for lo, hi in code)
    print(f"functions {len(funcs)}, tables {len(an.tables)}, relocs {len(rel)}, "
          f"code bytes {ncode}/{tot}")


if __name__ == "__main__":
    main()
