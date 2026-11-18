"""Minimal i386 COFF object writer."""
import struct

IMAGE_REL_I386_DIR32 = 0x0006
IMAGE_REL_I386_REL32 = 0x0014

SCN_CNT_CODE = 0x00000020
SCN_CNT_INITIALIZED_DATA = 0x00000040
SCN_CNT_UNINITIALIZED_DATA = 0x00000080
SCN_LNK_INFO = 0x00000200
SCN_LNK_REMOVE = 0x00000800
SCN_LNK_COMDAT = 0x00001000
SCN_MEM_EXECUTE = 0x20000000
SCN_MEM_READ = 0x40000000
SCN_MEM_WRITE = 0x80000000

ALIGN_FLAGS = {1: 0x00100000, 2: 0x00200000, 4: 0x00300000, 8: 0x00400000, 16: 0x00500000,
               32: 0x00600000, 64: 0x00700000}


def section_flags(name, align):
    base = name.split("$", 1)[0]
    if base == ".text":
        fl = SCN_CNT_CODE | SCN_MEM_EXECUTE | SCN_MEM_READ
    elif base == ".bss":
        fl = SCN_CNT_UNINITIALIZED_DATA | SCN_MEM_READ | SCN_MEM_WRITE
    elif base in (".rdata", ".xdata"):
        fl = SCN_CNT_INITIALIZED_DATA | SCN_MEM_READ
    elif base == ".drectve":
        return SCN_LNK_INFO | SCN_LNK_REMOVE | ALIGN_FLAGS[1]
    else:  # .data, .CRT
        fl = SCN_CNT_INITIALIZED_DATA | SCN_MEM_READ | SCN_MEM_WRITE
    return fl | ALIGN_FLAGS[align]


class Section:
    def __init__(self, name, data, flags, size=None, comdat=None):
        self.name = name
        self.data = data  # bytes, or None for uninitialized
        self.size = len(data) if data is not None else size
        self.flags = flags
        self.relocs = []  # (offset, symbol name, type)
        self.comdat = comdat  # selection number or None
        self.index = 0


class ObjectWriter:
    def __init__(self):
        self.sections = []
        self.symbols = []  # (name, value, section obj|None|-1, cls, is_func)
        self.defined = {}
        self.undef_key = {}  # name -> sort key for undefined externals
        self.phantoms = []  # undefined externals not referenced by relocations

    def add_section(self, sec):
        self.sections.append(sec)
        sec.index = len(self.sections)
        return sec

    def define(self, name, sec, value, external=True, func=False):
        if name in self.defined:
            raise ValueError(f"duplicate symbol {name}")
        self.defined[name] = True
        self.symbols.append((name, value, sec, 2 if external else 3, func))

    def absolute(self, name, value):
        self.symbols.append((name, value, -1, 3, False))
        self.defined[name] = True

    def build(self):
        # symbol table: absolute syms, section syms (with aux for COMDAT),
        # defined syms, then undefined externals referenced by relocations.
        order = []
        for s in self.symbols:
            if s[2] == -1:
                order.append(("sym", s))
        for sec in self.sections:
            order.append(("sec", sec))
        for s in self.symbols:
            if s[2] != -1:
                order.append(("sym", s))
        undef = []
        for sec in self.sections:
            for _, name, _ in sec.relocs:
                if name not in self.defined and name not in undef:
                    undef.append(name)
        for name in self.phantoms:
            if name not in self.defined and name not in undef:
                undef.append(name)
        # The linker pulls library members in the order of its undefined
        # symbol list, so the order of undefined externals matters.
        undef.sort(key=lambda n: self.undef_key.get(n, 1 << 32))
        strtab = bytearray()
        symtab = bytearray()
        index = {}
        nsym = 0

        def name_field(n):
            b = n.encode("latin-1")
            if len(b) <= 8:
                return b.ljust(8, b"\0")
            off = len(strtab) + 4
            strtab.extend(b + b"\0")
            return struct.pack("<II", 0, off)

        for kind, obj in order:
            if kind == "sec":
                sec = obj
                naux = 1
                symtab += name_field(sec.name) + struct.pack("<IhHBB", 0, sec.index, 0, 3, naux)
                nrel = len(sec.relocs)
                symtab += struct.pack("<IHHIHB3x", sec.size, min(nrel, 0xFFFF), 0, 0,
                                      0, sec.comdat or 0)
                index["$sec" + str(sec.index)] = nsym
                nsym += 2
            else:
                name, value, sec, cls, func = obj
                secno = -1 if sec == -1 else sec.index
                symtab += name_field(name) + struct.pack("<IhHBB", value, secno,
                                                         0x20 if func else 0, cls, 0)
                index[name] = nsym
                nsym += 1
        for name in undef:
            symtab += name_field(name) + struct.pack("<IhHBB", 0, 0, 0x20, 2, 0)
            index[name] = nsym
            nsym += 1

        nsec = len(self.sections)
        hdr_size = 20 + 40 * nsec
        body = bytearray()
        sechdrs = bytearray()
        off = hdr_size
        for sec in self.sections:
            raw_ptr = 0
            if sec.data is not None and sec.size:
                raw_ptr = off + len(body)
                body += sec.data
            rel_ptr = 0
            if sec.relocs:
                rel_ptr = off + len(body)
                for o, name, typ in sorted(sec.relocs):
                    body += struct.pack("<IIH", o, index[name], typ)
            if len(sec.relocs) > 0xFFFF:
                raise ValueError("too many relocations")
            sechdrs += sec.name.encode().ljust(8, b"\0")[:8] if len(sec.name) <= 8 else \
                ("/%d" % self._long_section_name(sec.name, strtab)).encode().ljust(8, b"\0")
            sechdrs += struct.pack("<IIIIIIHHI", 0, 0, sec.size, raw_ptr, rel_ptr, 0,
                                   len(sec.relocs), 0, sec.flags)
        symoff = off + len(body)
        out = struct.pack("<HHIIIHH", 0x14C, nsec, 0, symoff, nsym, 0, 0)
        out += sechdrs + body + symtab + struct.pack("<I", len(strtab) + 4) + strtab
        return bytes(out)

    @staticmethod
    def _long_section_name(name, strtab):
        off = len(strtab) + 4
        strtab.extend(name.encode() + b"\0")
        return off
