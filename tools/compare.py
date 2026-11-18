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
import re
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
# Third-party headers (unpacked by tools/build_libs.py).
LIB_INCLUDES = ["libogg-1.0/include", "libvorbis-1.0/include", "zlib-1.1.4", "libpng-1.2.5"]
OBJDIR = os.path.join(BUILD, "obj", "src")
TARGETDIR = os.path.join(BUILD, "target")


# The sources are UTF-8. cl gets a Shift-JIS (cp932) copy of src/ in
# SJIS_DIR, run with Wine's ANSI code page 932 so that it lexes the
# double-byte characters (a trail byte of 0x5c is not a backslash).
SRC_DIR = os.path.join(ROOT, "src")
SJIS_DIR = os.path.join(BUILD, "src-sjis")
CL_LOCALE = {"LANG": "ja_JP.UTF-8", "LC_ALL": "ja_JP.UTF-8"}


def to_cp932(path):
    """The cp932 bytes of the UTF-8 file `path`; fails on anything else."""
    data = open(path, "rb").read()
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as e:
        raise SystemExit(f"{path}: not UTF-8 (byte {e.start})")
    out = []
    for no, line in enumerate(text.splitlines(keepends=True), 1):
        try:
            out.append(line.encode("cp932"))
        except UnicodeEncodeError as e:
            raise SystemExit(f"{path}:{no}: {line[e.start:e.end]!r} (U+{ord(line[e.start]):04X}) "
                             "has no cp932 encoding")
    return b"".join(out)


def mirror_sources():
    """Writes the cp932 copy of src/ to SJIS_DIR (only files that changed)."""
    keep = set()
    for root, _, files in os.walk(SRC_DIR):
        for f in files:
            src = os.path.join(root, f)
            rel = os.path.relpath(src, SRC_DIR)
            dst = os.path.join(SJIS_DIR, rel)
            keep.add(dst)
            data = to_cp932(src)
            if os.path.exists(dst) and open(dst, "rb").read() == data:
                continue
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            open(dst, "wb").write(data)
    for root, _, files in os.walk(SJIS_DIR):
        for f in files:
            if os.path.join(root, f) not in keep:
                os.remove(os.path.join(root, f))


def compile_source(src):
    os.makedirs(OBJDIR, exist_ok=True)
    obj = os.path.join(OBJDIR, os.path.splitext(os.path.basename(src))[0] + ".obj")
    sjis = os.path.join(SJIS_DIR, os.path.relpath(os.path.abspath(src), SRC_DIR))
    if not os.path.exists(sjis):
        raise SystemExit(f"{src}: not under src/")
    incs = [SJIS_DIR] + [os.path.join(BUILD, "lib", "src", d) for d in LIB_INCLUDES]
    args = CFLAGS + ["/I" + winpath(i) for i in incs] + ["/Fo" + winpath(obj), winpath(sjis)]
    env = dict(vc71_env(), **CL_LOCALE)
    rc, out = run_wine(f"{VC71}/bin/cl.exe", args, env, check=False)
    out = re.sub(re.escape(winpath(SJIS_DIR)), winpath(SRC_DIR).replace("\\", "\\\\"), out,
                 flags=re.IGNORECASE)
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
        return self.compare_range(coff, sec, 0, len(sec["data"]), addr)

    def compare_range(self, coff, sec, lo, hi, addr):
        """Compares sec["data"][lo:hi] with the original function at `addr`."""
        size = self.syms[addr]["size"]
        data = sec["data"][lo:hi]
        notes = []
        if len(data) != size:
            notes.append(f"size {len(data):#x} != {size:#x}")
        orig = self.pe.read(addr, max(size, len(data)))
        mask = bytearray(len(data))
        unchecked = 0
        bad_relocs = 0
        for off, symidx, typ in sec["rels"]:
            if not lo <= off < hi:
                continue
            off -= lo
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
        diffs = [i for i in range(min(len(data), size)) if not mask[i] and data[i] != orig[i]]
        diff = len(diffs)
        if diff and VERBOSE:
            notes.append("at " + " ".join(f"+{i:#x}" for i in diffs[:8]))
        if diff:
            notes.append(f"{diff} bytes differ")
        ok = not notes
        return ok, unchecked, notes

    def original_target(self, sec, addr, coff, target):
        """The original address the DIR32 reloc to `target` in the function
        at `addr` holds, or None."""
        orig = self.pe.read(addr, len(sec["data"]))
        for off, symidx, typ in sec["rels"]:
            if typ == IMAGE_REL_I386_DIR32 and coff.symlist_by_index[symidx]["name"] == target:
                if off + 4 <= len(orig):
                    ov = struct.unpack_from("<I", orig, off)[0]
                    return (ov - struct.unpack_from("<I", sec["data"], off)[0]) & 0xFFFFFFFF
        return None


def section_pieces(sec):
    """(name, lo, hi) of each funclet/thunk the symbols of `sec` start."""
    starts = sorted({(y["value"], y["name"]) for y in sec["syms"] if y["cls"] in (2, 3)
                     and not y["name"].startswith(".")})
    out = []
    for i, (v, n) in enumerate(starts):
        hi = starts[i + 1][0] if i + 1 < len(starts) else len(sec["data"])
        if hi > v:
            out.append((n, v, hi))
    return out


