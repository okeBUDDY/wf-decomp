<img src="docs/wf-title-transparent.png" width="476" alt="Warning Forever">

# Warning Forever 1.08 decompilation

A byte-matching decompilation of Warning Forever 1.08 (Hikoza T. Ohkubo, 2005).

This decompilation was completed using generative AI (Claude Opus 5.5).

> [!WARNING]
> **Disclaimer:** there is no official release of Warning Forever or Warning Forever+ on any
> storefront. The only official sources are [the original
> website](https://www.hikware.com/Prod/index.html#wf) and the
> [wf-decomp](https://github.com/okeBUDDY/wf-decomp) and
> [wf-plus](https://github.com/okeBUDDY/wf-plus) repositories on GitHub.

Version 1.08 is the last release: the original developer ended development and support of the
game after it. The game is still [freely available to download from the official
website](https://www.hikware.com/Prod/index.html#wf). This repository does not contain the game;
building it needs your own copy of version 1.08's `wf.exe`.

All 678 game functions are decompiled to C++ in `src/` and compile to the original's bytes,
and the whole exe is relinked from them with the original linker. The relinked `wf.exe` is
byte-identical to the original except for the link timestamp, which the optional post-link
stamp restores (see [Remaining differences](#remaining-differences-of-buildwfexe)):

* All 678 game functions match byte for byte, and so do the 151 compiler-generated EH stubs;
  the relinked exe takes every game function from the compiled source (`tools/compare.py`,
  `objdiff.json`).
* All 361 identified library functions (libogg 1.0, libvorbis 1.0, zlib 1.1.4,
  libpng 1.2.5) are rebuilt from source and match byte for byte.
* The exe is relinked with the original LINK from game "blob" objects, the rebuilt
  libraries, libcmt and the SDK import libraries. Every section (`.text`, `.rdata` with the
  import table, `.data`, `.rsrc`) is byte-identical, and so is the Rich header.

## The original exe

`wf.exe` is not part of this repository. Take it from the 1.08 release (`wf108.zip`, the
game's download, see above); its SHA-1 is in `config/wf.sha1`. It is a PE32 file with image
base 0x400000 and no relocations, built with Visual C++ .NET 2003 (cl 13.10.3077, LINK 7.10);
its third-party libraries were built with Visual C++ .NET 2002 (cl 13.00.9466). The build
reads it for the bytes that are not compiled from source (the units' data and the icon) and
to verify the result. Never commit `wf.exe`, `data.bin` or toolchain binaries.

## Setup

* Linux, Wine 9 (with 32-bit support), Python 3 with `capstone` and `pefile`.
* The toolchains are not committed:
  * `WF_VC71` is the VS.NET 2003 `Vc7` directory (cl 13.10.3077, link 7.10.3077,
    including `PlatformSDK/`). Default: `/root/wf/tools/vc71/Vc7`.
  * `WF_VC70` is the VS.NET 2002 `Vc7` directory (cl 13.00.9466), used only for the
    libraries. Default: `/root/wf/tools/vc70`.
  * `WF_DXSDK` is a DirectX SDK April 2005 directory (`dxsdkver.h`: 9.06.168), used only
    for `Lib/x86/dxguid.lib` (3 objects, cl 13.10.4035, Rich `Utc1310_CPP 4035`;
    `config/link.txt` names it as `{DXSDK}/Lib/x86/dxguid.lib`). The VS.NET 2003 PlatformSDK's
    `dxguid.lib` (cl 13.00.9178) gives the same bytes but a different Rich header. The SDK
    is the DirectX 9.0c SDK April 2005 release; only that one file is needed (SHA-1
    `d9c7e86a243fb043d87c23f496b15742ec7c328e`). Default:
    `/root/wf/tools/dxsdk/apr2005`.
  * `WINEPREFIX` defaults to `/root/.wine-wf`.
* Put the original `wf.exe` in `orig/`, or point `WF_ORIG` at it.
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
python3 tools/iatorder.py --check  # predicted IAT vs the original
python3 tools/build_res.py         # res/wf.rc -> build/wf.res with rc.exe (link.py runs it too)
python3 tools/link.py              # link build/wf.exe and run verify
python3 tools/verify.py [-v]       # SHA-1, header and per-section diffs
python3 tools/postlink.py          # OPTIONAL: stamped copy build/wf-stamped.exe (see below)
```

### Source encoding

The sources in `src/` are UTF-8, and Japanese strings are written as text (not `\x`
escapes; see `.editorconfig`). The exe holds them in Shift-JIS, so `compare.py` first writes
a cp932 copy of `src/` to `build/src-sjis/` (same relative paths, so includes resolve there)
and compiles that. A character with no cp932 encoding stops the build with its file and
line. `cl` runs with `LANG`/`LC_ALL=ja_JP.UTF-8`, which gives Wine the ANSI code page 932:
only then does `cl` lex the double-byte characters, so a trail byte of 0x5c (as in 「表示」
or 「ソフト」) is not read as a backslash. Under the default code page the same literal is
mangled with warning C4129.

## Layout

| Path | Contents |
|---|---|
| `config/splits.txt` | Game translation units (address ranges per section, link order) |
| `config/symbols.txt` | Address, name, size and type of every known symbol |
| `config/libdata.txt` | Library data/functions identified by hand |
| `config/library-symbols.csv` | The 720 identified library functions |
| `config/comdats*.txt` | COMDAT constants that the game shares with the libraries |
| `config/imports.txt` | Import reference order (generated, reproduces the IAT) |
| `config/imports-discarded.txt` | Imports of discarded game code (placement found by search, names guessed) |
| `config/link.txt` | LINK command line |
| `config/postlink.txt` | Header values for the optional post-link stamp |
| `res/` | Resource script (`wf.rc`, `resource.h`); the icon is extracted at build time |
| `config/libs.json` | Library sources, flags and archives |
| `src/` | Decompiled C++ |
| `tools/` | The pipeline; `linksim.py` models LINK's library search and IAT sort |

## Compiler flags

* Game: `cl /O2 /GX /GS`. `/GS` is required: 19 functions use `__security_cookie`.
* ogg/vorbis: `cl /MT /W2 /GX /O2 /Ob1 /YX` (VC7.0).
* zlib and libpng: `cl /MT /W3 /O1 /Ob0 /YX` (VC7.0), built into **one** archive
  (`png.lib`), because their objects interleave in `.text`.
* `/YX` (automatic precompiled headers, the default of VC6-era project files) does not change
  any function's code, but it changes the object layout: `cl` emits the static functions of
  headers where the precompiled header ends (vorbis `unitnorm` from `scales.h` lands right
  before `_vp_psy_init`, `vorbis_ftoi` & co. move behind `_get_prev_page` in `vorbisfile.obj`)
  and it reorders some undefined symbols (`__CIacos` before `_frexp` in `lsp.obj`, which
  decides the libcmt pull order). `build_libs.py` compiles the files of a library one after
  the other in source order with one `.pch` (`/Fp`), as the IDE did. With `/YX`, LINK's
  `/OPT:ICF` also keeps sharedbook's copy of the two identical `bitreverse` functions, as the
  original does (which copy survives depends on the whole set of COMDATs: a small test link
  flips with the number of unrelated functions).

## How the relink is made to match

* **Library pull order.** LINK pulls library members in the order of one global
  undefined-symbol list (see `tools/linksim.py`). The blob writes its undefined externals sorted
  by the original target address. `extern` lines in `splits.txt` reproduce references from code
  that the original `/OPT:REF` discarded: `_png_create_write_struct`, `_png_set_IHDR`,
  `__fltused`, `__except_list`, `___security_cookie`, `_atof` and `__vsnprintf` (the last one
  is a stand-in, see "Remaining differences").
* **Shared COMDATs.** The first definition of a select-any COMDAT wins, so a float constant
  or string that the game shares with a library must be defined by the blob as a COMDAT.
  Several names at one address each get their own copy, and `/OPT:ICF` folds them back
  together. With a single copy, ICF would fold it into a later library copy.
* **IAT order.** LINK collects one entry per pulled import member, plus one per import
  descriptor, and sorts them by DLL name with the CRT's unstable `qsort`; the hint/name
  table (`.idata$6`) stays in pull order. The tiny test links behind this model all
  reproduce exactly. Three things reproduce the original:
  * `comctl32.lib` comes first on the command line. Its only import, `InitCommonControls`,
    is discarded by `/OPT:REF` (LINK warns LNK4089), but the `COMCTL32.dll` name string
    stays at the start of `.idata$6`, and its descriptor objects count in the Rich header.
  * kernel32 is searched twice: `Vc7/lib/kernel32.lib` before user32 for the game's
    imports, and `{VC71}/PlatformSDK/Lib/Kernel32.Lib` after libcmt for the CRT's (the
    `_RtlUnwind@16` thunk sits between libcmt and ogg in `.text`). LINK only drops a
    repeated library when it is the same file. `link.py` and `linksim.py` expand `{VC71}`.
  * `config/imports.txt` (written by `iatorder.py`) lists the game's imports in the
    original `.idata$6` order, plus 12 imports of game code that `/OPT:REF` discarded
    (`config/imports-discarded.txt`). They are not in the exe, but LINK pulled their members
    before the sort, so they change the qsort's input (Rich `Import` 173). An exhaustive
    search over their placement (labels only: the qsort compares DLL names) leaves exactly
    two solutions: 10 USER32 imports (a run of 7 anywhere in the game's USER32 group and
    3 at its end) plus one more KERNEL32 or COMCTL32 import. The names are free; the ones
    used are guesses (an about dialog for the unused dialog 102, a system menu item, a
    debug trace) and are documented as such. `iatorder.py --check` simulates the link
    of the current blobs and compares the predicted IAT per DLL with the original.
* **Group starts.** The start of a section group is aligned to the largest contribution
  alignment. The game ranges start at 0x451270 (`.rdata`), 0x45d0a0 (`.data`) and
  0x467320 (`.bss`).

## Resources

`res/wf.rc` is the resource script, reconstructed from the original `.rsrc`: the icon
(103), the about box (102, a `DIALOG`) and the DirectX settings dialog (101, a `DIALOGEX`), in
that order, which is the order of the resource data in the exe. `tools/build_res.py` compiles
it with the toolchain's `rc.exe` (via a Shift-JIS copy in `build/res/`), and LINK's `cvtres`
turns the `.res` into a `.rsrc` that equals the original. The icon is the only binary
resource. It is not committed: `build_res.py` extracts it from the original exe
(RT_GROUP_ICON 103 and RT_ICON 1) into `build/res/wf.ico` on every build. `build_res.py --check`
compares each resource that `rc` writes with the original's data. The symbolic names in
`resource.h` are made up; only the numbers are in the exe.

## Remaining differences of build/wf.exe

Baseline (differing bytes): headers 4 (the `TimeDateStamp`), `.text` 0, `.rdata` 0, `.data` 0,
`.rsrc` 0 (the count can drop when a byte of the link time happens to equal the original's).
With the optional `tools/postlink.py` stamp (`build/wf-stamped.exe`) the exe is
byte-identical.

The Rich header equals the original's (every entry, count and the entry order). The
`CheckSum` is 0 in both files. What it took:

* **dxguid.lib from the DirectX SDK April 2005** (`Utc1310_CPP 4035`, see Setup); one of its
  three objects is pulled.
* **The 151st libcmt object** (`Utc1310_C 3077`). It left no bytes in the exe, so it was
  pulled by game code that `/OPT:REF` discarded and has only COMDAT code. 116 libcmt members
  qualify, so the actual one cannot be identified; `extern __vsnprintf` in `splits.txt` (a
  debug trace next to `OutputDebugStringA` would fit) is a guess. The count matters beyond
  its own entry: LINK (`link.exe` 7.10 at 0x4591b0) sizes the Rich buffer as
  `entries * 8 + 0x20 + 8 * ((key >> 5) % 3)` bytes, so the zero padding after `Rich` (and
  with it `e_lfanew`) depends on the XOR key, a checksum over the DOS header and all entries.
* **Library order.** LINK lists Rich entries in reverse order of first use. `Implib710 2067`
  (the USER32 import descriptor) sits between the dxguid and uuid entries, so `user32.lib`
  is searched before `uuid.lib`: `config/link.txt` has `kernel32 user32 gdi32` before
  `uuid png vorbisfile` (VS.NET 2003's default library list also has uuid after gdi32). The
  IAT and every section stay the same.

**Post-link stamp (optional, separate).** `python3 tools/postlink.py` copies `build/wf.exe` to
`build/wf-stamped.exe`, writes the `TimeDateStamp` and `CheckSum` from `config/postlink.txt`
(the original's values) into the copy, and runs `verify.py` on it. It patches only values that
LINK takes from the clock; `build/wf.exe` stays the plain link output, and the baselines above
are given for both.

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
not depend on them, because the units stay contiguous and in order. Known case: `obj69_41eef0` holds four source files (`game_hud.cpp` and the three screens,
whose functions the core calls out of line). Splitting it would give 76 units, more than the
Rich header allows, so some other `guess` boundary must be merged first.

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
* File-`static` functions (storage class 3, such as `ReadPng` and the ogg callbacks) are
  linked from source too; `mkblobs.py` reports how many functions it took, which should equal
  compare.py's match count.

**Verifying a source change.** Remove `build/obj/src/` first (or check `compare.py`'s
output for `cl` errors): a file that fails to compile leaves its old object behind, and
`mkblobs.py` and the link then still report a clean build. Run `compare.py`, `mkblobs.py` and
`link.py`. The data of the units (strings, vtables, EH tables) still comes from the blobs, so
the relink does not check a change to a string or an initializer; compare the sections of
the compiled objects before and after for that.

A unit with `src=` whose object has been built is linked from that object directly (`link.py`).
That only gives a matching layout once the unit's data (vtables, strings, EH tables) also
matches.

## Decompiled code

All 678 game functions match (100% of the 128586 game code bytes; `tools/compare.py`),
and all 151 compiler-generated EH stubs (the `.text$x` unwind funclets and
`__ehhandler$` stubs, and the `.text$yc`/`.text$yd` dynamic initializers and atexit thunks at
0x44fed0-0x45092a) equal what cl emits for the TU that compiles their parent. The stubs are
counted apart from the game functions; `build/progress.json` has both. A `.text$x` funclet is
placed by its parent's push of `__ehhandler$<parent>`, so it is checked at its exact
address; a `.text$yc`/`$yd` thunk counts when it equals a stub of its size whose named
relocation targets agree. Matched stubs still link from the blob.

Every game function links into `build/wf.exe` from source. Per file
(matched/compiled; functions emitted in several files, such as header inlines, count in
each):

| Area | Files | Matched |
|---|---|---|
| Core objects | `object.cpp` 13/13, `task.cpp` 9/9, `task_list_delete.cpp` 1/1, `thread.cpp` 2/2, `file.cpp` 6/6, `pack.cpp` 8/8, `path_util.cpp` 2/2, `profile.cpp` 2/2 | 43/43 |
| Application, globals, stages | `win_main.cpp` 7/7, `application.cpp` 16/16, `cglobal.cpp` 34/34, `game.cpp` 10/10, `fn_game.cpp` 35/35 (with FnOption), `fn_title.cpp` 11/11, `game_hud.cpp` 9/9, `game_hud_warning.cpp` 13/13, `game_hud_name_entry.cpp` 9/9, `game_hud_result.cpp` 5/5, `graphics.cpp` 13/13 | 162/162 |
| HL library (device, input, 3D, camera, images, surfaces) | `hl_device.cpp` 21/21, `hl_device_init.cpp` 5/5, `hl_window.cpp` 13/13, `hl_sync.cpp` 17/17, `hl_input.cpp` 19/19, `hl_3d.cpp` 16/16, `hl_camera.cpp` 6/6, `hl_camera_init.cpp` 1/1, `hl_math.cpp` 5/5, `hl_image.cpp` 23/23, `hl_surface.cpp` 14/14, `sprite.cpp` 8/8, `line_sprites.cpp` 10/10 | 158/158 |
| HL sprite systems | `hls_anm.cpp` 16/16, `hls_pat.cpp` 37/37, `hls_txt.cpp` 18/18, `hls_txt_print.cpp` 2/2, `hls_hit_image.cpp` 8/8 | 81/81 |
| Sound | `snd_sys.cpp` 29/29, `snd_buf.cpp` 27/27, `snd_channel.cpp` 7/7, `snd_strm.cpp` 12/12, `snd_file_wav.cpp` 14/14, `snd_file_ogg.cpp` 21/21, `snd_file_seq.cpp` 36/36 | 146/146 |
| Actors and controls | `actor_system.cpp` 34/34, `actor_control.cpp` 68/68, `core_controls.cpp` 37/37, `boss_controls.cpp` 17/17, `boss_helpers.cpp` 4/4, `effect_controls.cpp` 5/5, `explosion_controls.cpp` 11/11, `player.cpp` 10/10, `background.cpp` 10/10 | 196/196 |
| Enemy generator (boss design) | `enm_param.cpp` 27/27 (plus a discarded duplicate of `~CEnmParam`, see below), `enm_plan.cpp` 18/18, `enm_tactics.cpp` 17/17, `enm_tactic_helpers.cpp` 4/4, `enm_battle.cpp` 2/2, `enm_title.cpp` 3/3 | 71/71 |
| Scores and replays | `score_table.cpp` 20/20, `replay.cpp` 19/19, `record.cpp` 5/5 | 44/44 |

`~CEnmParam` is an inline destructor, so every object that uses it gets its own COMDAT copy and LINK keeps the first one, `cglobal.cpp`'s, which matches. `enm_param.cpp`'s copy inlines `Finalize()` (defined in that file) and is discarded by the linker, as it would have been in the original build, so the exe has nothing to compare it against. `compare.py` still lists it as differing; it is not one of the 678 functions.

### Names

The names in `src/` are our own: the exe has no symbols. Classes, functions and members are
named from what the code does. Conventions: classes and functions `CamelCase`,
members `trailing_` (plain structs without the underscore), constants `kName`, globals
`g_name`, and each declaration notes its address. A member named `unused_XX_` (XX the
offset) is one that no code in the exe reads; the code that used it was discarded by
`/OPT:REF`. Rename a function, class or global together with its decorated name in
`config/symbols.txt`.

Compiler behaviour that the source depends on:

* **Short temps rotate through eax, ecx, edx; inline parameters do not count.**
  VC7.1 gives a short-lived temp (a load that is stored, pushed or used as an address) the
  first free register at or after the one following the previous such temp's register. One
  extra temp anywhere earlier in the function moves a later one from eax to ecx, two to edx,
  while the registers of longer-lived values stay. A parameter copy of an inline wrapper does
  not take part: `FnGame::Initialize` calls `Add(task, priority, TASK_APPEND)` directly
  instead of the `TaskList::AddBack()` wrapper, because the direct call's argument load is
  such a temp and puts the later `g_application.input` load in ecx.
* **`|` copies the later-defined operand; a parameter counts as defined first.**
  For `a | b` VC copies the value defined later into the result register and ors the other
  in, whatever the spelling. `CEnmParam::Grow` builds its child codes in the `code`
  parameter itself (`code = (code & kSideMask) | (code << 3);`, then `code | count`): the
  parameter counts as defined before every local, so the count is copied, as in the
  original.
* **Single-use locals are forward-substituted.** VC moves a local that is used once
  to its use, which reorders x87 work. In `CEnmParam::Plan`, `float travel` is used twice
  (`__min(static_cast<int>(travel), 50)` duplicates its argument), so it is computed in
  statement order, which gives the original's spill of `rate`. The stack slot of a spilled
  local follows liveness in IL order: `count` (the last use of `score`) comes after `rate`,
  so `rate` does not take the dead `score` parameter's slot.
* **A local reference changes register choices.** `const short& from = actor->angle;`
  instead of a copy (`BossBodyControl::RunTurnProgram`) and `const unsigned int& new_count =
  ++count;` (`SoundSystem::Pause`) give operand and CSE choices that no value form gives. A
  reference that is only an alias of a local that is never modified changes nothing.
* VC7.1 tracks which callee-saved registers a same-TU function clobbers and lets callers
  keep values in the others. A helper that the original calls (instead of inlining) and whose
  callers assume it saves everything belongs in a separate TU.
* A store through a pointer right before an increment keeps VC from folding the two (see
  `EnmProgram::Wait` / `SetArgAndNext` in `enm_param.h`).
* `strcpy` from a string literal becomes `rep movsd`; the original's byte loop comes from
  copying through a `const char*` variable that holds the literal (`GameHud::Initialize`).
* A file-`static` pointer whose address is never taken is not reloaded after stores through
  other pointers; a global one is (`kNameChars` in `game_hud_name_entry.cpp`).
* Transforms are built with the inline helpers `ScaleTransform`/`TranslateTransform`
  (`hl_3d.h`): their arguments are loaded before the body, which fixes the FPU order.
* `Lerp(int*)` is inlined at some call sites only (`MoveWarning`): it is the
  "if (time >= frames) ... else (to - from) * time / frames" shape.
* Header inlines can blow the inline budget of a big function; a small helper that the
  original inlines in every case may need `__forceinline` (`CSndFileSEQ::Play`).
* A ternary (`x = c > 0 ? c : -1`) and an `if`/`else` with two stores compile differently;
  try both. So do "assign, then reassign under a test" and an `if`/`else` that assigns in
  both arms: the forms leave the same instructions but shift the scratch registers or swap
  esi/edi (`HLSurface::InitTextureFormats`, `HLSTxtSys::Measure`). The branch order of `if (a <= 0) ... else ...` decides which side falls through.
* A value the original keeps in a register across a test (for example a constant used by a
  later ternary's other arm) shows the source: `mov eax, 2` before a test was the `: 2`
  arm of `HLSurface::InitTextureFormats`' kind choice (written as an `if`/`else`).
* `tools/compare.py -v` lists the offsets of the first differing bytes.
* Several classes sit in one original object: obj66 holds both FnGame and FnOption (and
  the out-of-line copy of the header inline `HL3D::DrawTexels`), obj04 holds WinMain with
  the definitions of `g_application` and `g_pat_sys`.
* Store scheduling follows the source order more than it seems: constant stores keep their
  order, but a register constant (0, 1.0f, `true`) is loaded early and its stores move up
  when its first store comes early in the source. `SetScale()` right after the rotation in
  `Node::Node`, `unused_18_ = 0` after the scale in `HLSurface()` and `team` before `angle`
  in `FlareControl::OnCreate` give the original's register and vtable-store order.
* A `memset` of a struct sits right before that struct's field stores; hoisting all the
  memsets to the top changes the constant CSE (`LaserCoreControl::Initialize`). A getter result
  stored after the null-check merge (one store, not one per branch) went through a local
  (`HLSObject* sprite = ...Get(8)` in `PlayerExplosionControl::Initialize`).
* Copying two floats through the FPU (`fld` early, `fstp` late) instead of integer moves is
  an inline setter with float parameters (`Actor::SetPosition` in `PlayerControl::Load`).
* `i / 48` compiled as a signed divide (`imul`/`sar`) rather than `mul`/`shr`: the loop
  counted `i` from 0 to 48 and divided `i * 0x400 / 48` (`PlayerExplosionControl::OnCreate`).
* An implicit or inline member is emitted by every TU that needs it, and LINK keeps the
  first object's copy. `~CEnmParam` (0x401b30) comes from `cglobal.cpp`, which defines
  `g_global`, `g_game`, `g_score_table` and `g_enm_param` and does not see `Finalize()`'s
  body; `enm_param.cpp`'s copy inlines it. `compare.py` writes `build/matched_obj.json`
  (the object of each function's matching copy) and `mkblobs.py` links that copy.
* A store that comes before the vtable store in a constructor was done by an inline base
  constructor: `CSndBuf`'s list link (`id_ = -1`, `next_`, `prev_`) is the base
  `CSndBufLink`, whose own vtable store VC drops as dead.
* `mov eax, ecx` instead of `mov ax, cx` for a returned 16-bit angle: the value was
  already an `unsigned short` local, returned through a ternary (`Actor::AngleTo`, which
  returns `short`).
* `Actor::angle`, `start_angle` and `matrix_angle` are `short`, and `AngleTo()` returns
  `short`: the original reads them with `movsx` and compares two angles as words
  (`cmp word ptr [eax+0x18], di`), as in `PlayerControl::Fire`; `MovePart`'s `old_angle`
  local is a `short` too.
* Two null-checked returns with a shared tail (`test`/`jmp` into the other's `jne`) are two
  `p ? p : ""` returns, one of them an inlined base-class call (`CSndStrm::GetPath`).
* An `if` whose only effect is a missing `break` (a fall-through into a case with the same
  test) shows up as a jump into the next case's test (`HLDevice::DialogProc`).
* A global used by one file only may have been file-`static`: VC then knows that stores through
  pointers leave it alone and moves its load and store across them (`g_explosion_depth`: its
  increment sinks below the loads of Create()'s arguments).
* The form of an inline helper decides register choice in its callers: `Actor::SetSprite` picks
  the null sprite with a ternary (the `if` form swapped registers in four callers).
* Two calls in an `if`/`else` that differ only in `this` are tail-merged: the arguments are
  pushed before `this` is chosen (`MoveWarning`'s title font); a ternary object picks first.
* Loop shape changes register choice: `CSndFileSEQ::Read` is a `for (;;)` with the read at the
  top, not a read before a `while`. A loop inside `if (n > 0) { ...; return x; }` keeps the
  empty-case return out of line (`CEnmParam::CountDestroyed`).
* `tools/permuter/` runs decomp-permuter on one function (see its README); inline the
  helpers the function calls by hand in `base.c` first. `ScoreTable::Insert` writes
  `FindRank()`'s search out in place.
* `if (!a() || !b()) return 0;` and `if (a() && b()) { ... }` compile to the same code, but
  separate `if (!a()) return 0; if (!b()) return 0;` statements place the shared `return 0`
  block differently (`CEnmParam::Save`/`Load`, `CSndFileOGG::Open`,
  `cGlobal::Initialize`, `Actor::ClampAngle`, and `InputTask::IsProtected`, which reads
  `devs_[i]` into a local before the null test).
* A search loop that does its work and returns from inside the loop (`return 1` in the loop,
  `return 0` after it) differs from `break` plus a test after the loop (`PackFile::Open`).
* The reverse also happens: `pack.cpp`'s globals are plain globals, which `OpenPack` and
  `PackFile::Open` need.
* Updating a member in place (`turret->angle += offset`) and updating a local copy then storing
  it choose different registers (`PartControl::MovePart`). `LaserSprite::Move` uses short
  locals for repeated subexpressions, `CSndFileSEQ::Open` a `names_size` local.
* A struct the original copies with word moves was copied field by field
  (`Replay::Start`'s header rules); a plain struct assignment copies dwords.
* `memcmp` argument order shows in `repe cmpsd`: `esi` is the first argument
  (`ScoreTable::Load`/`Store` compare `&rules_` against the file's copy).
* Identical per-case tails are cross-jumped. `Replay::LoadHeader` sets per-case `stage`,
  `score` and `timer` locals and stores them once after the switch.
* `HLImage::Load` wraps its switch in `if (format != kAuto) { ... }`.
* Same-TU auto-inlining tells files apart: `FnOption::Initialize` inlines `Sprite::FadeIn`
  only when compiled with the FnGame functions, so FnOption lives in `fn_game.cpp` (obj66). The four `game_hud*` files must stay separate. The
  `Channel::SetVolume`/`Release`/`Play` functions sit in obj24 and live in `snd_sys.cpp`.
* The declaration order of locals decides their registers and stack slots (`Actor::Destroy`:
  `layer`, `y`, `x`; `CEnmParam::GetParams`: `side` before `total`). A matrix that
  `DrawWarning` builds starts from `SetIdentity` and then `TranslateTransform`.
* Check that function sizes in `symbols.txt` stay inside their unit's `.text` range. The
  generated cut at 0x40e910 falls inside `CSndFileOGG::Open`, so `splits.txt` is
  hand-corrected there.
* Inline helper bodies: `EnmSideStats::Score()` returns early for mode 0 (a ternary swaps
  registers in `GetParams`); `OnActorDestroyed` clamps the slot's power written out in
  place (through `AddClamped()` the flags load moves after the add).
* `Application::Initialize` tests the device and the window in one `||` condition with the
  `set_title` call between them (`a() || (set(), b())`): that places the shared `return 0`
  after the input failure and lets the later failures return AddSystem's 0 in eax.
* Member types show in conversions: `FnOption::enabled_` is an `unsigned char` array, not
  `bool`, and `SetPreset`'s flag an `int` (the original zeroes ecx before the `sete`; a `bool`
  store needs no zeroing).
* A member pointer kept as `add edi, 0xc` (the address of `device_`) and reloaded after the
  call is an inline member using `this->device_` twice: `HL3D::SetPointFilter()` in
  `DrawResult`/`DrawNameEntry`, where an explicit `d3d->device_->...` pair keeps `d3d` instead.
* **A block placed after the function's return tail is made by jump threading.**
  `FnGame::RateGame` keeps its shared `g_rating` tail right after the first case and the
  grade-1 case as a lone `mov eax, 1; jmp` block at the very end. That is a `bool titled`
  flag set by the grade 5/6 cases and `int title = titled ? <first word> - 13 : 0;` after
  the chain: VC threads every case's jump through the flag test (the 5/6 cases get their own
  copy of the title load, the others jump to the `title = 0` block), and the threaded copy
  for the grade-1 case is created late and placed last. Per-case `title = ...` stores leave
  the tail at the end, and an `if (titled) { title = ...; }` after an `int title = 0;` is not
  threaded. `CEnmParam::Plan` works the same way: its kinds loop
  sets only `side->mode` in each case and then resets the count once with `if (side->mode
  != 0) { side->rapid_kills = 0; }`. VC threads that test (it knows the stored mode) into
  the per-case `mov word ptr [eax], K; jmp` blocks and the shared count store, but the test
  still counts for the allocator, which then keeps 0x20000 in ebp for the flags test and
  `AddTactic(17)`.
* An empty user-declared destructor changes the code of the implicit destructor that contains
  the class: `~HLSTxtSys() {}` gives the original's inlined member destruction in `~Fonts`
  (0x401f80).
* `SetIdentity()` instead of the nine written-out stores reorders the stores and the `lea` of
  the matrix (`ActorSystem::InitRoot`).
* A local reference to a member (`const unsigned int& flags = work->flags;`, the tests read
  `flags`, the updates write `work->flags`) gives the register choice of
  `PartControl::OnDamage`. A local pointer to a member list (`CSndBufList* list = &buffers_;`)
  places the increment in `SoundSystem::Pause`, and `CEnmParam::Plan` steps a pointer along
  with the index (`EnmSideStats* side = state_.sides; ...; ++i, ++side`).
* The `.text$yc`/`$yd` dynamic initializers and atexit thunks only exist in the TU that defines
  the global. Defining the globals in their original objects (`g_replay`, `g_fn_title`,
  `g_fn_game`, `g_fn_option`, `g_sounds`, `g_bgm` in `cglobal.cpp`; `g_application`,
  `g_pat_sys`, `g_anm_sys`, `g_actor_system` in `win_main.cpp`; `g_hit_image` in
  `actor_system.cpp`; `g_laser` in `line_sprites.cpp`; the static members of `HLSPat` and the
  unused `g_unused_null_sprite` in `hls_pat.cpp`) puts each stub in the right object. A thunk
  counts only with at least one checked (named) relocation. In `HLSPatFile::Load` a `max_id`
  local that drives `count_` gives the original frame size, and with it the `PackFile`
  funclet (0x4502a0).
* VC7.1 rotates `for (;;)`, `while` and `do ... while (true)` loops (the test moves to the
  bottom, with a jump into it and alignment padding). A `do { ... } while (p != NULL);` whose
  condition repeats a test of the body keeps the loop as written: test and exits at the top,
  `jmp` back from the bottom (`PartControl::OnDestroy`).
* `mov al, [esi+0x34]; mov ecx, 1; test cl, al` is a flag test stored in a `bool` local
  (`bool dashing = (flags & kDashing) != 0; if (dashing)`); the plain `if (flags & 1)` gives
  `test al, 1` (`BossBodyControl::RunMoveProgram`).
* VC does not unroll loops with calls in them (it does fully unroll the six-pass arithmetic
  loop of `GameHud::UpdateChart`): four calls in a row in the original are four calls in the source
  (`SendEvent()` to the arms in `BossBodyControl::Dash`). A `return 0` placed after all the other
  code is `if (ok) { ...; return 1; } return 0;` rather than an early `if (!ok) return 0;`.
* Dead parameter slots: VC7.1 puts spilled locals and temporaries into the slots of
  parameters that are dead, and a dead `short` parameter's slot is taken first, before the int
  and pointer ones (which go in address order). `HLSTxtSys::Initialize` takes `char_count`
  as an int (its member is `short`): a short parameter rotates the whole assignment by one
  slot. Parameter and local names do not matter.
* A global that only one file uses may also be a file-`static` that stands in for a static
  member: `PlayerControl::weapon_` (0x4690ec) is `g_player_weapon` in `player.cpp`. With a static
  member, the inlined `SelectWeapon()` puts the laser in the wrong register or loads it
  after the store to the weapon; with the file-static, VC knows that the store does not alias
  `laser->color` (`PlayerControl::OnCreate`, `OnMove`).
* A local copy of a member next to the member itself: `BossBodyControl::Dash`'s brake formula
  takes the acceleration from a local (`float acceleration = work->deceleration;`, declared in
  the `if`) and the deceleration from `work->deceleration` (v^2 = 2 d a b / (a + b), brake after
  v / a frames, with a == b here). The local is then homed in the dead `actor` slot and
  `distance2` in its own local, as in the original; with one name for both, VC keeps one of
  the two on the FPU stack.
* The author reuses variables, and a reused variable's longer life changes slots and
  registers. `BossBodyControl::StartTurn` (16-bit `angle`, as its caller shows) stores the peak of a
  short turn in `angle` itself (`angle = sqrt(...); turn_peak = angle; turn_end = angle * 2;`):
  `angle` then stays live and the `sqrt()` temporary goes into the dead `actor` slot (with
  `turn_peak` written directly it takes `angle`'s slot). `HLSTxtSys::Print` turns the
  character code into the glyph index in place (`code -= first_char_`, no `index` local):
  `code` then lives across `DrawGlyph()` and is kept in the dead `text` slot, stored as a dword
  and reloaded as a word. A probe TU compiled with `/FAs` (cl writes `tvNNN = 16` and
  `_angle$ = 16` lines for each slot) answers slot questions in seconds.
* Per-case calls with a shared tail: a switch whose cases each call a function and clear the
  op on failure is `if (!Call(...)) { program->op = 0; } break;` in every case, with the
  common `++program->time` after the switch and the other path in an `else` (not an early
  `return`): VC cross-jumps the calls into one call site, which loads the parameter
  registers once at the top (`BossBodyControl::RunMoveProgram`). A `++program->time;
  return;` per case keeps separate tails.
* `fcomp; fnstsw ax; test ah, 0x41; jp` is `if (x <= 0.0f)`; `!(x > 0.0f)` gives `je`
  (they differ only for NaN). A `PartWork* part` local for `part->flags |= ...`, instead
  of the cast written out, keeps the `or dword ptr [esi+0x34], ecx` memory form in
  `RunMoveProgram` (with the cast VC splits it into load/or/store and hoists the load).
  Updating a member in place (`work->turn = a - b; work->turn += work->turn >= 0 ? 45 :
  -45;`) gives the original's `sets` on the subtraction's flags where a `done` local gives
  `test`/`setge` (`RunTurnProgram`).
* C-style declarations at the top of the function share stack slots differently from block
  locals: `HLSPatFile::Load` declares the pattern record's locals at function scope and
  reads the header's highest id into the same `id` local. Scope does not change register
  choice.
* The mangled names are made up, so parameter types are free as long as the callee still
  matches. A short local passed to a short parameter is pushed with a dword load; the
  original's `movsx` before the push means the parameter was an int (`HLSPatSys::Create`'s
  u and v). A short parameter that the original loads as a dword and sign-extends from the
  register later was an int stored into a short member, with the later use reading the
  member (`HLSTxtSys::Initialize`'s `txt_count`, `new HLSTxt[txt_count_]`).
* A pointer member that the original reads once for two computations and reloads for the
  next pair is an inline helper taking it as a parameter (`HLSTxtSys::GetTexCoord()`, the
  inline twin of `HL3D::GetTexCoord()`, for each glyph corner).
* `(c) + 1` gives `sbb`/`neg`/`inc`; `c ? 2 : 1` gives `xor`/`cmp`/`seta`/`inc`.
* Block order follows the if/else orientation: VC emits the then-block first and lets the
  else-block fall into the code after the `if`. `CSndFileSEQ::OpenPiece` tests
  `rate_ != 0` first (the first piece sets the format in the else branch) and leaves through
  `goto failed` to the one Close/delete path (the author uses `goto failed` elsewhere too,
  `HLSTxtSys::Initialize`). `cGlobal::NextReplay` returns 1 from inside the last-record test
  and ends with `if (next[0] == 0) { if (first[0] != 0) copy first; else return 0; } else
  copy next;`.
* A negation computed before an `if` whose two branches use it (`fld; fchs` above the `je`)
  is a local declared in each branch (`float left = -offset;` in `PlayerControl::Fire`): VC
  hoists the two negations and stores `offset` with `fstp`/`fld`; one local before the `if`
  keeps it with `fst`, and `-offset + x` written in both branches becomes `x - offset`.
* **Same-TU escape analysis; the draw helpers are header inlines.** VC7.1 lets a
  load through a pointer pass stores to an address-taken local only when it can prove the
  pointer does not hold the local's address. It can when every function that receives the
  local's address has its body in the TU and lets the pointer go nowhere unknown (a probe
  with `Draw(const T*)` defined in the TU hoists the texture test above the transform stores;
  with `Draw` only declared, it stays below). The original's `HL3D::DrawTexels`,
  `HL3D::DrawPoints`, `MultiplyTransform` and `RotateTransform` were inline functions in
  headers, too big to inline, so every TU that calls them compiles a COMDAT copy and LINK
  keeps the first object's: each sits in the object of its first caller in link order
  (DrawTexels before `FnOption::Draw` in obj66, DrawPoints before `ShockwaveSprite::Draw`,
  RotateTransform before `BackgroundGrid`, MultiplyTransform before `RotateTransformX`).
  With them in `hl_3d.h`/`background.h`, `DrawResult`, `DrawNameEntry`, `FnOption::Draw`
  and `BackgroundGrid::Run` match as written, and the helpers still match from their first
  objects. A load through a file-`static` pointer whose address is never taken is also
  hoisted (VC knows every value stored to it), and so is one through a pointer parameter;
  a pointer read from an extern global or member is not.
* **A named copy of a global pointer blocks CSE through it.** `FnOption::Move` keeps
  `g_application.input` in edi and reads `pad(0)` again for `GetPressed()`: the source
  declares `HLInput* input = g_application.input;` right before
  `g_application.input->pad(0)->pressed()` (which still reads the global), uses
  `input->pad(0)->GetPressed()` in the button case and the global again for `set_trigger()`.
  VC then shares the `g_application.input` load with `input` but does not treat
  `input->pad(0)` as the same value as the earlier `g_application.input->pad(0)`. Reading
  `pressed()` through `input`, a `pad` local, or the global everywhere makes VC keep the pad
  (and moves the 0 constant into ebp).
* **Struct assignment pins base registers.** `chart_[6] = chart_[0]` (a
  `D3DVECTOR` copy) made VC keep `&chart_[0]` and `&chart_[6]` in registers and address
  every corner store off them (`lea eax, [ecx+0xb8]`, `add ecx, 0x100`, short
  displacements). `GameHud::UpdateChart` copies the three fields one by one, with a
  `D3DVECTOR& corner = chart_[i]` per pass (which keeps each corner's loads after the
  previous stores), which gives the original's `[ecx+disp32]` code.
* **Where a value is computed changes the whole allocation.** `HLSurface::Load` declares
  `width` from the image before `bytes` and divides it in place, with a local copy of
  `scale_y_` and one `int pad;` shared by all cases; each of these moves registers across
  the whole function.
* **Callees that are out of line:** `HLSurface::Lock` and `HL3D::SetTexture`
  (0x407820/0x407880, in obj19 after `HLSHitImage`) are not header inlines (their callers
  come later in link order, so a COMDAT would sit in a later object); the `HLImage`
  constructor, `Load` and `Release` live in `hl_image.cpp`'s objects; `Object::Log` is out
  of line in `object.cpp` (obj09).
* `tools/fas.py --print` compiles a probe TU with `/FAs` in about a second, which answers
  register and slot questions quickly.
