#!/usr/bin/env python3
"""OPTIONAL post-link patch, separate from the link itself.

Copies build/wf.exe to build/wf-stamped.exe and writes the PE header fields
listed in config/postlink.txt (TimeDateStamp, CheckSum) into the copy. LINK
always stamps the current time, so this is the only way to make that field
equal; it does not change anything LINK decides. build/wf.exe stays the
unpatched link output, and the baselines in the README are given both ways.

usage: postlink.py [in.exe [out.exe]] [--no-verify]
"""
import os
import struct
import sys

from wfcommon import BUILD, CONFIG

FIELDS = {"TimeDateStamp": 8, "CheckSum": 24 + 64}  # offsets from the PE signature


def load_values():
    values = {}
    for line in open(os.path.join(CONFIG, "postlink.txt")):
        f = line.split("#", 1)[0].split()
        if f:
            if f[0] not in FIELDS:
                raise SystemExit(f"postlink.txt: unknown field {f[0]}")
            values[f[0]] = int(f[1], 0)
    return values


def stamp(src, dst):
    data = bytearray(open(src, "rb").read())
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise SystemExit(f"{src}: no PE header")
    for name, value in load_values().items():
        struct.pack_into("<I", data, pe + FIELDS[name], value)
    open(dst, "wb").write(data)
    return dst


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    src = args[0] if args else os.path.join(BUILD, "wf.exe")
    dst = args[1] if len(args) > 1 else os.path.join(BUILD, "wf-stamped.exe")
    stamp(src, dst)
    print("stamped", dst)
    if "--no-verify" not in sys.argv:
        import verify
        sys.argv = ["verify.py", dst]
        return verify.main()
    return 0


if __name__ == "__main__":
    sys.exit(main())
