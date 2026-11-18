"""Small in-place COFF edits used by the library build."""
import struct


def swap_undefined_symbols(path, a, b):
    """Swap the symbol-table positions of two undefined externals a and b
    (and the relocations that refer to them). Changes only the order in
    which LINK sees the references, not the code."""
    d = bytearray(open(path, "rb").read())
    nsec, symoff, nsym, optsz = (struct.unpack_from("<H", d, 2)[0],
                                 struct.unpack_from("<I", d, 8)[0],
                                 struct.unpack_from("<I", d, 12)[0],
                                 struct.unpack_from("<H", d, 16)[0])
    strtab = symoff + 18 * nsym
    idx = {}
    i = 0
    while i < nsym:
        e = d[symoff + 18 * i:symoff + 18 * i + 18]
        nm = e[:8]
        if nm[:4] == b"\0\0\0\0":
            o = struct.unpack_from("<I", nm, 4)[0]
            nm = d[strtab + o:d.index(b"\0", strtab + o)]
        else:
            nm = nm.rstrip(b"\0")
        sec = struct.unpack_from("<h", e, 12)[0]
        aux = e[17]
        if nm.decode("latin-1") in (a, b) and sec == 0 and aux == 0:
            idx[nm.decode("latin-1")] = i
        i += 1 + aux
    ia, ib = idx[a], idx[b]
    ea = bytes(d[symoff + 18 * ia:symoff + 18 * ia + 18])
    eb = bytes(d[symoff + 18 * ib:symoff + 18 * ib + 18])
    d[symoff + 18 * ia:symoff + 18 * ia + 18] = eb
    d[symoff + 18 * ib:symoff + 18 * ib + 18] = ea
    for s in range(nsec):
        h = 20 + optsz + 40 * s
        prel, nrel = struct.unpack_from("<I", d, h + 24)[0], struct.unpack_from("<H", d, h + 32)[0]
        for k in range(nrel):
            o = prel + 10 * k + 4
            v = struct.unpack_from("<I", d, o)[0]
            if v == ia:
                struct.pack_into("<I", d, o, ib)
            elif v == ib:
                struct.pack_into("<I", d, o, ia)
    open(path, "wb").write(bytes(d))
