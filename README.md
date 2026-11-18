# Warning Forever 1.08 decompilation

A byte-matching decompilation of `wf.exe` from Warning Forever 1.08 (Hikoza T. Ohkubo, 2005).
The exe is a PE32 file with image base 0x400000 and no relocations. It was built with
Visual C++ .NET 2003 (cl 13.10.3077, LINK 7.10). Its third-party libraries were built with
Visual C++ .NET 2002 (cl 13.00.9466).

The current state:

* All 361 identified library functions (libogg 1.0, libvorbis 1.0, zlib 1.1.4,
  libpng 1.2.5) are rebuilt from source and match byte for byte.
* The whole exe is relinked with the original LINK from game "blob" objects,
  the rebuilt libraries, libcmt and the SDK import libraries. The result has the
  same size and layout, and `.data` and the IAT order match. The remaining differences
  are listed below.
* There is a per-function compare pipeline for decompiled C++ (`tools/compare.py`, `objdiff.json`).

## Setup

* Linux, Wine 9 (with 32-bit support), Python 3 with `capstone` and `pefile`.
* The toolchains are not committed:
  * `WF_VC71` is the VS.NET 2003 `Vc7` directory (cl 13.10.3077, link 7.10.3077,
    including `PlatformSDK/`). Default: `/root/wf/tools/vc71/Vc7`.
  * `WF_VC70` is the VS.NET 2002 `Vc7` directory (cl 13.00.9466), used only for the
    libraries. Default: `/root/wf/tools/vc70`.
  * `WINEPREFIX` defaults to `/root/.wine-wf`.
* Put the original `wf.exe` in `orig/`, or point `WF_ORIG` at it. Its SHA-1 is in
  `config/wf.sha1`. Never commit `wf.exe`, `data.bin` or toolchain binaries.
* Library source tarballs (`libogg-1.0`, `libvorbis-1.0`, `zlib-1.1.4`, `libpng-1.2.5`)
  go in `WF_THIRDPARTY` (default `/root/wf/thirdparty`). `config/libs.json` records their SHA-1s.
* **wineserver:** every tool that runs Wine first starts a persistent
  `wineserver -p` in the background (`wfcommon.ensure_wineserver`), so that every later
  `cl`/`link` call skips Wine start-up. That cuts the build time several times over. It
  keeps running after the build; stop it with `wineserver -k`.

## Build

```
python3 tools/build_libs.py        # libraries -> build/lib/*.lib (VC7.0)
python3 tools/libcheck.py          # compare library functions with the exe
python3 tools/analyze.py           # recursive-descent analysis -> build/analysis.json
python3 tools/gensyms.py           # (re)generate config/symbols.txt
python3 tools/shared_comdats.py    # config/comdats.txt
python3 tools/tusplit.py           # (re)generate config/splits.txt (73 units)
python3 tools/compare.py           # compile src/, per-function match report
python3 tools/mkblobs.py           # one object per unit -> build/blobs/ (uses compiled src/)
python3 tools/iatorder.py && python3 tools/mkblobs.py   # import order (config/imports.txt)
python3 tools/link.py              # link build/wf.exe and run verify
python3 tools/verify.py [-v]       # SHA-1, header and per-section diffs
```

## Layout

| Path | Contents |
|---|---|
| `config/splits.txt` | Game translation units (address ranges per section, link order) |
| `config/symbols.txt` | Address, name, size and type of every known symbol |
| `config/libdata.txt` | Library data/functions identified by hand |
| `config/library-symbols.csv` | The 720 identified library functions |
| `config/comdats*.txt` | COMDAT constants that the game shares with the libraries |
| `config/imports.txt` | Import reference order (generated, reproduces the IAT) |
| `config/link.txt` | LINK command line |
| `config/libs.json` | Library sources, flags and archives |
| `src/` | Decompiled C++ |
| `tools/` | The pipeline; `linksim.py` models LINK's library search and IAT sort |

## Compiler flags

* Game: `cl /O2 /GX /GS`. `/GS` is required: 19 functions use `__security_cookie`.
* ogg/vorbis: `cl /MT /W2 /GX /O2 /Ob1` (VC7.0).
* zlib and libpng: `cl /MT /W3 /O1 /Ob0` (VC7.0), built into **one** archive
  (`png.lib`), because their objects interleave in `.text`.
* vorbis `lsp.obj` needs the order of its undefined symbols `_frexp`/`__CIacos` swapped
  (`swap_symbols` in libs.json). No flag reproduces the original order, which points to a
  slightly different compiler build.

## How the relink is made to match

* **Library pull order.** LINK pulls library members in the order of one global
  undefined-symbol list (see `tools/linksim.py`). The blob writes its undefined externals sorted
  by the original target address. `extern` lines in `splits.txt` reproduce references from code
  that the original `/OPT:REF` discarded: `_png_create_write_struct`, `_png_set_IHDR`,
  `__fltused`, `__except_list`, `___security_cookie` and `_atof`.
* **Shared COMDATs.** The first definition of a select-any COMDAT wins, so a float constant
  or string that the game shares with a library must be defined by the blob as a COMDAT.
  Several names at one address each get their own copy, and `/OPT:ICF` folds them back
  together. With a single copy, ICF would fold it into a later library copy.
