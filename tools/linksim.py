"""Model of how LINK 7.10 pulls library members and orders the IAT.

Library search (verified against real links, see README):
  * one global list of undefined symbols, in the order they were first seen
    (object symbol tables in command-line order, then the entry point);
  * each library, in command-line order followed by /DEFAULTLIB libraries,
    is scanned along that list, repeatedly, until it resolves nothing more;
    a pulled member appends its own undefined symbols to the list;
  * passes over all libraries repeat until nothing changes.

IAT order: LINK collects the import entries (one per pulled import member,
plus one per DLL when its import descriptor is pulled) in pull order and
sorts them by DLL name with the CRT's qsort, which is not stable; the order
of entries inside one DLL is whatever that qsort leaves. Entries that end up
unreferenced (/OPT:REF) are dropped afterwards.
"""
import os
import re
import struct

from wfcommon import BUILD, VC71, Coff, ar_members, expand_dirs

LIBDIRS = [os.path.join(BUILD, "lib"), f"{VC71}/lib", f"{VC71}/PlatformSDK/Lib"]


def member_info(body):
    """defs/undefs/drectve of an object or short-import archive member."""
    if len(body) < 20:
        return None
    sig1, sig2 = struct.unpack_from("<HH", body, 0)
    if sig1 == 0 and sig2 == 0xFFFF:
        size = struct.unpack_from("<I", body, 12)[0]
        typ = struct.unpack_from("<H", body, 18)[0]
        sym, dll = body[20:20 + size].split(b"\0")[:2]
        sym, dll = sym.decode(), dll.decode()
        defs = ["__imp_" + sym]
        if typ & 3 == 0:  # code import: also defines the thunk
            defs.append(sym)
        return dict(defs=defs, undefs=["__IMPORT_DESCRIPTOR_" + dll.rsplit(".", 1)[0]],
                    drectve="", imp=sym, dll=dll.rsplit(".", 1)[0].upper())
    if sig1 != 0x14C:
        return None
    c = Coff(body)
    defs, undefs = [], []
    for s in c.symlist:
        if s["cls"] != 2:
            continue
        if s["section"] != 0 or s["value"] != 0:
            defs.append(s["name"])
        else:
            undefs.append(s["name"])
    dr = ""
    for sec in c.sections:
        if sec["name"] == ".drectve":
            dr = sec["data"].decode("latin-1")
    desc = None
    for d in defs:
        if d.startswith("__IMPORT_DESCRIPTOR_"):
            desc = d[len("__IMPORT_DESCRIPTOR_"):].upper()
    return dict(defs=defs, undefs=undefs, drectve=dr, desc=desc)


class Lib:
    def __init__(self, path):
        self.path = path
        self.members = []
        self.index = {}
        for name, body in ar_members(path):
            info = member_info(body)
            if info is None:
                continue
            for d in info["defs"]:
                self.index.setdefault(d, len(self.members))
            self.members.append((name, info))


_LIBS = {}


def find_lib(name):
    """A library name searched in LIBDIRS, or a path ({VC71}/..., {DXSDK}/... as in link.txt)."""
    if name.startswith("{"):
        return expand_dirs(name)
    if "/" in name:
        return name
    n = name.lower() if name.lower().endswith(".lib") else name.lower() + ".lib"
    for d in LIBDIRS:
        for f in os.listdir(d):
            if f.lower() == n:
                return os.path.join(d, f)
    raise KeyError(name)


def get_lib(name):
    p = find_lib(name)
    if p not in _LIBS:
        _LIBS[p] = Lib(p)
    return _LIBS[p]


def default_libs(drectve):
    return [m.strip('"') for m in re.findall(r'/DEFAULTLIB:("[^"]+"|\S+)', drectve, re.I)]


