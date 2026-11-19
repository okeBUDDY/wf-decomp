#!/usr/bin/env python3
"""Compile edited copies of a probe TU with /FAs and print the matching asm lines.

usage: python3 tools/fas_variants.py probe.cpp variants.py [--grep REGEX] [--only LABEL ...]

variants.py defines V = [(label, [(old, new), ...]), ...]; each variant's edits are applied
(first occurrence each) to a copy of probe.cpp, which is compiled with tools/fas.py (so it
may include src/ headers), and the instructions of every PROC matching REGEX are printed on
one line (default: imul|add|sub|or|and|lea|mov e.., WORD). This is the quick way to ablate a
function: copy it into a probe TU and cut cases or statements until a register or operand
order flips (step 37/39 notes in the README). Takes about 0.3 s per variant. Do not run it
while compare.py rewrites build/src-sjis in the same tree.
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('probe')
    ap.add_argument('variants')
    ap.add_argument('--grep', default=r'^(imul|add|sub|or|and|lea|mov\s+e.., WORD)')
    ap.add_argument('--only', nargs='*')
    args = ap.parse_args()
    with open(args.probe, encoding='utf-8') as f:
        base = f.read()
    ns = {}
    with open(args.variants) as f:
        exec(f.read(), ns)
    tmpdir = tempfile.mkdtemp(prefix='fasvar')
    for i, (label, edits) in enumerate(ns['V']):
        if args.only and label not in args.only:
            continue
        text = base
        missing = [o for o, _ in edits if o not in text]
        if missing:
            print(f'{label}: OLD NOT FOUND {missing[0][:40]!r}')
            continue
        for old, new in edits:
            text = text.replace(old, new, 1)
        path = os.path.join(tmpdir, f'_fv{i}.cpp')
        with open(path, 'w', encoding='utf-8') as f:
            f.write(text)
        out = subprocess.run([sys.executable, os.path.join(HERE, 'fas.py'), '--print', path],
                             capture_output=True, text=True).stdout
        ins = [l.strip().replace('\t', ' ') for l in out.splitlines() if l.startswith('\t')]
        hits = [x for x in ins if re.search(args.grep, x)]
        print(f'{label}: ' + (' ; '.join(hits) if hits else '(no match or compile error)'), flush=True)


if __name__ == '__main__':
    main()