* **IAT order.** LINK collects one entry per pulled import member, plus one per import
  descriptor, and sorts them by DLL name with the CRT's unstable `qsort`. The tiny test links
  behind this model all reproduce exactly. `iatorder.py` inverts that permutation and
  orders the blob's import references to match. The IAT now matches.
* **Group starts.** The start of a section group is aligned to the largest contribution
  alignment. The game ranges start at 0x451270 (`.rdata`), 0x45d0a0 (`.data`) and
  0x467320 (`.bss`).

## Remaining differences of build/wf.exe

| Where | Bytes | Cause / ideas |
|---|---|---|
| `.text` 0x442b70-0x446fff (vorbis `psy.obj`, `sharedbook.obj`, `codebook.obj`) | ~4.2k | Placement of static functions. `unitnorm` (from scales.h) is emitted first in our `psy.obj` but sits before `_vp_psy_init` in the original. The two identical `bitreverse` functions (sharedbook/codebook) are ICF-folded into codebook's copy, but the original keeps sharedbook's. Both point to a different VC7.0 build or service pack for the libraries (compare the `lsp.obj` symbol order). Every function on its own matches. |
| `.rdata` 0x451000-0x451261 values, 0x45b9bc-0x45c6ff | ~3k | The IAT order matches, but the hint/name table (`.idata$6`) is laid out in a different order and is 14 bytes longer, so every IAT/ILT value differs. Probably the same unstable sort over a different list. The Rich header suggests the original pulled 12 more imports and 3 more import descriptors (Implib 9 vs 6), possibly from a different DirectX SDK. |
| `.rsrc` | 483 | `extract_res.py` rebuilds the `.res`. The resource order or memory flags differ. |
| headers | 130 | Rich header (object counts: see above), TimeDateStamp, checksum. |

## Translation units

`config/splits.txt` has 73 units, the number of game C++ objects implied by the Rich header (84
Utc1310_CPP objects minus 11 from libcmt; the relinked exe's Rich header now shows 84 too). It is
generated by `tools/tusplit.py` from this evidence:

* **EH funclets** (`config/tu-evidence.txt`, `tools/tuevidence.py`). Each object's `.text$x`
  section is 16-aligned and padded with `int3`. That gives 35 chunks in link order, each tied
  to its parent functions through the `__ehhandler$` stubs they push. No boundary can fall
  inside a chunk's functions, and at least one must fall between neighbouring chunks: 34
  boundaries marked `# eh`.
* **Reference graph.** All other boundaries (`# guess`) go into gaps that no nearby
  call/reference or vtable neighbour crosses (window of 20 functions, ignoring functions with more
  than 8 callers). The largest units are split further until there are 73.
* **Other sections** follow the functions: `.text$x`/`.xdata$x` by EH function, `.CRT$XCU`/
  `.text$yc`/`.text$yd` by the global object each initializer constructs, `.rdata` by the first
  user (the COMDAT rule), and `.data`/`.bss` by the main user. Boundaries are placed monotonically
  to agree with the most items.

The `.text` boundaries marked `guess` and the data boundaries are approximate. The relink does
not depend on them, because the units stay contiguous and in order. They matter once a whole
unit is compiled from source. Correct them as decompilation shows where the source files end.

## Linking decompiled code

`mkblobs.py` takes every function that `compare.py` compiled into `build/obj/src/` and found to
match (`build/matched.json`), and puts the compiled section, with its relocations, into its unit's object in
place of the original bytes. The code that `build/wf.exe` contains then really is the compiler's
output, linked by LINK:

* Relocations to symbols named in `symbols.txt` or defined by the libraries keep their names.
* Static or unnamed targets (EH handler stubs, string literals, float constants) are mapped
  through the address the original instruction uses.
* EH funclets (`.text$x`) and EH tables (`.xdata$x`) of compiled functions still come from
  the blob.
* Functions that do not match yet keep the original bytes.
* With the 5 seed functions linked from source, the relink is unchanged.

A unit with `src=` whose object has been built is linked from that object directly (`link.py`).
That only gives a matching layout once the unit's data (vtables, strings, EH tables) also
matches.

## Decompiled code

| File | Contents | Status |
|---|---|---|
| `src/object.h` | `Object` (root class, vtable 0x451270) | `~Object` 0x401090 and scalar deleting dtor 0x401070 match |
| `src/task.h`, `src/task.cpp` | `TaskBase : Object` (task tree, vtable 0x451544) | `~TaskBase` 0x402880 and scalar deleting dtor 0x402860 match |
| `src/sprite.h`, `src/sprite.cpp` | `Sprite` with `TLVertex[4]` | `SetColor` 0x401000 matches |

Code style: explicit braces everywhere and the repo `.clang-format` (Google, left pointer
alignment). Use structs and named fields instead of raw offsets, and recover classes with
their vtables and inheritance. Fields not recovered yet are padding arrays named after their
offset. Name decompiled functions by their MSVC-mangled names in `config/symbols.txt`;
`gensyms.py` keeps manual names.