def simulate(objects, libs, entry="_WinMainCRTStartup"):
    """objects: [(tag, info)]; returns the pulled members [(lib, name, info)]
    in load order."""
    defined, undef, seen = set(), [], set()
    deflibs = []
    pulled = []

    def load(info):
        defined.update(info["defs"])
        for u in info["undefs"]:
            if u not in defined and u not in seen:
                seen.add(u)
                undef.append(u)
        for lib in default_libs(info.get("drectve", "")):
            if lib.lower().removesuffix(".lib") not in [x.lower().removesuffix(".lib") for x in deflibs]:
                deflibs.append(lib)

    for _, info in objects:
        load(info)
    if entry not in defined and entry not in seen:
        seen.add(entry)
        undef.append(entry)
    names = [n.lower().removesuffix(".lib") for n in libs]
    order = [get_lib(n) for n in libs]
    loaded = set()
    changed = True
    while changed:
        changed = False
        for lib in deflibs:
            if lib.lower().removesuffix(".lib") not in names:
                names.append(lib.lower().removesuffix(".lib"))
                order.append(get_lib(lib))
        for lib in order:
            while True:
                got = False
                for u in list(undef):
                    if u in defined:
                        continue
                    m = lib.index.get(u)
                    if m is None or (lib.path, m) in loaded:
                        continue
                    loaded.add((lib.path, m))
                    name, info = lib.members[m]
                    load(info)
                    pulled.append((os.path.basename(lib.path), name, info))
                    got = changed = True
                if not got:
                    break
    return pulled


def object_info(path):
    return os.path.basename(path), member_info(open(path, "rb").read())


def import_entries(pulled):
    """The list LINK sorts for the IAT: (dll, import symbol or None)."""
    out = []
    for lib, name, info in pulled:
        if info.get("imp"):
            out.append((info["dll"], info["imp"]))
        elif info.get("desc"):
            out.append((info["desc"], None))
    return out


def crt_qsort(a, cmp):
    """qsort of the VC7.1 CRT (median of three, shortsort below 9 elements)."""
    def shortsort(lo, hi):
        while hi > lo:
            mx = lo
            for p in range(lo + 1, hi + 1):
                if cmp(a[p], a[mx]) > 0:
                    mx = p
            a[mx], a[hi] = a[hi], a[mx]
            hi -= 1

    if len(a) < 2:
        return a
    stack = []
    lo, hi = 0, len(a) - 1
    while True:
        size = hi - lo + 1
        if size <= 8:
            shortsort(lo, hi)
        else:
            mid = lo + size // 2
            if cmp(a[lo], a[mid]) > 0:
                a[lo], a[mid] = a[mid], a[lo]
            if cmp(a[lo], a[hi]) > 0:
                a[lo], a[hi] = a[hi], a[lo]
            if cmp(a[mid], a[hi]) > 0:
                a[mid], a[hi] = a[hi], a[mid]
            loguy, higuy = lo, hi
            while True:
                if mid > loguy:
                    loguy += 1
                    while loguy < mid and cmp(a[loguy], a[mid]) <= 0:
                        loguy += 1
                if mid <= loguy:
                    loguy += 1
                    while loguy <= hi and cmp(a[loguy], a[mid]) <= 0:
                        loguy += 1
                higuy -= 1
                while higuy > mid and cmp(a[higuy], a[mid]) > 0:
                    higuy -= 1
                if higuy < loguy:
                    break
                a[loguy], a[higuy] = a[higuy], a[loguy]
                if mid == higuy:
                    mid = loguy
            higuy += 1
            if mid < higuy:
                higuy -= 1
                while higuy > mid and cmp(a[higuy], a[mid]) == 0:
                    higuy -= 1
            if mid >= higuy:
                higuy -= 1
                while higuy > lo and cmp(a[higuy], a[mid]) == 0:
                    higuy -= 1
            if higuy - lo >= hi - loguy:
                if lo < higuy:
                    stack.append((lo, higuy))
                if loguy < hi:
                    lo = loguy
                    continue
            else:
                if loguy < hi:
                    stack.append((loguy, hi))
                if lo < higuy:
                    hi = higuy
                    continue
        if not stack:
            return a
        lo, hi = stack.pop()


def sort_entries(entries):
    def cmp(x, y):
        return (x[0] > y[0]) - (x[0] < y[0])

    return crt_qsort(list(entries), cmp)


def iat_order(entries):
    """dll -> import symbols in IAT order (before /OPT:REF drops any)."""
    out = {}
    for dll, imp in sort_entries(entries):
        if imp:
            out.setdefault(dll, []).append(imp)
    return out
