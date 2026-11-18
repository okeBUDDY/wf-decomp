#!/usr/bin/env python3
"""Compile src/*.cpp and compare every function with the original exe.

Each source is compiled with VC7.1 (`cl /O2 /GX /GS`) into build/obj/src/.
Each function the object defines is looked up by name in config/symbols.txt,
then compared with the original bytes:
  * the size must be equal;
  * every byte outside relocation fields must be equal;
  * every relocation must point at the same address as the original
    (its symbol resolved through symbols.txt). Relocations to symbols with
    no known address (EH handlers, funclets, string literals, ...) are only
    masked, and are reported as "unchecked".

For objdiff, a target object holding the original bytes of the same functions
is written to build/target/<src>.obj, and objdiff.json is (re)generated.

usage: compare.py [-v] [src/foo.cpp ...]
"""
import glob
import json
import os
import struct
import sys

from coffwriter import IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32, ObjectWriter, Section, section_flags
from gensyms import read_symbols
from mkblobs import Resolver
from splits import load_splits
from wfcommon import BUILD, ORIG_EXE, PE, ROOT, VC71, Coff, run_wine, vc71_env, winpath

CFLAGS = ["/nologo", "/c", "/O2", "/GX", "/GS", "/W3"]
OBJDIR = os.path.join(BUILD, "obj", "src")
TARGETDIR = os.path.join(BUILD, "target")


def compile_source(src):
    os.makedirs(OBJDIR, exist_ok=True)
    obj = os.path.join(OBJDIR, os.path.splitext(os.path.basename(src))[0] + ".obj")
    args = CFLAGS + ["/I" + winpath(os.path.join(ROOT, "src")), "/Fo" + winpath(obj), winpath(src)]
    rc, out = run_wine(f"{VC71}/bin/cl.exe", args, vc71_env(), check=False)
    lines = [ln for ln in out.splitlines() if ln.strip() and ln.strip() != os.path.basename(src)]
    if lines:
        print("\n".join(lines))
    if rc != 0:
        raise SystemExit(f"cl failed for {src}")
    return obj


def functions(coff):
    """(name, section) for every function that starts a code section."""
    for sec in coff.sections:
        if not sec["flags"] & 0x20:
            continue
        for y in sec["syms"]:
            if y["type"] & 0x20 and y["value"] == 0 and y["cls"] in (2, 3):
                yield y["name"], sec
                break


class Comparer:
    def __init__(self):
        self.pe = PE(ORIG_EXE)
        self.syms = read_symbols()
        self.by_name = {}
        for a, s in self.syms.items():
            self.by_name.setdefault(s["name"], a)

    def compare(self, coff, name, sec, addr):
        size = self.syms[addr]["size"]
        data = sec["data"]
        notes = []
        if len(data) != size:
            notes.append(f"size {len(data):#x} != {size:#x}")
        orig = self.pe.read(addr, max(size, len(data)))
        mask = bytearray(len(data))
        unchecked = 0
        bad_relocs = 0
        for off, symidx, typ in sec["rels"]:
            mask[off:off + 4] = b"\1\1\1\1"
            target_name = coff.symlist_by_index[symidx]["name"]
            if off + 4 > len(orig):
                continue
            addend = struct.unpack_from("<I", data, off)[0]
            ov = struct.unpack_from("<I", orig, off)[0]
            if typ == IMAGE_REL_I386_DIR32:
                want = (ov - addend) & 0xFFFFFFFF
            elif typ == IMAGE_REL_I386_REL32:
                want = (addr + off + 4 + ov - addend) & 0xFFFFFFFF
            else:
                continue
            got = self.by_name.get(target_name)
            if got is None:
                unchecked += 1
            elif got != want:
                bad_relocs += 1
                notes.append(f"reloc +{off:#x} {target_name} -> {got:#x}, original {want:#x}")
        diff = sum(1 for i in range(min(len(data), size)) if not mask[i] and data[i] != orig[i])
        if diff:
            notes.append(f"{diff} bytes differ")
        ok = not notes
        return ok, unchecked, notes


