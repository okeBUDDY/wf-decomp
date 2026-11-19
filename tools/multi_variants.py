"""Try variants that each apply several edits at once, scoring several functions.

usage: python3 tools/multi_variants.py src/file.cpp addr1,addr2,... variants.py [--base]

variants.py defines V = [[(old, new), ...], ...]; every variant's edits are applied together
(in order, first occurrence each); an edit written as (path, old, new) applies to another file,
such as a header the source includes. The file is compiled with compare.py, and each address is
reported as OK or <differing bytes>b/<differing instructions>i (fdiff.py -n). --base scores
the unchanged source first. The source is restored afterwards. compare.py overwrites
build/matched.json, so run a full compare before mkblobs/link.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    path, addrs, vf = sys.argv[1], sys.argv[2].split(','), sys.argv[3]
    ns = {}
    with open(vf) as f:
        exec(f.read(), ns)
    variants = ns['V']
    with open(path, encoding='utf-8') as f:
        orig = f.read()
    env = dict(os.environ, WINEDEBUG='-all', WINEPREFIX=os.environ.get('WINEPREFIX', '/root/.wine-wf'),
               LANG='ja_JP.UTF-8')

    def score():
        out = subprocess.run([sys.executable, ROOT + '/tools/compare.py', '-v', path], cwd=ROOT,
                             capture_output=True, text=True, env=env).stdout
        res = []
        for a in addrs:
            a = a.lower().removeprefix('0x').zfill(8)
            m = re.search(a + r"\s+(\S+).*?\n((?:            .*\n)*)", out)
            if not m:
                res.append(a[-5:] + ':ERR')
                continue
            if m.group(1) == 'OK':
                res.append(a[-5:] + ':OK')
                continue
            fd = subprocess.run([sys.executable, ROOT + '/tools/fdiff.py', '-n', a], cwd=ROOT,
                                capture_output=True, text=True, env=env).stdout
            n = re.search(r"(\d+) instructions differ", fd)
            d = re.search(r"(\d+) bytes differ", m.group(2))
            res.append(f"{a[-5:]}:{d.group(1) if d else '?'}b/{n.group(1) if n else '?'}i")
        return ' '.join(res)

    originals = {path: orig}

    def restore():
        for p, text in originals.items():
            with open(p, 'w', encoding='utf-8') as f:
                f.write(text)

    try:
        if '--base' in sys.argv:
            print('base', score())
        for i, v in enumerate(variants):
            texts = {path: orig}
            bad = False
            for edit in v:
                p, old, new = edit if len(edit) == 3 else (path, *edit)
                if p not in originals:
                    with open(p, encoding='utf-8') as f:
                        originals[p] = f.read()
                s = texts.get(p, originals[p])
                if old not in s:
                    print(i, 'OLD NOT FOUND', p, old[:60])
                    bad = True
                    break
                texts[p] = s.replace(old, new, 1)
            if bad:
                continue
            restore()
            for p, text in texts.items():
                with open(p, 'w', encoding='utf-8') as f:
                    f.write(text)
            print(i, score(), flush=True)
    finally:
        restore()


if __name__ == '__main__':
    main()
