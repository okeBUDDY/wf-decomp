"""Shared helpers: paths, Wine/MSVC invocation, PE and COFF parsing."""
import os
import struct
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONFIG = os.path.join(ROOT, "config")
BUILD = os.path.join(ROOT, "build")
ORIG_EXE = os.environ.get("WF_ORIG", os.path.join(ROOT, "orig", "wf.exe"))
# Toolchain roots (not committed). See README.
VC71 = os.environ.get("WF_VC71", "/root/wf/tools/vc71/Vc7")
VC70 = os.environ.get("WF_VC70", "/root/wf/tools/vc70")
THIRDPARTY = os.environ.get("WF_THIRDPARTY", "/root/wf/thirdparty")

IMAGE_BASE = 0x400000


def winpath(p):
    return "Z:" + os.path.abspath(p).replace("/", "\\")


def wine_env(include=(), lib=()):
    env = dict(os.environ)
    env.setdefault("WINEDEBUG", "-all")
    env.setdefault("WINEPREFIX", "/root/.wine-wf")
    env["INCLUDE"] = ";".join(winpath(p) for p in include)
    env["LIB"] = ";".join(winpath(p) for p in lib)
    return env


def vc71_env():
    return wine_env(
        [f"{VC71}/include", f"{VC71}/PlatformSDK/Include"],
        [f"{VC71}/lib", f"{VC71}/PlatformSDK/Lib"],
    )


_WINESERVER_STARTED = False


def ensure_wineserver(env):
    """Start a persistent wineserver for the prefix (once per process), so each
    cl/link call doesn't cold-start one (3-7 s -> ~0.1 s per call)."""
    global _WINESERVER_STARTED
    if _WINESERVER_STARTED:
        return
    _WINESERVER_STARTED = True
    try:
        subprocess.Popen(["wineserver", "-p"], env=env, stdin=subprocess.DEVNULL,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                         start_new_session=True)
        # Boot the prefix with output detached: the services wine starts on
        # the first call (winedevice.exe, ...) outlive it and would otherwise
        # inherit, and hold open, the stdout pipe of the first cl/link call.
        subprocess.run(["wine", "cmd", "/c", "exit"], env=env, stdin=subprocess.DEVNULL,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       start_new_session=True, timeout=120)
    except (OSError, subprocess.TimeoutExpired):
        pass


def run_wine(exe, args, env, cwd=None, check=True):
    ensure_wineserver(env)
    r = subprocess.run(["wine", exe] + list(args), env=env, cwd=cwd,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = r.stdout.decode("latin-1").replace("\r", "")
    if check and r.returncode != 0:
        raise RuntimeError(f"{os.path.basename(exe)} failed ({r.returncode}):\n{out}")
    return r.returncode, out


# --------------------------------------------------------------------- PE ---
class PE:
    def __init__(self, path):
        self.data = open(path, "rb").read()
        d = self.data
        self.pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, self.pe + 6)[0]
        opt = struct.unpack_from("<H", d, self.pe + 20)[0]
        self.sections = []
        for i in range(nsec):
            o = self.pe + 24 + opt + 40 * i
            name = d[o:o + 8].rstrip(b"\0").decode()
            vs, va, rs, rp = struct.unpack_from("<IIII", d, o + 8)
            self.sections.append(dict(name=name, va=va, vsize=vs, rsize=rs, raw=rp))

    def section(self, name):
        return next(s for s in self.sections if s["name"] == name)

    def off(self, va):
        rva = va - IMAGE_BASE
        for s in self.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                if rva - s["va"] >= s["rsize"]:
                    return None  # bss
                return s["raw"] + rva - s["va"]
        if rva < 0x1000:
            return rva
        return None

    def read(self, va, n):
        o = self.off(va)
        if o is None:
            return b"\0" * n
        return self.data[o:o + n]

    def u32(self, va):
        return struct.unpack("<I", self.read(va, 4))[0]

    def section_of(self, va):
        rva = va - IMAGE_BASE
        for s in self.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                return s["name"]
        return None