def write_target(path, coff, matched, pe, res):
    """Object with the original bytes of the functions in `matched`."""
    w = ObjectWriter()
    for name, addr, size in matched:
        sec = w.add_section(Section(".text", bytearray(pe.read(addr, size)), section_flags(".text", 16)))
        w.define(name, sec, 0, func=True)
        for at, kind, t in res.relocs_in(addr, addr + size):
            tname, addend = res.resolve(t)
            if addend is None:
                continue
            off = at - addr
            if kind == "dir32":
                sec.data[off:off + 4] = (addend & 0xFFFFFFFF).to_bytes(4, "little")
                sec.relocs.append((off, tname, IMAGE_REL_I386_DIR32))
            else:
                sec.data[off:off + 4] = (addend & 0xFFFFFFFF).to_bytes(4, "little")
                sec.relocs.append((off, tname, IMAGE_REL_I386_REL32))
    open(path, "wb").write(w.build())


class RelocIndex(Resolver):
    def __init__(self, splits, syms, an):
        super().__init__(splits, syms)
        self.relocs = sorted(an["relocs"])

    def relocs_in(self, lo, hi):
        return [r for r in self.relocs if lo <= r[0] and r[0] + 4 <= hi]


def main():
    verbose = "-v" in sys.argv
    srcs = [a for a in sys.argv[1:] if not a.startswith("-")]
    srcs = srcs or sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True))
    cmp = Comparer()
    an = json.load(open(os.path.join(BUILD, "analysis.json")))
    res = RelocIndex(load_splits(), cmp.syms, an)
    os.makedirs(TARGETDIR, exist_ok=True)
    units = []
    total_match = total_funcs = 0
    matched_bytes = 0
    report = []
    for src in srcs:
        obj = compile_source(src)
        coff = Coff(open(obj, "rb").read(), obj)
        coff.symlist_by_index = {y["index"]: y for y in coff.symlist}
        found = []
        for name, sec in functions(coff):
            if name.startswith("$"):  # EH funclet (text$x), static label
                continue
            addr = cmp.by_name.get(name)
            if addr is None or cmp.syms[addr]["type"] != "func":
                report.append((os.path.basename(src), name, None, "no address in symbols.txt", []))
                continue
            ok, unchecked, notes = cmp.compare(coff, name, sec, addr)
            total_funcs += 1
            if ok:
                total_match += 1
                matched_bytes += cmp.syms[addr]["size"]
            status = "OK" if ok else "DIFF"
            if ok and unchecked:
                status += f" ({unchecked} relocs unchecked)"
            report.append((os.path.basename(src), name, addr, status, notes))
            found.append((name, addr, cmp.syms[addr]["size"]))
        unit = os.path.splitext(os.path.relpath(src, os.path.join(ROOT, "src")))[0].replace(os.sep, "/")
        target = os.path.join(TARGETDIR, unit.replace("/", "_") + ".obj")
        write_target(target, coff, found, cmp.pe, res)
        units.append(dict(name=unit, target_path=os.path.relpath(target, ROOT),
                          base_path=os.path.relpath(obj, ROOT)))
    for srcname, name, addr, status, notes in report:
        loc = f"{addr:08x}" if addr is not None else "--------"
        print(f"{loc}  {status:<28} {srcname:<16} {name}")
        if notes and (verbose or len(notes) <= 3):
            for n in notes:
                print(f"            {n}")
    game_funcs = [(a, s) for a, s in cmp.syms.items() if s["type"] == "func"]
    game_bytes = sum(s["size"] for _, s in game_funcs)
    print(f"\n{total_match}/{total_funcs} compiled functions match; "
          f"{total_match}/{len(game_funcs)} game functions "
          f"({100.0 * matched_bytes / game_bytes:.2f}% of {game_bytes} code bytes) matched")
    cfg = {
        "min_version": "2.0.0",
        "build_target": False,
        "build_base": False,  # run tools/compare.py, which also compiles
        "watch_patterns": ["src/**/*.cpp", "src/**/*.h"],
        "units": units,
    }
    json.dump(cfg, open(os.path.join(ROOT, "objdiff.json"), "w"), indent=2)
    open(os.path.join(BUILD, "progress.json"), "w").write(json.dumps(dict(
        functions_matched=total_match, functions_total=len(game_funcs),
        bytes_matched=matched_bytes, bytes_total=game_bytes)))


if __name__ == "__main__":
    main()
