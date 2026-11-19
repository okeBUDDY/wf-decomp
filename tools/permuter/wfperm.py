"""Bridge between decomp-permuter and the VC7.1 C++ build (see README.md here).

The permuter parses C with pycparser, so a member function is handed to it as
a C rendering: `Class::Method` becomes `Class__Method`, `A::B` becomes `A__B`,
C++ casts become C casts, and every type name the function uses gets a
`typedef int T;` stub so that pycparser can tell declarations from
expressions. compile.py undoes this on each candidate: it drops the stubs, puts
back the original C++ header and the `::`, splices the body into a copy of the
real source file (so the TU, its headers and its other functions stay as they
are) and compiles that with cl. The function's bytes, with every relocation
resolved to its address in wf.exe, become a flat ELF object, which is what the
permuter's x86 objdump scoring reads. target.o holds the original bytes the same
way.
"""
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from wfcommon import ROOT  # noqa: E402

# Types from the Windows/DirectX/CRT headers that functions use. Extend as needed.
BUILTIN_TYPES = """
BOOL BYTE WORD DWORD UINT LONG HRESULT HWND HANDLE HINSTANCE LPARAM WPARAM LPCSTR
LPSTR LPVOID LRESULT RECT POINT SIZE tagSIZE D3DCOLOR D3DVECTOR D3DTLVERTEX D3DMATRIX
D3DPRIMITIVETYPE DDSURFACEDESC2 DDPIXELFORMAT WAVEFORMATEX IDirectSoundBuffer
IDirect3DDevice7 IDirectDrawSurface7 CRITICAL_SECTION FILE size_t bool wchar_t
""".split()

CAST_RE = re.compile(r"\b(static_cast|reinterpret_cast|const_cast)\s*<")


def _match(text, i, open_ch, close_ch):
    """Index just past the bracket that closes the one at text[i]."""
    depth = 0
    for j in range(i, len(text)):
        if text[j] == open_ch:
            depth += 1
        elif text[j] == close_ch:
            depth -= 1
            if depth == 0:
                return j + 1
    raise ValueError("unbalanced " + open_ch)


def convert_casts(text):
    while True:
        m = CAST_RE.search(text)
        if not m:
            return text
        lt = m.end() - 1
        gt = _match(text, lt, "<", ">")
        typ = text[lt + 1:gt - 1].strip()
        p = gt
        while text[p].isspace():
            p += 1
        assert text[p] == "(", "cast without ("
        end = _match(text, p, "(", ")")
        text = text[:m.start()] + f"(({typ}){text[p:end]})" + text[end:]


METHOD_RE = re.compile(r"(\.|->)\s*([A-Za-z_]\w*)\s*\(")


def _object_start(text, end):
    """Start of the postfix expression that ends at text[end] (exclusive)."""
    i = end
    while i > 0:
        c = text[i - 1]
        if c.isalnum() or c == "_":
            i -= 1
        elif c in ")]":
            open_ch = "(" if c == ")" else "["
            depth = 0
            j = i - 1
            while True:
                if text[j] == c:
                    depth += 1
                elif text[j] == open_ch:
                    depth -= 1
                    if depth == 0:
                        break
                j -= 1
            i = j
        elif text[i - 2:i] == "->":
            i -= 2
        elif c == ".":
            i -= 1
        elif c.isspace() and i < end and not (text[i] in "([" or text[i].isalnum()):
            i -= 1
        else:
            break
    return i


def methods_to_calls(text):
    """`obj->f(a)` -> `f__m(obj, a)`, `obj.f(a)` -> `f__d(obj, a)`."""
    while True:
        ms = list(METHOD_RE.finditer(text))
        if not ms:
            return text
        m = ms[-1]
        start = _object_start(text, m.start())
        obj = text[start:m.start()].strip()
        paren = m.end() - 1
        end = _match(text, paren, "(", ")")
        args = text[paren + 1:end - 1].strip()
        kind = "m" if m.group(1) == "->" else "d"
        call = f"{m.group(2)}__{kind}({obj}{', ' + args if args else ''})"
        text = text[:start] + call + text[end:]


def _split_first_arg(args):
    depth = 0
    for i, c in enumerate(args):
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
        elif c == "," and depth == 0:
            return args[:i], args[i + 1:]
    return args, ""


def calls_to_methods(text):
    """Undoes methods_to_calls."""
    while True:
        m = re.search(r"\b([A-Za-z_]\w*?)__([md])\s*\(", text)
        if not m:
            return text
        paren = m.end() - 1
        end = _match(text, paren, "(", ")")
        obj, rest = _split_first_arg(text[paren + 1:end - 1])
        op = "->" if m.group(2) == "m" else "."
        text = text[:m.start()] + f"({obj.strip()}){op}{m.group(1)}({rest.strip()})" + text[end:]


FOR_DECL_RE = re.compile(r"\bfor\s*\(\s*((?:unsigned\s+|const\s+)?[A-Za-z_]\w*(?:\s+|\s*\*+\s*))([A-Za-z_]\w*)\s*=")