def match_stubs(cmp, coff, stubs, matched_stubs):
    """Marks the EH stubs (.text$x funclets, .text$yc/$yd init and atexit
    thunks) that `coff` reproduces byte for byte (relocations masked, named
    targets checked).

    A .text$x section is placed by its parent function's push of
    __ehhandler$<parent>, whose original value gives the handler's address.
    The $yc/$yd thunks have no such anchor; one counts when it equals a
    stub of its size whose checked relocations all agree (at least one)."""
    texts = {}
    for sec in coff.sections:
        if sec["name"] == ".text":
            for y in sec["syms"]:
                if y["type"] & 0x20 and y["value"] == 0 and y["cls"] in (2, 3):
                    texts[y["name"]] = sec
                    break
    for sec in coff.sections:
        name = sec["name"]
        if name == ".text$x":
            pieces = section_pieces(sec)
            handler = [p for p in pieces if p[0].startswith("__ehhandler$")]
            if not handler:
                continue
            hname, hlo, _ = handler[0]
            parent = hname[len("__ehhandler$"):]
            paddr = cmp.by_name.get(parent)
            if parent not in texts or paddr is None:
                continue
            haddr = cmp.original_target(texts[parent], paddr, coff, hname)
            if haddr is None:
                continue
            base = haddr - hlo
            for _, lo, hi in pieces:
                a = base + lo
                if a in stubs:
                    ok, _, _ = cmp.compare_range(coff, sec, lo, hi, a)
                    if ok:
                        matched_stubs.add(a)
        elif name in (".text$yc", ".text$yd"):
            for _, lo, hi in section_pieces(sec):
                for a in sorted(stubs):
                    if a in matched_stubs or cmp.syms[a]["size"] != hi - lo:
                        continue
                    ok, unchecked, _ = cmp.compare_range(coff, sec, lo, hi, a)
                    nrel = sum(1 for r in sec["rels"] if lo <= r[0] < hi)
                    if ok and nrel - unchecked >= 1:
                        matched_stubs.add(a)
                        break


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


VERBOSE = "-v" in sys.argv


def main():
    verbose = VERBOSE
    srcs = [a for a in sys.argv[1:] if not a.startswith("-")]
    srcs = srcs or sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True))
    cmp = Comparer()
    an = json.load(open(os.path.join(BUILD, "analysis.json")))
    res = RelocIndex(load_splits(), cmp.syms, an)
    os.makedirs(TARGETDIR, exist_ok=True)
    units = []
    seen, matched = set(), set()
    splits = load_splits()
    in_text = lambda a: any(sec == ".text" and lo <= a < hi
                            for u in splits.units for sec, lo, hi in u.ranges)
    stubs = {a for a, y in cmp.syms.items() if y["type"] == "func" and not in_text(a)}
    matched_stubs = set()
    report = []
    mirror_sources()
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
            seen.add(addr)
            if ok:
                matched.add(addr)
            status = "OK" if ok else "DIFF"
            if ok and unchecked:
                status += f" ({unchecked} relocs unchecked)"
            report.append((os.path.basename(src), name, addr, status, notes))
            found.append((name, addr, cmp.syms[addr]["size"]))
        match_stubs(cmp, coff, stubs, matched_stubs)
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
    game_funcs = [(a, y) for a, y in cmp.syms.items() if y["type"] == "func" and a not in stubs]
    game_bytes = sum(y["size"] for _, y in game_funcs)
    total_match, total_funcs = len(matched - stubs), len(seen - stubs)
    matched_bytes = sum(cmp.syms[a]["size"] for a in matched - stubs)
    print(f"\n{total_match}/{total_funcs} compiled functions match; "
          f"{total_match}/{len(game_funcs)} game functions "
          f"({100.0 * matched_bytes / game_bytes:.2f}% of {game_bytes} code bytes) matched; "
          f"EH stubs {len(matched_stubs)}/{len(stubs)}")
    cfg = {
        "min_version": "2.0.0",
        "build_target": False,
        "build_base": False,  # run tools/compare.py, which also compiles
        "watch_patterns": ["src/**/*.cpp", "src/**/*.h"],
        "units": units,
    }
    json.dump(cfg, open(os.path.join(ROOT, "objdiff.json"), "w"), indent=2)
    json.dump(sorted(matched), open(os.path.join(BUILD, "matched.json"), "w"))
    open(os.path.join(BUILD, "progress.json"), "w").write(json.dumps(dict(
        functions_matched=total_match, functions_total=len(game_funcs),
        bytes_matched=matched_bytes, bytes_total=game_bytes,
        eh_stubs_matched=len(matched_stubs), eh_stubs_total=len(stubs),
        eh_stubs_bytes_matched=sum(cmp.syms[a]["size"] for a in matched_stubs),
        eh_stubs_bytes_total=sum(cmp.syms[a]["size"] for a in stubs))))


if __name__ == "__main__":
    main()
