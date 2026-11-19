#!/usr/bin/env python3
"""compile.sh for the permuter: compile.py meta.json input.c -o output.o

Turns the candidate back into C++ (wfperm.from_c), splices it into a copy of
its source file next to the cp932 mirror (build/src-sjis), compiles it with the
game flags and writes the function's resolved bytes as an ELF object."""
import json
import os
import re
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(HERE))

import wfperm  # noqa: E402
from compare import CL_LOCALE, LIB_INCLUDES, cflags_for, SJIS_DIR, mirror_sources  # noqa: E402
from wfcommon import BUILD, ORIG_EXE, PE, ROOT, VC71, Coff, run_wine, vc71_env, winpath  # noqa: E402
from gensyms import read_symbols  # noqa: E402


def main():
    meta_path, cand = sys.argv[1], sys.argv[2]
    out = sys.argv[sys.argv.index("-o") + 1]
    meta = json.load(open(meta_path))
    cpp = wfperm.from_c(open(cand, encoding="utf-8").read(), meta["func_name"], meta["header"])
    src = os.path.join(ROOT, meta["src"])
    text = open(src, encoding="utf-8").read()
    start, brace, end = wfperm.find_function(text, meta["addr"])
    text = text[:start] + cpp + text[end:]
    sjis = os.path.join(SJIS_DIR, os.path.relpath(src, os.path.join(ROOT, "src")))
    if not os.path.exists(sjis):
        mirror_sources()
    tmpdir = os.path.join(BUILD, "permuter", "tmp")
    os.makedirs(tmpdir, exist_ok=True)
    fd, obj = tempfile.mkstemp(suffix=".obj", dir=tmpdir)
    os.close(fd)
    # The mirror copy's directory goes first on the include path, so "..."
    # includes resolve as they do for the real file (compare.py's
    # mirror_sources would delete a file placed in the mirror).
    cfile = obj[:-4] + ".cpp"
    try:
        open(cfile, "wb").write(text.encode("cp932"))
        incs = [os.path.dirname(sjis), SJIS_DIR] + [os.path.join(BUILD, "lib", "src", d) for d in LIB_INCLUDES]
        args = cflags_for(src) + ["/I" + winpath(i) for i in incs] + ["/Fo" + winpath(obj), winpath(cfile)]
        rc, log = run_wine(f"{VC71}/bin/cl.exe", args, dict(vc71_env(), **CL_LOCALE), check=False)
        if rc != 0:
            sys.stderr.write(log)
            sys.exit(1)
        coff = Coff(open(obj, "rb").read(), obj)
    finally:
        for p in (cfile, obj):
            if os.path.exists(p):
                os.remove(p)
    names = {y["index"]: y["name"] for y in coff.symlist}
    sec = None
    for s in coff.sections:
        if s["flags"] & 0x20 and any(y["name"] == meta["symbol"] and y["value"] == 0 for y in s["syms"]):
            sec = s
    if sec is None:
        sys.exit(f"{meta['symbol']} not in the object")
    by_name = {}
    for a, y in read_symbols().items():
        by_name.setdefault(y["name"], a)
    rels = [(off, names[i], "dir32" if typ == 6 else "rel32") for off, i, typ in sec["rels"]
            if typ in (6, 20)]
    orig = PE(ORIG_EXE).read(meta["addr"], meta["size"])
    code = wfperm.resolve(sec["data"], rels, by_name.get, meta["addr"], orig)
    wfperm.write_elf(code, out)


if __name__ == "__main__":
    main()