# ------------------------------------------------------------------- COFF ---
def ar_members(path_or_bytes):
    d = path_or_bytes if isinstance(path_or_bytes, bytes) else open(path_or_bytes, "rb").read()
    if d[:8] != b"!<arch>\n":
        yield os.path.basename(path_or_bytes) if isinstance(path_or_bytes, str) else "?", d
        return
    o = 8
    longnames = b""
    while o + 60 <= len(d):
        h = d[o:o + 60]
        name = h[:16].decode("latin-1").strip()
        size = int(h[48:58])
        body = d[o + 60:o + 60 + size]
        if name == "//":
            longnames = body
        elif name not in ("/", ""):
            if name.startswith("/") and name[1:].isdigit():
                k = int(name[1:])
                e = longnames.find(b"\0", k)
                name = longnames[k:e].decode("latin-1")
            yield name.rstrip("/"), body
        o += 60 + size
        o += o & 1


class Coff:
    """Minimal i386 COFF object reader."""

    def __init__(self, b, name="?"):
        self.name = name
        self.raw = b
        (self.machine, ns, self.timestamp, symoff, nsym, optsz,
         self.chars) = struct.unpack_from("<HHIIIHH", b, 0)
        self.sections = []
        for i in range(ns):
            s = b[20 + optsz + 40 * i:20 + optsz + 40 * (i + 1)]
            vs, va, sz, ptr, prel, pln, nrel, nln, fl = struct.unpack_from("<IIIIIIHHI", s, 8)
            nm = s[:8].rstrip(b"\0").decode("latin-1")
            rels = [struct.unpack_from("<IIH", b, prel + 10 * k) for k in range(nrel)]
            data = b[ptr:ptr + sz] if ptr else b""
            self.sections.append(dict(name=nm, size=sz, data=data, rels=rels, flags=fl,
                                      syms=[], comdat=None, index=i + 1))
        strtab = symoff + 18 * nsym
        self.symbols = {}  # index -> dict
        self.symlist = []
        i = 0
        while i < nsym:
            e = b[symoff + 18 * i:symoff + 18 * i + 18]
            nm = e[:8]
            if nm[:4] == b"\0\0\0\0":
                o = struct.unpack_from("<I", nm, 4)[0]
                nm = b[strtab + o:b.index(b"\0", strtab + o)]
            else:
                nm = nm.rstrip(b"\0")
            val, sec, typ, cls, aux = struct.unpack_from("<IhHBB", e, 8)
            sym = dict(index=i, name=nm.decode("latin-1"), value=val, section=sec,
                       type=typ, cls=cls, naux=aux)
            if cls == 3 and aux and 0 < sec <= ns and val == 0:
                a = b[symoff + 18 * (i + 1):symoff + 18 * (i + 2)]
                ln, nrel, nln, chk, num, sel = struct.unpack_from("<IHHIHB", a, 0)
                if sym["name"] == self.sections[sec - 1]["name"]:
                    self.sections[sec - 1]["comdat"] = (sel, num)
                    sym["secdef"] = True
            self.symbols[i] = sym
            self.symlist.append(sym)
            if 0 < sec <= ns and not sym.get("secdef"):
                self.sections[sec - 1]["syms"].append(sym)
            i += 1 + aux
        self.comp_id = None
        for s in self.symlist:
            if s["name"] == "@comp.id":
                self.comp_id = s["value"]


def coff_members(path):
    for name, body in ar_members(path):
        if len(body) < 20:
            continue
        if struct.unpack_from("<H", body, 0)[0] != 0x14C:
            continue
        try:
            yield Coff(body, name)
        except Exception:
            continue


def import_symbols(lib_path):
    """Parse short-format import members: returns list of
    (symbol, dll, import_name_or_ordinal, name_type)."""
    out = []
    for name, body in ar_members(lib_path):
        if len(body) < 20:
            continue
        sig1, sig2, ver, mach, ts, size, hint, typ = struct.unpack_from("<HHHHIIHH", body, 0)
        if sig1 != 0 or sig2 != 0xFFFF:
            continue
        rest = body[20:20 + size]
        sym, dll = rest.split(b"\0")[:2]
        sym = sym.decode()
        ntype = (typ >> 2) & 7
        if ntype == 0:
            imp = hint  # ordinal
        else:
            imp = sym
            if ntype in (2, 3):
                imp = imp.lstrip("?@_") if ntype == 2 else imp.lstrip("?@_").split("@")[0]
        out.append((sym, dll.decode(), imp, ntype))
    return out
