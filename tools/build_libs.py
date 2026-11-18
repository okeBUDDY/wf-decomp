#!/usr/bin/env python3
"""Build the third-party static libraries (libogg, libvorbis, vorbisfile, zlib,
libpng) with VS.NET 2002 under Wine, as described in config/libs.json.

Output: build/lib/<name>/*.obj and build/lib/<name>.lib
usage: build_libs.py [--only name,...] [--flags "/O1 ..."] [-j N]
"""
import argparse
import concurrent.futures
import json
import os
import shutil
import tarfile

from coffpatch import swap_undefined_symbols
from wfcommon import BUILD, CONFIG, THIRDPARTY, VC70, VC71, run_wine, wine_env, winpath

SRC = os.path.join(BUILD, "lib", "src")


def extract(cfg):
    dest = os.path.join(SRC, cfg["dir"])
    if not os.path.isdir(dest):
        os.makedirs(SRC, exist_ok=True)
        with tarfile.open(os.path.join(THIRDPARTY, cfg["tarball"])) as t:
            t.extractall(SRC, filter="tar") if hasattr(tarfile, "data_filter") else t.extractall(SRC)
    return dest


def build_lib(name, libs, flags=None, jobs=4, gy=False):
    cfg = libs[name]
    root = extract(cfg)
    incs = []
    for i in cfg["include"]:
        if i.startswith("@"):
            other, _, sub = i[1:].partition("/")
            incs.append(os.path.join(extract(libs[other]), sub))
        else:
            incs.append(os.path.join(root, i))
    # VS.NET 2002 public headers; sys/*.h is missing there, take VS2003's.
    sysinc = [f"{VC70}/include", f"{VC71}/include"]
    env = wine_env(incs + sysinc, [])
    outdir = os.path.join(BUILD, "lib", name)
    os.makedirs(outdir, exist_ok=True)
    fl = list(flags if flags is not None else cfg["flags"])
    if gy:
        fl.append("/Gy")
    defs = [f"/D{d}" for d in cfg["defines"]]
    cl = f"{VC70}/bin/cl.exe" if cfg["compiler"] == "vc70" else f"{VC71}/bin/cl.exe"

    def one(src):
        obj = os.path.join(outdir, os.path.splitext(os.path.basename(src))[0] + ".obj")
        args = ["/nologo", "/c"] + fl + defs + ["/Fo" + winpath(obj), winpath(os.path.join(root, src))]
        return obj, run_wine(cl, args, env, check=False)

    objs = []
    with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
        for obj, (rc, out) in ex.map(one, cfg["sources"]):
            if rc != 0:
                raise RuntimeError(out)
            objs.append(obj)
    for objname, (a, b) in cfg.get("swap_symbols", {}).items():
        swap_undefined_symbols(os.path.join(outdir, objname), a, b)
    return objs


def archive(libname, objs):
    libpath = os.path.join(BUILD, "lib", libname + ".lib")
    if os.path.exists(libpath):
        os.remove(libpath)
    run_wine(f"{VC71}/bin/lib.exe", ["/nologo", "/OUT:" + winpath(libpath)] + [winpath(o) for o in objs],
             wine_env())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only")
    ap.add_argument("--flags")
    ap.add_argument("--gy", action="store_true")
    ap.add_argument("-j", type=int, default=4)
    a = ap.parse_args()
    libs = json.load(open(os.path.join(CONFIG, "libs.json")))["libs"]
    names = a.only.split(",") if a.only else list(libs)
    archives = {}
    for n in names:
        objs = build_lib(n, libs, a.flags.split() if a.flags else None, a.j, a.gy)
        archives.setdefault(libs[n].get("archive", n), []).extend(objs)
    for lib, objs in archives.items():
        if a.only:  # partial rebuild: keep the other members of the archive
            for n, cfg in libs.items():
                if cfg.get("archive", n) == lib and n not in names:
                    d = os.path.join(BUILD, "lib", n)
                    objs += [os.path.join(d, os.path.splitext(os.path.basename(s))[0] + ".obj")
                             for s in cfg["sources"]]
        archive(lib, objs)
        print("built", lib + ".lib")


if __name__ == "__main__":
    main()