def hoist_for_decls(text):
    """`for (T x = a; ...) {...}` -> `{ T x; for (x = a; ...) {...} }`: the
    permuter's type pass does not know declarations in a for header."""
    while True:
        m = FOR_DECL_RE.search(text)
        if not m:
            return text
        paren = text.index("(", m.start())
        hdr_end = _match(text, paren, "(", ")")
        i = hdr_end
        while text[i].isspace():
            i += 1
        if text[i] == "{":
            end = _match(text, i, "{", "}")
        else:
            end = text.index(";", i) + 1
        typ, name = m.group(1).strip(), m.group(2)
        loop = "for (" + name + " =" + text[m.end():end]
        text = text[:m.start()] + "{\n" + f"{typ} {name};\n" + loop + "\n}" + text[end:]


def find_function(src_text, addr):
    """(start, brace, end) of the definition after the `// 0x<addr>` comment:
    the header is src_text[start:brace], the body src_text[brace:end]."""
    m = re.search(rf"^// 0x{addr:x}\b.*\n", src_text, re.M)
    if not m:
        raise SystemExit(f"no '// 0x{addr:x}' comment in the source")
    start = m.end()
    while src_text.startswith("//", start):
        start = src_text.index("\n", start) + 1
    brace = src_text.index("{", start)
    end = src_text.index("\n}\n", brace) + 2
    return start, brace, end


C_KEYWORDS = set("""
auto break case char const continue default do double else enum extern float for goto if
int long register return short signed sizeof static struct switch typedef union unsigned
void volatile while this NULL true false
""".split())
C_BASE = {"int", "short", "long", "char", "float", "double", "unsigned", "signed", "void"}
# Windows typedefs that are plain integers.
INT_TYPES = {"BOOL": "int", "BYTE": "unsigned char", "WORD": "unsigned short",
             "DWORD": "unsigned int", "UINT": "unsigned int", "LONG": "long",
             "HRESULT": "long", "D3DCOLOR": "unsigned int", "LPARAM": "long",
             "WPARAM": "unsigned int", "LRESULT": "long", "size_t": "unsigned int",
             "bool": "int", "wchar_t": "unsigned short"}

_TYPE = (r"((?:unsigned\s+|signed\s+|const\s+|struct\s+|class\s+)*[A-Za-z_][\w:]*"
         r"(?:\s*\*+|\s*&)?)")
_SPEC = r"^[ \t]*(?:(?:static|extern|virtual|inline|__forceinline|const|mutable|friend)\s+)*"
DECL_RE = re.compile(_SPEC + _TYPE + r"\s*(\*?)\s*([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*(?:=[^;]*)?;", re.M)
FUNC_RE = re.compile(_SPEC + _TYPE + r"\s*(\*?)\s*(?:[A-Za-z_]\w*::)?([A-Za-z_]\w*)\s*\(", re.M)
NOT_TYPES = {"return", "delete", "new", "goto", "else", "case", "typedef", "using"}


def _sources():
    for d, _, files in os.walk(os.path.join(ROOT, "src")):
        for f in sorted(files):
            yield open(os.path.join(d, f), encoding="utf-8").read()


def known_types():
    names = set(BUILTIN_TYPES) | set(INT_TYPES)
    for t in _sources():
        names.update(re.findall(r"\b(?:class|struct|union|enum)\s+(\w+)", t))
        names.update(re.findall(r"\btypedef\b[^;{]*?\b(\w+)\s*;", t))
        names.update(re.findall(r"\btypedef\s+(?:struct|union|enum)[^;]*?\}\s*(\w+)\s*;", t))
    return names


def declared_types():
    """name -> C++ type, for the variables/fields and the functions/methods
    declared anywhere in src/ (the first declaration of a name wins)."""
    var, fn = {}, {}
    for t in _sources():
        t = re.sub(r"//[^\n]*", "", t)
        for m in DECL_RE.finditer(t):
            typ, star, name, arr = m.groups()
            if typ.split()[-1] not in NOT_TYPES:
                var.setdefault(name, (typ + star, arr or ""))
        for m in FUNC_RE.finditer(t):
            typ, star, name = m.groups()
            if typ.split()[-1] not in NOT_TYPES:
                fn.setdefault(name, typ + star)
    return var, fn


def c_type(typ, types):
    """A C type for a C++ type: integer and float types stay, a known class
    becomes its stub struct (pointers keep their stars), anything else int."""
    typ = re.sub(r"\b(const|struct|class)\b", "", typ).replace("&", "*")
    stars = typ.count("*")
    base = typ.replace("*", "").strip()
    words = base.split()
    if words and all(w in C_BASE for w in words):
        c = " ".join(words)
    elif base in INT_TYPES:
        c = INT_TYPES[base]
    elif base.split("::")[-1] in types:
        c = "struct " + base.replace("::", "__")
    else:
        c = "int"
    return c + " " + "*" * stars


