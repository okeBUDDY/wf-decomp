#!/usr/bin/env python3
"""Rebuild a .res file from the resource section of the original wf.exe.

usage: extract_res.py out.res
The resources are written in directory order (type, name, language); the
linker's cvtres turns them back into an identical .rsrc section.
"""
import struct
import sys

import pefile

from wfcommon import ORIG_EXE


def id_field(entry):
    if entry.name is not None:
        s = str(entry.name)
        return s.upper().encode("utf-16-le") + b"\0\0"
    return struct.pack("<HH", 0xFFFF, entry.id)


def res_entry(typ, name, lang, data):
    hdr = id_field(typ) + id_field(name)
    if len(hdr) % 4:
        hdr += b"\0" * (4 - len(hdr) % 4)
    hdr += struct.pack("<IHHII", 0, 0x1030, lang, 0, 0)
    out = struct.pack("<II", len(data), 8 + len(hdr)) + hdr + data
    if len(out) % 4:
        out += b"\0" * (4 - len(out) % 4)
    return out


def main():
    pe = pefile.PE(ORIG_EXE)
    out = bytearray(struct.pack("<II", 0, 32) + struct.pack("<HHHH", 0xFFFF, 0, 0xFFFF, 0) +
                    struct.pack("<IHHII", 0, 0, 0, 0, 0))
    for t in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        for n in t.directory.entries:
            for l in n.directory.entries:
                d = l.data.struct
                data = pe.get_data(d.OffsetToData, d.Size)
                out += res_entry(t, n, l.id, data)
                print(f"type={t.name or t.id} name={n.name or n.id} lang={l.id:#x} size={d.Size}")
    open(sys.argv[1], "wb").write(out)


if __name__ == "__main__":
    main()
