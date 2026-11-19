# decomp-permuter bridge

Runs [decomp-permuter](https://github.com/simonlindholm/decomp-permuter) on one
C++ function of `src/`. The permuter is not vendored: clone it anywhere (tested at
860b6d9, with `pycparser` and `toml` installed) and call its `permuter.py`.

```
python3 tools/compare.py                       # builds build/src-sjis and checks the tree
python3 tools/permuter/setup.py src/core_controls.cpp 0x414700
python3 $WF_PERMUTER/permuter.py build/permuter/00414700 -j4 --stop-on-zero --better-only
```

How it works (`wfperm.py`):

* **base.c** is a C rendering of the function that pycparser can parse:
  `Class::Method` becomes `Class__Method`, other `A::B` become `A__B`, C++ casts
  become C casts, and method calls become free calls with the object first
  (`obj->f(a)` -> `f__m(obj, a)`, `obj.f(a)` -> `f__d(obj, a)`). Implicit
  members and `this` stay as plain names. Stubs above the function declare
  every name it uses, with the type of the first declaration of that name in
  `src/`; each class is a struct with every member the function uses, so the
  permuter's type pass works and its new temporaries get real class types.
* **compile.sh** (`compile.py`) undoes all that on a candidate: it drops the
  stubs, restores the original C++ header (`const`, default `this`), the
  method calls and the `::`, splices the result into the real source file in
  place of the function, and compiles that file with cl `/O2 /GX /GS` like
  compare.py (cp932, headers from `build/src-sjis`). The rest of the TU is
  unchanged, so inlining and register tracking see the same context, and the
  codegen is the real `__thiscall` member's: a matching function scores 0
  (checked on `CSndFileSEQ::Read`).
* The function's bytes, with each relocation filled in with the address the exe
  holds (symbols.txt; a symbol with no known address takes the original's
  dword), are written as a flat i386 ELF object, so the permuter's x86 objdump
  scoring compares plain instructions. **target.o** holds the original bytes.

Notes:

* `settings.toml` turns off the passes whose output cannot be mapped back
  (`perm_inline`, the external/function type passes, `perm_sameline`).
  Candidates that are not valid C++ (a temporary holding a class by value, a
  member-function pointer, ...) just fail to compile and are skipped.
* `new T`, `new T[n]` and `delete p` are rendered (as `new__()`, `new_array__(n)`,
  `delete__(p)`); references and templates are not: setup.py warns, and base.c can
  be edited by hand. Stubs made from string literals or labels (`int d種の;`,
  `int default;`) and undeclared SDK structs (`SMALL_RECT`) need fixing by hand too.
* A candidate the permuter reports is a hint, not code: port the idea back to
  natural C++ in `src/` and confirm it with `tools/compare.py`. Never commit
  permuter output.

Simpler helpers in `tools/`: `try_variants.py` (apply listed source edits one
at a time and report each result) and `perm_lines.py` (try every single-line
move inside a block).
