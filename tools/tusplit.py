#!/usr/bin/env python3
"""Derive per-object units for config/splits.txt from the evidence of
tools/tuevidence.py and the reference graph.

.text:
  * Functions whose EH funclets share one .text$x chunk belong to one
    object (no boundary inside a chunk's span), and consecutive chunks belong
    to different objects (at least one boundary between them).
  * Boundaries go into the gaps with the fewest "local" references crossing
    them: calls/references between functions at most WINDOW functions
    apart, plus vtable neighbours. Widely used functions (more than 8
    callers) are ignored. Required boundaries take the best gap in their
    interval; more boundaries are added best-first until there are TARGET
    objects (Rich header: about 73 game C++ objects).
.text$x / .xdata$x: follow the EH functions (chunk -> unit; FuncInfo, unwind
  and try maps -> unit).
.text$yc / .text$yd / .CRT$XCU: follow the unit that owns the initialised
  global object.
.rdata: an item belongs to the first object that uses it (COMDAT rule), i.e.
  the unit of its lowest referencing function. .data/.bss: the unit that
  references the item most often. Section boundaries are placed
  monotonically so that they agree with the most items.

Every boundary chosen by the reference-graph heuristic is marked with
"# guess" and its cut weight; boundaries forced by EH chunks with "# eh".

usage: tusplit.py [--target N] [--window W]   (rewrites config/splits.txt)
"""
import bisect
import json
import os
import sys

from splits import load_splits
from wfcommon import BUILD, CONFIG, ORIG_EXE, PE

TARGET = 73
WINDOW = 20


def arg(name, default):
    if name in sys.argv:
        return int(sys.argv[sys.argv.index(name) + 1])
    return default


class Evidence:
    def __init__(self):
        self.pe = PE(ORIG_EXE)
        self.an = json.load(open(os.path.join(BUILD, "analysis.json")))
        old = load_splits()
        self.old = old
        self.rng = {}
        for u in old.units:
            for sec, lo, hi in u.ranges:
                lo0, hi0 = self.rng.get(sec, (lo, hi))
                self.rng[sec] = (min(lo0, lo), max(hi0, hi))
        self.externs = [e for u in old.units for e in u.externs]
        tlo, thi = self.rng[".text"]
        self.thunks = self.import_thunks(tlo, thi)
        self.funcs = sorted((a, e) for a, e in self.an["functions"] if tlo <= a < self.thunks)
        self.starts = [a for a, _ in self.funcs]

    def import_thunks(self, lo, hi):
        """Start of the trailing `jmp [__imp_x]` thunks in the game .text."""
        a = hi
        while a - 6 >= lo and self.pe.read(a - 6, 2) == b"\xff\x25":
            a -= 6
        return a

    def func_index(self, a):
        i = bisect.bisect_right(self.starts, a) - 1
        if i >= 0 and a < self.funcs[i][1]:
            return i
        return None

    def eh_chunks(self):
        xlo, xhi = self.rng[".text$x"]
        x = self.pe.read(xlo, xhi - xlo)
        tlo = self.starts[0]
        text = self.pe.read(tlo, self.thunks - tlo)
        pushers = {}
        for k in range(len(text) - 5):
            if text[k] == 0x68:
                v = int.from_bytes(text[k + 1:k + 5], "little")
                if xlo <= v < xhi:
                    f = self.func_index(tlo + k)
                    if f is not None:
                        pushers.setdefault(v, set()).add(f)
        bounds, start, i = [], 0, 0
        while i < len(x):
            if x[i] == 0xCC:
                j = i
                while j < len(x) and x[j] == 0xCC:
                    j += 1
                if (xlo + j) % 16 == 0:
                    bounds.append((xlo + start, xlo + j))
                    start = j
                i = j
            else:
                i += 1
        if start < len(x):
            bounds.append((xlo + start, xhi))
        chunks = []
        for c0, c1 in bounds:
            fs, funcinfos = set(), []
            for k in range(c1 - c0 - 9):
                o = c0 - xlo + k
                if x[o] == 0xB8 and x[o + 5] == 0xE9:
                    fs |= pushers.get(c0 + k, set())
                    funcinfos.append(int.from_bytes(x[o + 1:o + 5], "little"))
            chunks.append(dict(lo=c0, hi=c1, funcs=sorted(fs), funcinfos=funcinfos))
        return chunks

    def cut_weights(self, window):
        idx = {a: i for i, a in enumerate(self.starts)}
        edges, callers = {}, {}
        for at, kind, t in self.an["relocs"]:
            if t in idx:
                a = self.func_index(at)
                if a is not None and a != idx[t]:
                    callers.setdefault(idx[t], set()).add(a)
                    edges[(a, idx[t])] = edges.get((a, idx[t]), 0) + 1
        rlo, rhi = self.rng[".rdata"]
        vt = sorted((at, idx[t]) for at, kind, t in self.an["relocs"] if rlo <= at < rhi and t in idx)
        for (a1, f1), (a2, f2) in zip(vt, vt[1:]):
            if a2 == a1 + 4 and f1 != f2:
                edges[(f1, f2)] = edges.get((f1, f2), 0) + 2
        n = len(self.starts)
        diff = [0] * (n + 2)
        for (a, b), w in edges.items():
            if len(callers.get(a, ())) > 8 or len(callers.get(b, ())) > 8 or abs(a - b) > window:
                continue
            p, q = min(a, b), max(a, b)
            diff[p + 1] += w
            diff[q + 1] -= w
        self.connected = [False] * n
        for (a, b), w in edges.items():
            if len(callers.get(a, ())) > 8 or len(callers.get(b, ())) > 8 or abs(a - b) > window:
                continue
            self.connected[a] = self.connected[b] = True
        cut, c = [], 0
        for i in range(n + 1):
            c += diff[i]
            cut.append(c)
        return cut  # cut[i]: weight crossing the gap before function i


