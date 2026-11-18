#!/usr/bin/env python3
"""Link build/wf.exe from game unit objects, rebuilt libraries, libcmt and the
PlatformSDK import libraries with LINK 7.10, using config/link.txt.

For each unit in config/splits.txt the compiled object build/obj/<unit>.obj
is used when the unit has a src= and was built, else build/blobs/<unit>.obj.
usage: link.py [--no-verify]
"""
import os
import sys

from splits import load_splits
from wfcommon import BUILD, CONFIG, VC71, run_wine, wine_env, winpath


def unit_objects():
    sp = load_splits()
    objs = []
    for u in sp.units:
        cand = os.path.join(BUILD, "obj", u.name + ".obj")
        if u.src and os.path.exists(cand):
            objs.append(cand)
        else:
            objs.append(os.path.join(BUILD, "blobs", u.name + ".obj"))
    return objs


def link(out=None, extra=()):
    out = out or os.path.join(BUILD, "wf.exe")
    args = []
    for line in open(os.path.join(CONFIG, "link.txt")):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        if line == "{OBJS}":
            args += [winpath(o) for o in unit_objects()]
        elif line == "wf.res":
            args.append(winpath(os.path.join(BUILD, "wf.res")))
        else:
            args.append(line)
    args += ["/OUT:" + winpath(out), "/MAP:" + winpath(os.path.splitext(out)[0] + ".map")]
    args += list(extra)
    env = wine_env([], [f"{VC71}/lib", f"{VC71}/PlatformSDK/Lib", os.path.join(BUILD, "lib")])
    rc, txt = run_wine(f"{VC71}/bin/link.exe", args, env, check=False)
    if txt.strip():
        print(txt.strip())
    if rc != 0:
        raise SystemExit(f"link failed ({rc})")
    return out


def main():
    if not os.path.exists(os.path.join(BUILD, "wf.res")):
        import extract_res
        sys.argv = ["extract_res.py", os.path.join(BUILD, "wf.res")]
        extract_res.main()
    out = link()
    print("linked", out)
    if "--no-verify" not in sys.argv:
        import verify
        sys.argv = ["verify.py", out]
        sys.exit(verify.main())


if __name__ == "__main__":
    main()
