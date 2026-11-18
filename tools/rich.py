#!/usr/bin/env python3
"""Decode the Rich header of a PE file.  usage: rich.py exe [exe2]"""
import struct
import sys

PRODUCTS = {0x01: "Import", 0x0f: "Masm710", 0x19: "Implib700", 0x1c: "Utc13_C",
            0x1d: "Utc13_CPP", 0x5a: "Linker710", 0x5d: "Implib710", 0x5e: "Cvtres710",
            0x5f: "Utc1310_C", 0x60: "Utc1310_CPP"}


def rich(path):
    d = open(path, "rb").read()
    end = d.find(b"Rich", 0x80)
    key = struct.unpack_from("<I", d, end + 4)[0]
    start = end
    while start > 0x80:
        start -= 4
        if struct.unpack_from("<I", d, start)[0] ^ key == 0x536E6144:  # "DanS"
            break
    out = []
    for o in range(start + 16, end, 8):
        cid, cnt = struct.unpack_from("<II", d, o)
        cid ^= key
        cnt ^= key
        out.append((cid >> 16, cid & 0xFFFF, cnt))
    return out


def main():
    rs = [rich(p) for p in sys.argv[1:]]
    keys = []
    for r in rs:
        for p, b, c in r:
            if (p, b) not in keys:
                keys.append((p, b))
    for p, b in keys:
        cols = []
        for r in rs:
            cols.append(next((str(c) for pp, bb, c in r if (pp, bb) == (p, b)), "-"))
        print(f"{PRODUCTS.get(p, hex(p)):12} {b:5}  " + "  ".join(f"{c:>5}" for c in cols))


if __name__ == "__main__":
    main()