def monotone_bounds(items, nunits):
    """items: sorted [(addr, unit)]. Returns start address index per unit
    (non-decreasing) agreeing with as many items as possible."""
    n = len(items)
    pos = [0] * (nunits + 1)
    pos[nunits] = n
    lo = 0
    for k in range(1, nunits):
        # choose p >= lo maximising (#items < p with unit < k) + (#items >= p with unit >= k)
        best, bestp = None, lo
        left = sum(1 for _, u in items[:lo] if u < k)
        right = sum(1 for _, u in items[lo:] if u >= k)
        score = left + right
        best, bestp = score, lo
        for p in range(lo, n):
            if items[p][1] < k:
                score += 1
            else:
                score -= 1
            if score > best:
                best, bestp = score, p + 1
        pos[k] = bestp
        lo = bestp
    return pos


def main():
    target = arg("--target", TARGET)
    window = arg("--window", WINDOW)
    ev = Evidence()
    n = len(ev.starts)
    chunks = ev.eh_chunks()
    cut = ev.cut_weights(window)
    forbidden = set()
    required = []
    prev_hi = None
    for ch in chunks:
        if not ch["funcs"]:
            continue
        lo, hi = min(ch["funcs"]), max(ch["funcs"])
        forbidden |= set(range(lo + 1, hi + 1))
        if prev_hi is not None:
            required.append((prev_hi + 1, lo))
        prev_hi = hi
    bounds = {}
    for lo, hi in required:
        cands = [g for g in range(lo, hi + 1) if g not in forbidden]
        g = min(cands, key=lambda g: (cut[g], g))
        bounds[g] = "eh"
    # natural boundaries: no local reference crosses the gap and the function
    # after it is connected to its neighbours (isolated helpers stay with the
    # preceding code)
    conn = ev.connected
    for g in range(1, n):
        if cut[g] == 0 and conn[g] and g not in forbidden and g not in bounds:
            bounds[g] = "guess"
    # too many: merge the smallest neighbouring pair (never at an EH boundary)
    natural = len(bounds) + 1
    while len(bounds) + 1 > target:
        gs = sorted(bounds)
        edges_ = [0] + gs + [n]
        best = None
        for i, g in enumerate(gs):
            if bounds[g] == "eh":
                continue
            size = edges_[i + 2] - edges_[i]
            if best is None or size < best[0]:
                best = (size, g)
        if best is None:
            break
        del bounds[best[1]]
    # too few: split the largest unit at its lowest-cut gap
    while len(bounds) + 1 < target:
        edges_ = [0] + sorted(bounds) + [n]
        spans = sorted(zip(edges_, edges_[1:]), key=lambda se: se[0] - se[1])
        for lo, hi in spans:
            cands = [g for g in range(lo + 1, hi) if g not in forbidden]
            if cands:
                mid = (lo + hi) / 2
                g = min(cands, key=lambda g: (cut[g], abs(g - mid)))
                bounds[g] = "guess"
                break
        else:
            break
    gaps = sorted(bounds)
    unit_of_func = []
    u = 0
    for i in range(n):
        if u < len(gaps) and gaps[u] == i:
            u += 1
        unit_of_func.append(u)
    nunits = len(gaps) + 1
    text_lo = [ev.starts[0]] + [ev.starts[g] for g in gaps]
    text_hi = text_lo[1:] + [ev.rng[".text"][1]]
    ranges = {k: [] for k in range(nunits)}
    for k in range(nunits):
        ranges[k].append((".text", text_lo[k], text_hi[k]))

    def unit_at(a):
        f = ev.func_index(a)
        return unit_of_func[f] if f is not None else None

    # EH: .text$x chunks and .xdata$x
    xdata_items = []
    ch_items = []
    for ch in chunks:
        us = sorted({unit_of_func[f] for f in ch["funcs"]})
        uu = us[0] if us else None
        ch_items.append((ch["lo"], uu))
        for fi in ch["funcinfos"]:
            for a in (fi, ev.pe.u32(fi + 8), ev.pe.u32(fi + 16)):
                if a:
                    xdata_items.append((a, uu))
    # fill unknown units forward
    def fill(items):
        out, last = [], 0
        for a, uu in sorted(items):
            if uu is None:
                uu = last
            last = uu
            out.append((a, uu))
        return out

    # initializers: global object -> unit by references
    refs = {}
    for at, kind, t in ev.an["relocs"]:
        uu = unit_at(at)
        if uu is not None:
            refs.setdefault(t, []).append(uu)

    def mode(t):
        r = refs.get(t)
        if not r:
            return None
        return max(set(r), key=r.count)

    clo, chi = ev.rng[".CRT$XCU"]
    yc_items, yd_items, xcu_items = [], [], []
    for a in range(clo, chi, 4):
        init = ev.pe.u32(a)
        code = ev.pe.read(init, 0x20)
        obj = int.from_bytes(code[1:5], "little") if code[0] == 0xB9 else None
        uu = mode(obj) if obj else None
        dtor = None
        p = code.find(b"\x68")
        if p >= 0:
            dtor = int.from_bytes(code[p + 1:p + 5], "little")
        xcu_items.append((a, uu))
        yc_items.append((init, uu))
        if dtor and ev.rng[".text$yd"][0] <= dtor < ev.rng[".text$yd"][1]:
            yd_items.append((dtor, uu))
    # data items
    data_items = {}
    for sec in (".rdata", ".data", ".bss"):
        lo, hi = ev.rng[sec]
        items = []
        for t, us in refs.items():
            if lo <= t < hi:
                items.append((t, min(us) if sec == ".rdata" else mode(t)))
        data_items[sec] = items

    sections = [(".text$x", ch_items), (".text$yc", yc_items), (".text$yd", yd_items),
                (".rdata", data_items[".rdata"]), (".xdata$x", xdata_items),
                (".CRT$XCU", xcu_items), (".data", data_items[".data"]), (".bss", data_items[".bss"])]
    for sec, items in sections:
        lo, hi = ev.rng[sec]
        items = fill([(a, uu) for a, uu in items if lo <= a < hi])
        if not items:
            ranges[0].append((sec, lo, hi))
            continue
        pos = monotone_bounds(items, nunits)
        starts = []
        for k in range(nunits):
            p = pos[k]
            starts.append(lo if k == 0 else (items[p][0] if p < len(items) else hi))
        if sec == ".xdata$x":
            starts = [lo] + starts[1:]
        for k in range(nunits):
            s = starts[k]
            e = starts[k + 1] if k + 1 < nunits else hi
            if s < e:
                ranges[k].append((sec, s, e))
    order = [".text", ".text$x", ".text$yc", ".text$yd", ".rdata", ".xdata$x", ".CRT$XCU", ".data", ".bss"]
    old_opts = {u.name: u.opts for u in ev.old.units}
    with open(os.path.join(CONFIG, "splits.txt"), "w") as fp:
        fp.write(open(os.path.join(CONFIG, "splits-header.txt")).read())
        for k in range(nunits):
            name = f"obj{k:02d}_{text_lo[k]:06x}"
            opts = old_opts.get(name, {})
            opt = "".join(f" {kk}={vv}" for kk, vv in opts.items())
            why = ""
            if k:
                g = gaps[k - 1]
                why = f"   # {bounds[g]} cut={cut[g]}"
            fp.write(f"unit {name}{opt}{why}\n")
            for sec in order:
                for s, lo, hi in ranges[k]:
                    if s == sec:
                        fp.write(f"    {s:<9} {lo:#x} {hi:#x}\n")
            if k == 0:
                for sym, key in ev.externs:
                    fp.write(f"    extern {sym} {key:#x}\n")
            fp.write("\n")
    print(f"{natural} natural units; {nunits} units ({sum(1 for v in bounds.values() if v == 'eh')} EH-forced boundaries)")


if __name__ == "__main__":
    main()