def stubs_for(text):
    """C declarations that give pycparser, and the permuter's type pass, a
    type for every name the function uses. Every class the function names
    becomes a struct with all the members the function uses (types from the
    first declaration of that name in src/)."""
    types = known_types()
    var, fn = declared_types()
    idents = set(re.findall(r"\b[A-Za-z_]\w*\b", text))
    members = set(re.findall(r"(?:\.|->)\s*([A-Za-z_]\w*)", text))
    called = set(re.findall(r"\b([A-Za-z_]\w*)\s*\(", text))
    fields = []
    for m in sorted(members):
        if m in called:
            fields.append(f"  {c_type(fn.get(m, 'int'), types)}(*{m})();")
        else:
            typ, arr = var.get(m, ("int", ""))
            fields.append(f"  {c_type(typ, types)}{m}{arr};")
    body = "{\n" + "\n".join(fields or ["  int unused_;"]) + "\n}"
    # Classes: named in the function or reached through a member's type.
    named = {n for n in idents if n in types or
             ("__" in n and n.split("__")[-1] in types and n.split("__")[-1][0].isupper())}
    reached = set(re.findall(r"struct (\w+)", "\n".join(fields)))
    for n in idents:
        if n in var:
            reached.update(re.findall(r"struct (\w+)", c_type(var[n][0], types)))
        if n in fn:
            reached.update(re.findall(r"struct (\w+)", c_type(fn[n], types)))
    out = []
    for n in sorted(named | reached):
        if n in INT_TYPES:
            out.append(f"typedef {INT_TYPES[n]} {n};")
        else:
            out.append(f"typedef struct {n} {n};")
    for n in sorted((named | reached) - set(INT_TYPES)):
        out.append(f"struct {n} {body};")
    own = re.match(r"[^(]*?(\w+)\s*\(", text).group(1)
    bare = set(re.findall(r"\b[A-Za-z_]\w*\b", re.sub(r"(?:\.|->)\s*\w+", "", text)))
    for n in sorted(bare - C_KEYWORDS - named - {own}):
        if n in called:
            real = re.sub(r"__[md]$", "", n)
            out.append(f"{c_type(fn.get(real, 'int'), types)}{n}();")
        elif n in var:
            typ, arr = var[n]
            out.append(f"{c_type(typ, types)}{n}{arr};")
        else:
            out.append(f"int {n};")
    # NULL, true and false as constants.
    out.append("int NULL; int true; int false;")
    return "\n".join(out) + "\n"


def to_c(header, body):
    """C rendering of a C++ definition (header up to `{`, then body)."""
    # `new T` -> `((T*)new__())`; compile.py turns it back.
    body = re.sub(r"\bnew\s+([A-Za-z_]\w*)(?!\s*[\[(])", r"((\1*)new__())", body)
    for bad in (r"\bnew\b", r"\bdelete\b", r"\boperator\b", r"\btemplate\b"):
        if re.search(bad, body):
            print(f"warning: {bad} is C++ only; edit base.c by hand", file=sys.stderr)
    h = re.sub(r"\)\s*const\s*$", ")", header.strip())
    text = h + " " + body
    text = convert_casts(text)
    text = re.sub(r"(?<![\w:])::(\w)", r"\1", text)  # ::Global
    text = re.sub(r"\b(\w+)::(?=\w)", r"\1__", text)
    text = methods_to_calls(text)
    text = hoist_for_decls(text)
    return stubs_for(text) + "\n" + text + "\n"


def from_c(c_text, func_name, header):
    """C++ definition from a (permuted) C rendering."""
    m = re.search(rf"^[^\n;]*\b{re.escape(func_name)}\s*\([^;{{]*\)\s*{{", c_text, re.M)
    if not m:
        raise SystemExit(f"{func_name} not found in candidate")
    text = c_text[m.start():]
    body = text[text.index("{"):]
    body = re.sub(r"\bstruct\s+(?=\w)", "", body)
    body = re.sub(r"\(\s*(\w+)\s*\*\s*\)\s*new__\s*\(\s*\)", r"(new \1)", body)
    body = calls_to_methods(body)
    body = re.sub(r"\b([A-Za-z]\w*?)__(?=[A-Za-z])", r"\1::", body)
    return header.rstrip() + " " + body.rstrip() + "\n"


def resolve(data, rels, sym_addr, addr, orig):
    """Function bytes with every relocation field filled in with the value the
    exe would hold. `rels` is [(offset, symbol name, 'dir32'|'rel32')]; a
    symbol with no known address takes the original's dword at that offset."""
    data = bytearray(data)
    for off, name, kind in rels:
        if off + 4 > len(data):
            continue
        addend = struct.unpack_from("<I", data, off)[0]
        target = sym_addr(name)
        if target is None:
            v = struct.unpack_from("<I", orig, off)[0] if off + 4 <= len(orig) else 0
        elif kind == "dir32":
            v = target + addend
        else:
            v = target + addend - (addr + off + 4)
        struct.pack_into("<I", data, off, v & 0xFFFFFFFF)
    return bytes(data)


def write_elf(code, path):
    """A relocatable i386 ELF object whose .text holds `code`."""
    raw = path + ".bin"
    open(raw, "wb").write(code)
    subprocess.run(["objcopy", "-I", "binary", "-O", "elf32-i386", "-B", "i386",
                    "--rename-section", ".data=.text,alloc,load,readonly,code,contents",
                    raw, path], check=True)
    os.remove(raw)
