"""Print the /FAs stack-slot table of one function for a list of source variants.

usage: python3 tools/fas_slots.py src/file.cpp FUNCSUBSTR < variants

The variants use the try_variants.py format: `old\n----\nnew`, separated by `\n====\n`.
Each is applied alone, compiled with tools/fas.py, and the `name$ = off` / `tvNNN = off`
lines above the function's PROC are printed on one line. The source is restored afterwards.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from fas import compile_fas  # noqa: E402


def slots(asm_path, func):
    with open(asm_path, encoding='cp932', errors='replace') as f:
        asm = f.read()
    k = asm.find(func)
    k = asm.find('PROC', k)
    start = asm.rfind('_TEXT\tSEGMENT', 0, k)
    lines = [ln for ln in asm[start:k].splitlines() if '$ = ' in ln or ln.startswith('tv')]
    return ' '.join(ln.split(';')[0].replace('\t', '').strip() for ln in lines)


def main():
    path, func = sys.argv[1], sys.argv[2]
    with open(path, encoding='utf-8') as f:
        orig = f.read()
    vs = [v.split('\n----\n') for v in sys.stdin.read().split('\n====\n')]
    try:
        for i, (old, new) in enumerate(vs):
            old, new = old.strip('\n'), new.strip('\n')
            if old not in orig:
                print(i, 'OLD NOT FOUND')
                continue
            with open(path, 'w', encoding='utf-8') as f:
                f.write(orig.replace(old, new, 1))
            rc, o, asm = compile_fas(path)
            if rc:
                print(i, 'ERR', o[-300:])
                continue
            print(i, slots(asm, func))
    finally:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(orig)


if __name__ == '__main__':
    main()
