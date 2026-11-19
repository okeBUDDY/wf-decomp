#!/usr/bin/env python3
"""Tries source edits on one function and reports its compare.py result for each.

usage: try_variants.py src/file.cpp ADDR [--compile src/other.cpp] < variants

stdin holds variants separated by lines '===='; each is 'old text', a line
'----', then 'new text'. Each variant is applied alone to a pristine copy of
the file, compiled and compared; the file is restored at the end. With
--compile, the edited file (a header) is checked by compiling the given
source instead. Note that a single-file compare.py run rewrites
build/matched.json and objdiff.json.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def run(path, addr, variants, compile_path=None):
    orig = open(path, encoding="utf-8").read()
    env = dict(os.environ, WINEDEBUG="-all", WINEPREFIX=os.environ.get("WINEPREFIX", "/root/.wine-wf"),
               LANG="ja_JP.UTF-8")
    results = []
    try:
        for old, new in variants:
            if old not in orig:
                results.append("OLD NOT FOUND")
                continue
            open(path, "w", encoding="utf-8").write(orig.replace(old, new, 1))
            out = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "compare.py"), "-v", compile_path or path],
                                 cwd=ROOT, capture_output=True, text=True, env=env).stdout
            m = re.search(addr + r"\s+(\S+).*?\n((?:            .*\n)*)", out)
            if not m:
                results.append("compile error")
                continue
            d = re.search(r"(\d+) bytes differ", m.group(2))
            sz = re.search(r"size \S+ != \S+", m.group(2))
            results.append(" ".join(x for x in (m.group(1), d.group(1) if d else "",
                                                sz.group(0) if sz else "") if x))
    finally:
        open(path, "w", encoding="utf-8").write(orig)
    return results


def main():
    path, addr = sys.argv[1], sys.argv[2].lower().removeprefix("0x").zfill(8)
    compile_path = sys.argv[sys.argv.index("--compile") + 1] if "--compile" in sys.argv else None
    variants = []
    for v in sys.stdin.read().split("\n====\n"):
        old, new = v.split("\n----\n")
        variants.append((old.strip("\n"), new.strip("\n")))
    for i, r in enumerate(run(path, addr, variants, compile_path)):
        print(i, r)


if __name__ == "__main__":
    main()
