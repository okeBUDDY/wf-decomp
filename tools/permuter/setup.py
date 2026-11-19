#!/usr/bin/env python3
"""Sets up a decomp-permuter directory for one function.

usage: setup.py <src/file.cpp> <address|mangled name> [dir]

Writes <dir> (default build/permuter/<address>/) with base.c (the C rendering,
see wfperm.py), target.o, compile.sh, settings.toml and meta.json. Check that
`compile.sh base.c -o base.o` reproduces the current draft's score, then run
  python3 $WF_PERMUTER/permuter.py <dir> -j8
Port any improvement back to natural C++ in src/ by hand and confirm it with
tools/compare.py.
"""
import json
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(HERE))

import wfperm  # noqa: E402
from gensyms import read_symbols  # noqa: E402
from wfcommon import BUILD, ORIG_EXE, PE, ROOT  # noqa: E402


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    src = os.path.relpath(os.path.abspath(sys.argv[1]), ROOT)
    syms = read_symbols()
    key = sys.argv[2]
    try:
        addr = int(key, 16)
    except ValueError:
        addr = next(a for a, y in syms.items() if y["name"] == key)
    sym = syms[addr]
    d = sys.argv[3] if len(sys.argv) > 3 else os.path.join(BUILD, "permuter", f"{addr:08x}")
    os.makedirs(d, exist_ok=True)
    text = open(os.path.join(ROOT, src), encoding="utf-8").read()
    start, brace, end = wfperm.find_function(text, addr)
    header = text[start:brace]
    cname = header.split("(")[0].split()[-1].lstrip("*&").replace("::", "__")
    open(os.path.join(d, "base.c"), "w", encoding="utf-8").write(
        wfperm.to_c(header, text[brace:end]))
    meta = dict(src=src, addr=addr, size=sym["size"], symbol=sym["name"],
                func_name=cname, header=header)
    json.dump(meta, open(os.path.join(d, "meta.json"), "w"), indent=1)
    open(os.path.join(d, "settings.toml"), "w").write(
        f'func_name = "{cname}"\ncompiler_type = "base"\n\n'
        "# Passes whose output cannot be turned back into C++ here.\n"
        "[weight_overrides]\nperm_inline = 0\nperm_randomize_external_type = 0\n"
        "perm_randomize_function_type = 0\nperm_sameline = 0\n")
    sh = os.path.join(d, "compile.sh")
    open(sh, "w").write(
        "#!/bin/sh\n"
        f'exec python3 "{os.path.join(HERE, "compile.py")}" "$(dirname "$0")/meta.json" "$@"\n')
    os.chmod(sh, 0o755)
    orig = PE(ORIG_EXE).read(addr, sym["size"])
    wfperm.write_elf(orig, os.path.join(d, "target.o"))
    print(f"{d}: {sym['name']} as {cname}")


if __name__ == "__main__":
    main()
