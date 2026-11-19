"""Compile one source file with /FAs and write build/fas/<name>.asm (and .obj).

usage: python3 tools/fas.py [--print] <file.cpp>

The file may be a source in src/ or a standalone probe TU anywhere (it is compiled from
build/src-sjis/_probe/, so `#include "hl_3d.h"` and the like resolve against src/).
`--print` prints the code of every PROC ... ENDP without comment lines.

The listing shows each function's stack slots (`_x$ = 16`, `tvNNN = -4`) above its PROC
line, which answers slot questions without a link or compare run.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import compare  # noqa: E402
from wfcommon import BUILD, VC71, ensure_wineserver, run_wine, vc71_env, winpath  # noqa: E402

ROOT = os.path.dirname(HERE)
OUT = os.path.join(BUILD, 'fas')


def compile_fas(src):
    src = os.path.abspath(src)
    rel = os.path.relpath(src, os.path.join(ROOT, 'src'))
    if rel.startswith('..'):
        rel = os.path.join('_probe', os.path.basename(src))
    sjis = os.path.join(BUILD, 'src-sjis', rel)
    os.makedirs(os.path.dirname(sjis), exist_ok=True)
    os.makedirs(OUT, exist_ok=True)
    with open(src, encoding='utf-8') as f:
        text = f.read()
    with open(sjis, 'wb') as f:
        f.write(text.encode('cp932'))
    env = vc71_env()
    ensure_wineserver(env)
    out = os.path.join(OUT, os.path.splitext(os.path.basename(src))[0])
    incs = [os.path.join(BUILD, 'src-sjis')]
    args = compare.cflags_for(src) + ['/I' + winpath(i) for i in incs] + [
        '/FAs', '/Fa' + winpath(out + '.asm'), '/Fo' + winpath(out + '.obj'), winpath(sjis)]
    rc, o = run_wine(f"{VC71}/bin/cl.exe", args, env, check=False)
    return rc, o, out + '.asm'


def proc_text(asm):
    """The PROC ... ENDP blocks of a listing, without comment and blank lines."""
    out, inside = [], False
    with open(asm, encoding='cp932', errors='replace') as f:
        for line in f:
            s = line.rstrip()
            if ' PROC' in s:
                inside = True
            if inside and s.strip() and not s.lstrip().startswith(';'):
                out.append(s)
            if ' ENDP' in s:
                inside = False
    return '\n'.join(out)


if __name__ == '__main__':
    args = sys.argv[1:]
    show = '--print' in args
    args = [a for a in args if a != '--print']
    rc, o, asm = compile_fas(args[0])
    if rc:
        print(o[-2000:])
    else:
        print(proc_text(asm) if show else asm)
    sys.exit(1 if rc else 0)
