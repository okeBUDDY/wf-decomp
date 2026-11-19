#!/usr/bin/env python3
"""Tries every single-line move inside a block of one function.

usage: perm_lines.py src/file.cpp ADDR [-w] < block

The block (stdin) must occur in the file verbatim; put each statement on one
line. Prints the six best moves (by differing bytes) and, when one matches or
with -w, the best block. Check by hand that a move keeps the meaning.
"""
import sys

from try_variants import run


def main():
    path, addr = sys.argv[1], sys.argv[2].lower().removeprefix("0x").zfill(8)
    body = sys.stdin.read().rstrip("\n")
    lines = body.split("\n")
    variants, desc = [], []
    for i in range(len(lines)):
        for j in range(len(lines) + 1):
            if j in (i, i + 1):
                continue
            moved = lines[:]
            x = moved.pop(i)
            moved.insert(j if j < i else j - 1, x)
            variants.append((body, "\n".join(moved)))
            desc.append(f"move {i}->{j}")
    scored = []
    for k, r in enumerate(run(path, addr, variants)):
        f = r.split()
        n = 0 if f[0] == "OK" else (int(f[1]) if len(f) > 1 and f[1].isdigit() else 99999)
        scored.append((n, k, r))
    scored.sort()
    for n, k, r in scored[:6]:
        print(desc[k], r)
    if scored and (scored[0][0] == 0 or "-w" in sys.argv):
        print("---- best block:")
        print(variants[scored[0][1]][1])


if __name__ == "__main__":
    main()
