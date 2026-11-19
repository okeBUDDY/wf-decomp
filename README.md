# Warning Forever 1.08 decompilation

A byte-matching decompilation of `wf.exe` from Warning Forever 1.08 (Hikoza T. Ohkubo, 2005).
The exe is a PE32 file with image base 0x400000 and no relocations. It was built with
Visual C++ .NET 2003 (cl 13.10.3077, LINK 7.10). Its third-party libraries were built with
Visual C++ .NET 2002 (cl 13.00.9466).

The current state:

* All 361 identified library functions (libogg 1.0, libvorbis 1.0, zlib 1.1.4,
  libpng 1.2.5) are rebuilt from source and match byte for byte.
* The whole exe is relinked with the original LINK from game "blob" objects,
  the rebuilt libraries, libcmt and the SDK import libraries. Every section (`.text`,
  `.rdata` with the import table, `.data`, `.rsrc`) is byte-identical; only the Rich header
  and the link timestamp differ (listed below).
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
  `__fltused`, `__except_list`, `___security_cookie` and `_atof`.
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

Baseline (differing bytes): headers 128, `.text` 0, `.rdata` 0, `.data` 0, `.rsrc` 0 (the
header count can drop by a byte or so when a byte of the link time happens to equal the original's).
With the optional `tools/postlink.py` stamp (`build/wf-stamped.exe`): headers 124, all
sections 0.

| Section | Cause | Bytes | Fixable |
|---|---|---|---|
| headers | Rich header: `Utc13_CPP 9178` (PSDK `dxguid.lib`) vs `Utc1310_CPP 4035` (a newer DirectX SDK's `dxguid.lib`, built with cl 13.10.4035). Every Rich DWORD is XORed with a key that sums all entries, so this one entry changes all 124 bytes | 124 | No: that SDK is not in the toolchain; the data it adds already matches |
| headers | Rich header: `Utc1310_C 3077` 150 vs 151 (one more libcmt C object, see below) | (in the 124) | Known how, but it makes the header worse until dxguid matches |
| headers | Rich header entry order: `Implib710 2067` is first seen after `Utc13_C 9466` instead of right after the dxguid entry | (in the 124) | Not examined; probably the newer dxguid.lib too |
| headers | TimeDateStamp | 4 | Only by the post-link stamp |

The `CheckSum` is 0 in both files. No difference comes from the 15 unmatched game functions:
they link from the original bytes. The Rich `Import` (173) and `Implib` counts equal the
original's since step 27 (from the import fix).

**The missing libcmt object.** The original pulled 151 libcmt C objects (`Utc1310_C 3077`),
the relink 150. The extra one left no bytes in the exe, so it was pulled by discarded game
code, has only COMDAT code (no `.data`/`.bss`/`.CRT$` contributions) and needs nothing that is
not already linked. 116 libcmt members qualify (`__vsnprintf`, `_strtok`, `__snprintf`,
`_strlwr`, the `_mbs*` functions, ...), so it cannot be identified. An `extern __vsnprintf`
(a debug trace next to `OutputDebugStringA` would fit) gives 151 and leaves every section
identical, but it moves the PE header from 0x110 to 0x108 (headers 128 -> 289), so it is not
applied. The reason is in LINK (`link.exe` 7.10 at 0x4591b0): the Rich buffer is
`entries * 8 + 0x20 + 8 * ((key >> 5) % 3)` bytes, so the zero padding after `Rich` (and with
it `e_lfanew`) depends on the XOR key. The key is a checksum over the DOS header and all
entries, so with our dxguid entry, 150 objects happen to give the original's padding (16) and
151 give 8. Once the dxguid entry matches, the extern should be added with it.

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
not depend on them, because the units stay contiguous and in order. They matter once a whole
unit is compiled from source. Correct them as decompilation shows where the source files end.
Known case: `obj69_41eef0` holds four source files (`game_hud.cpp` and the three screens,
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
* Functions that do not match yet keep the original bytes.
* File-`static` functions (storage class 3, such as `ReadPng` and the ogg callbacks) are
  linked from source too; `mkblobs.py` reports how many functions it took, which should equal
  compare.py's match count.
* With every matching function linked from source, the relink is unchanged.

A unit with `src=` whose object has been built is linked from that object directly (`link.py`).
That only gives a matching layout once the unit's data (vtables, strings, EH tables) also
matches.

## Decompiled code

673 of 678 game functions match (95.78% of the 128586 game code bytes; `tools/compare.py`),
and all 151 compiler-generated EH stubs (the `.text$x` unwind funclets and
`__ehhandler$` stubs, and the `.text$yc`/`.text$yd` dynamic initializers and atexit thunks at
0x44fed0-0x45092a) equal what cl emits for the TU that compiles their parent. The stubs are
counted apart from the game functions; `build/progress.json` has both. A `.text$x` funclet is
placed by its parent's push of `__ehhandler$<parent>`, so it is checked at its exact
address; a `.text$yc`/`$yd` thunk counts when it equals a stub of its size whose named
relocation targets agree. Matched stubs still link from the blob.

Every game function now has source; all matching ones are linked into `build/wf.exe` from
source. Per file (matched/compiled; the rest are `// TODO: nonmatching` drafts; functions
emitted in several files, such as header inlines, count in each):

| Area | Files | Matched |
|---|---|---|
| Core objects | `object.cpp` 13/13, `task.cpp` 9/9, `task_list_delete.cpp` 1/1, `thread.cpp` 2/2, `file.cpp` 6/6, `pack.cpp` 8/8, `path_util.cpp` 2/2, `profile.cpp` 2/2 | 43/43 |
| Application, globals, stages | `win_main.cpp` 7/7, `application.cpp` 16/16, `cglobal.cpp` 34/34, `game.cpp` 10/10, `fn_game.cpp` 34/35 (with FnOption), `fn_title.cpp` 11/11, `game_hud.cpp` 9/9, `game_hud_game_over.cpp` 13/13, `game_hud_name_entry.cpp` 9/9, `game_hud_result.cpp` 5/5, `graphics.cpp` 13/13 | 161/162 |
| HL library (device, input, 3D, camera, images, surfaces) | `hl_device.cpp` 21/21, `hl_device_init.cpp` 5/5, `hl_window.cpp` 13/13, `hl_sync.cpp` 17/17, `hl_input.cpp` 19/19, `hl_3d.cpp` 16/16, `hl_camera.cpp` 6/6, `hl_camera_init.cpp` 1/1, `hl_math.cpp` 5/5, `hl_image.cpp` 23/23, `hl_surface.cpp` 13/14, `sprite.cpp` 8/8, `line_sprites.cpp` 10/10 | 157/158 |
| HL sprite systems | `hls_anm.cpp` 16/16, `hls_pat.cpp` 37/37, `hls_txt.cpp` 18/18, `hls_txt_print.cpp` 2/2, `hls_hit_image.cpp` 8/8 | 81/81 |
| Sound | `snd_sys.cpp` 29/29, `snd_buf.cpp` 27/27, `snd_channel.cpp` 7/7, `snd_strm.cpp` 12/12, `snd_file_wav.cpp` 14/14, `snd_file_ogg.cpp` 21/21, `snd_file_seq.cpp` 36/36 | 146/146 |
| Actors and controls | `actor_system.cpp` 33/33, `actor_control.cpp` 68/68, `core_controls.cpp` 37/37, `boss_controls.cpp` 17/17, `boss_helpers.cpp` 4/4, `effect_controls.cpp` 5/5, `explosion_controls.cpp` 11/11, `player.cpp` 10/10, `background.cpp` 10/10 | 195/195 |
| Enemy generator (boss design) | `enm_param.cpp` 25/28, `enm_plan.cpp` 18/18, `enm_tactics.cpp` 17/17, `enm_tactic_helpers.cpp` 4/4, `enm_battle.cpp` 2/2, `enm_title.cpp` 3/3 | 69/72 |
| Scores and replays | `score_table.cpp` 20/20, `replay.cpp` 19/19, `record.cpp` 4/5 | 43/44 |

Known compiler behaviour worth remembering:

* VC7.1 tracks which callee-saved registers a same-TU function clobbers and lets callers
  keep values in the others. A helper that the original calls (instead of inlining) and whose
  callers assume it saves everything belongs in a separate TU.
* A store through a pointer right before an increment keeps VC from folding the two (see
  `EnmList::Wait` / `SetArgAndNext` in `enm_param.h`).
* `strcpy` from a string literal becomes `rep movsd`; the original's byte loop comes from
  copying through a `const char*` variable that holds the literal (`GameHud::Initialize`).
* A file-`static` pointer whose address is never taken is not reloaded after stores through
  other pointers; a global one is (`kNameChars` in `game_hud_name_entry.cpp`).
* Transforms are built with the inline helpers `ScaleTransform`/`TranslateTransform`
  (`hl_3d.h`): their arguments are loaded before the body, which fixes the FPU order.
* `Lerp(int*)` is inlined at some call sites only (`MoveGameOver` step 4); try it when a
  "if (time >= frames) ... else (to - from) * time / frames" shape shows up.
* Header inlines can blow the inline budget of a big function; a small helper that the
  original inlines in every case may need `__forceinline` (`CSndFileSEQ::Play`).
* A ternary (`x = c > 0 ? c : -1`) and an `if`/`else` with two stores compile differently;
  try both. So do "assign, then reassign under a test" and an `if`/`else` that assigns in
  both arms: the forms leave the same instructions but shift the scratch registers or swap
  esi/edi (step 38: `HLSurface::InitTextureFormats` and `HLSTxtSys::Measure` matched this
  way). When a draft differs only in register names, rewrite every conditional assignment
  in it the other way. The branch order of `if (a <= 0) ... else ...` decides which side falls through.
* A value the original keeps in a register across a test (for example a constant used by a
  later ternary's other arm) shows the source: `mov eax, 2` before a test was the `: 2`
  arm of `HLSurface::InitTextureFormats`' kind choice (which matched as an `if`/`else`).
* `tools/compare.py -v` lists the offsets of the first differing bytes.
* Several classes sit in one original object: obj66 holds both FnGame and FnOption (and
  the out-of-line copy of the header inline `HL3D::DrawTexels`), obj04 holds WinMain with
  the definitions of `g_application` and `g_pat_sys`.
* Store scheduling follows the source order more than it seems: constant stores keep their
  order, but a register constant (0, 1.0f, `true`) is loaded early and its stores move up
  when its first store comes early in the source. Moving one assignment (`SetScale()` right
  after the rotation in `Node::Node`, `unknown_18_ = 0` after the scale in `HLSurface()`,
  `team` before `angle` in `Control23::OnCreate`) fixed register and vtable-store order.
  Trying every single-line move of a block is cheap and often finds it.
* A `memset` of a struct sits right before that struct's field stores; hoisting all the
  memsets to the top changes the constant CSE (`Control42::Initialize`). A getter result
  stored after the null-check merge (one store, not one per branch) went through a local
  (`HLSObject* sprite = ...Get(8)` in `Control16::Initialize`).
* Copying two floats through the FPU (`fld` early, `fstp` late) instead of integer moves is
  an inline setter with float parameters (`Actor::SetPosition` in `Control01::Load`).
* `i / 48` compiled as a signed divide (`imul`/`sar`) rather than `mul`/`shr`: the loop
  counted `i` from 0 to 48 and divided `i * 0x400 / 48` (`Control16::OnCreate`).
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
  (`cmp word ptr [eax+0x18], di`). With the `unsigned short` types `Control01::Fire` was 98
  instructions off (it matches now). Every matching function kept matching once `MovePart`'s
  `old_angle` local became a `short` too.
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
  pushed before `this` is chosen (`MoveGameOver`'s title font); a ternary object picks first.
* Loop shape changes register choice: `CSndFileSEQ::Read` is a `for (;;)` with the read at the
  top, not a read before a `while`. A loop inside `if (n > 0) { ...; return x; }` keeps the
  empty-case return out of line (`CEnmParam::CountDestroyed`).
* `tools/permuter/` runs decomp-permuter on one function (see its README). Inline the helpers
  the function calls by hand in `base.c` first: on `ScoreTable::Insert` the hand-inlined base
  already scored 0, which showed that the original wrote `FindRank()`'s search out in place.
* `if (!a() || !b()) return 0;` and `if (a() && b()) { ... }` compile to the same code, but
  separate `if (!a()) return 0; if (!b()) return 0;` statements place the shared `return 0`
  block differently. That fixed `CEnmParam::Save`/`Load`, `CSndFileOGG::Open` and
  `cGlobal::Initialize`. Early returns also fixed `Actor::ClampAngle` and
  `InputTask::IsProtected` (reading `devs_[i]` into a local before the null test).
* A search loop that does its work and returns from inside the loop (`return 1` in the loop,
  `return 0` after it) differs from `break` plus a test after the loop (`PackFile::Open`).
* The reverse of the file-static lead also happens: `pack.cpp`'s globals had to be plain
  globals for `OpenPack` and `PackFile::Open` to match.
* Updating a member in place (`turret->angle += offset`) and updating a local copy then storing
  it choose different registers (`PartControl::MovePart`). Short locals for repeated
  subexpressions fixed `LaserSprite::Move`; a `names_size` local fixed `CSndFileSEQ::Open`.
* A struct the original copies with word moves was copied field by field
  (`Replay::Start`'s header rules); a plain struct assignment copies dwords.
* `memcmp` argument order shows in `repe cmpsd`: `esi` is the first argument
  (`ScoreTable::Load`/`Store` compare `&rules_` against the file's copy).
* Identical per-case tails are cross-jumped. `Replay::LoadHeader` sets per-case `stage`,
  `score` and `timer` locals and stores them once after the switch.
* `if (format != kAuto) { switch (format) ... }` instead of a bare switch fixed `HLImage::Load`.
* Same-TU auto-inlining tells files apart: `FnOption::Initialize` inlines `Sprite::FadeIn`
  only when compiled with the FnGame functions, so `fn_option.cpp` was merged into
  `fn_game.cpp` (obj66). The four `game_hud*` files must stay separate. The
  `Channel::SetVolume`/`Release`/`Play` functions sit in obj24 and live in `snd_sys.cpp`.
* The declaration order of locals decides their registers and stack slots (`Actor::Destroy`:
  `layer`, `y`, `x`; `CEnmParam::GetParams`: `side` before `total`). A matrix that
  `DrawGameOver` builds starts from `SetIdentity` and then `TranslateTransform`.
* Check that function sizes in `symbols.txt` stay inside their unit's `.text` range. The
  generated cut at 0x40e910 fell inside `CSndFileOGG::Open`; once Open matched, its compiled
  section overflowed the unit and moved the libraries. `splits.txt` is hand-corrected there.
* Inline helper bodies: `EnmSideStats::Score()` returns early for mode 0 (the ternary form
  swapped registers in `GetParams`); `OnActorDestroyed` clamps the slot's power written out
  in place (through `AddClamped()` the flags load moved after the add).
* `Application::Initialize` tests the device and the window in one `||` condition with the
  `set_title` call between them (`a() || (set(), b())`): that places the shared `return 0`
  after the input failure and lets the later failures return AddSystem's 0 in eax.
* Member types show in conversions: `FnOption::enabled_` is an `unsigned char` array, not
  `bool`, and `SetPreset`'s flag an `int` (the original zeroes ecx before the `sete`; a `bool`
  store needs no zeroing).
* A member pointer kept as `add edi, 0xc` (the address of `device_`) and reloaded after the
  call is an inline member using `this->device_` twice: `HL3D::SetPointFilter()` in
  `DrawResult`/`DrawNameEntry`, where an explicit `d3d->device_->...` pair keeps `d3d` instead.
* Unresolved leads: `RateGame` (`record.cpp`) keeps the shared `g_rating` tail
  right after the first case (both modes 0) and the other cases jump back into it; VC puts
  it at the end here. Per-branch `SetRating()` calls (with or without `return`), a `goto`
  into the first case, `title = 0` as the initializer, a wrapping `if (mode != 0 || ...)`
  and `/Os` do not move it, and the permuter found nothing in 25 minutes.
  (`HLSTxtSys::Measure` and `HLSurface::InitTextureFormats`, listed here before, matched
  in step 38.)
  (`SoundSystem::Pause` and `Control32::RunProgramB0` matched in step 41, see there.)
  `SoundSystem::Pause` (2 bytes, `or edx, edi` operands): the
  setter's operand order, `count + 1`, a separate `++count`, `count++` in the test, a
  function-scope `count`, a `const unsigned int&` to the flags, `&=`/`|=` statements in the
  setter and a bitfield do not change it; the permuter's fix takes `&count`, so `count` may
  have gone through a by-reference helper.
  `CEnmParam::Plan` is the original size; it spills `rate` in the prologue (the 0x20000 in
  ebp was fixed in step 41). `FnGame::Initialize` uses ecx for `g_application.input` where
  this picks eax; the parameter type of `set_time_stop_mask`, a `bool` or `unsigned` flag, an
  `input` local, an inverted test, a `mode` local, a `default:` case and moving the flag read
  do not change it (the permuter found nothing either).
  `Control32::RunProgramB0` (15 instructions): only case 0x20004 is left. The
  original adds the turn into `from`'s register (`add ebx, eax; push ebx`), so the push of
  case 0x20001 stays its own; here it is `add eax, ebx; push eax` and the two pushes are
  cross-jumped. An `int from` gives the right add but loads `from` with `movsx` instead of
  `mov bx`; `short`/`unsigned short` `from` and `off` in every combination, `from += off`,
  `off + from`, a `short`/`int` result local, a `WrapAngle()` inline, function-scope
  `from`/`off` and an `unsigned short` StartTurn parameter all give `add eax, ebx` (a probe
  TU shows VC canonicalizes all of them the same). A 45-minute permuter run found nothing.
  `CEnmParam::Plan`: the original
  computes `rate` between the travel average and its conversion (and so spills it); every
  order of the `count`, `add` and `rate` lines and a float `travel` local compute `rate`
  either before or after the `_ftol`. `CEnmParam::Grow`: `code` reused as `base`, `index` as
  the first loop's counter, and `base`/`count` computed before the hits check do not help;
  a 45-minute permuter run only got from 1880 to 1485 with an unnatural `code << 3`
  temporary computed before the hits check.
* An empty user-declared destructor changes the code of the implicit destructor that contains
  the class: `~HLSTxtSys() {}` made the inlined member destruction in `~Fonts` (0x401f80)
  match.
* `SetIdentity()` instead of the nine written-out stores reorders the stores and the `lea` of
  the matrix (`ActorSystem::InitRoot`).
* A local reference to a member (`const unsigned int& flags = work->flags;`, the tests read
  `flags`, the updates write `work->flags`) fixed the register choice of
  `PartControl::OnDamage`. A local pointer to a member list (`CSndBufList* list = &buffers_;`)
  moved the increment in `SoundSystem::Pause`; a pointer stepped along with the index
  (`EnmSideStats* side = state_.sides; ...; ++i, ++side`) took 343 bytes off
  `CEnmParam::Plan`.
* The `.text$yc`/`$yd` dynamic initializers and atexit thunks only exist in the TU that defines
  the global. Defining the globals in their original objects (`g_replay`, `g_fn_title`,
  `g_fn_game`, `g_fn_option`, `g_sounds`, `g_bgm` in `cglobal.cpp`; `g_application`,
  `g_pat_sys`, `g_anm_sys`, `g_actor_system` in `win_main.cpp`; `g_hit_image` in
  `actor_system.cpp`; `g_laser` in `line_sprites.cpp`; the static members of `HLSPat` and the
  unused `g_unused_null_sprite` in `hls_pat.cpp`) took the stubs from 122 to 149. A thunk counts
  only with at least one checked (named) relocation. In `HLSPatFile::Load` a `max_id` local
  that drives `count_` gave the original frame size, and with it the `PackFile` funclet
  (0x4502a0).
* VC7.1 rotates `for (;;)`, `while` and `do ... while (true)` loops (the test moves to the
  bottom, with a jump into it and alignment padding). A `do { ... } while (p != NULL);` whose
  condition repeats a test of the body keeps the loop as written: test and exits at the top,
  `jmp` back from the bottom (`PartControl::OnDestroy`).
* `mov al, [esi+0x34]; mov ecx, 1; test cl, al` is a flag test stored in a `bool` local
  (`bool dashing = (flags & kDashing) != 0; if (dashing)`); the plain `if (flags & 1)` gives
  `test al, 1` (`Control32::RunProgram9c`).
* VC does not unroll loops with calls in them (it does fully unroll the six-pass arithmetic
  loop of `GameHud::UpdateChart`): four calls in a row in the original are four calls in the source
  (`SendEvent()` to the arms in `Control32::Dash`). A `return 0` placed after all the other
  code is `if (ok) { ...; return 1; } return 0;` rather than an early `if (!ok) return 0;`.
* Dead parameter slots: VC7.1 puts spilled locals and temporaries into the slots of
  parameters that are dead, and a dead `short` parameter's slot is taken first, before the int
  and pointer ones (which go in address order). `HLSTxtSys::Initialize` matched once
  `char_count` became an int (its member stays `short`): with the short parameter the whole
  assignment was rotated by one slot. Parameter and local names do not matter.
* A global that only one file uses may also be a file-`static` that stands in for a static
  member: `Control01::weapon_` (0x4690ec) is `g_player_weapon` in `player.cpp`. With the static
  member, the inlined `SelectWeapon()` either put the laser in the wrong register or loaded it
  after the store to the weapon; with the file-static, VC knows that the store does not alias
  `laser->color`, which fixed `Control01::OnCreate` and `OnMove` (Load kept matching).
* A local copy of a member next to the member itself: `Control32::Dash`'s brake formula
  takes the acceleration from a local (`float acceleration = work->deceleration;`, declared in
  the `if`) and the deceleration from `work->deceleration` (v^2 = 2 d a b / (a + b), brake after
  v / a frames, with a == b here). The local is then homed in the dead `actor` slot and
  `distance2` in its own local, as in the original; with one name for both, VC kept one of the
  two on the FPU stack.
* The author reuses variables, and a reused variable's longer life changes slots and
  registers. `Control32::StartTurn` (16-bit `angle`, as its caller shows) stores the peak of a
  short turn in `angle` itself (`angle = sqrt(...); turn_peak = angle; turn_end = angle * 2;`):
  `angle` then stays live and the `sqrt()` temporary goes into the dead `actor` slot (with
  `turn_peak` written directly it took `angle`'s slot). `HLSTxtSys::Print` turns the
  character code into the glyph index in place (`code -= first_char_`, no `index` local):
  `code` then lives across `DrawGlyph()` and is kept in the dead `text` slot, stored as a dword
  and reloaded as a word. A probe TU compiled with `/FAs` (cl writes `tvNNN = 16` and
  `_angle$ = 16` lines for each slot) answers slot questions in seconds.
* Per-case calls with a shared tail: a switch whose cases each call a function and clear the
  op on failure is `if (!Call(...)) { program->op = 0; } break;` in every case, with the
  common `++program->time` after the switch and the other path in an `else` (not an early
  `return`): VC cross-jumps the calls into one call site, which loads the parameter
  registers once at the top (`Control32::RunProgram9c` matched, `RunProgramB0` is down to
  one register). The `++program->time; return;` per case kept separate tails.
* `fcomp; fnstsw ax; test ah, 0x41; jp` is `if (x <= 0.0f)`; `!(x > 0.0f)` gives `je`
  (they differ only for NaN). A `PartWork* part` local for `part->flags |= ...`, instead
  of the cast written out, kept the `or dword ptr [esi+0x34], ecx` memory form in
  `RunProgram9c` (with the cast VC split it into load/or/store and hoisted the load).
  Updating a member in place (`work->turn = a - b; work->turn += work->turn >= 0 ? 45 :
  -45;`) gave the original's `sets` on the subtraction's flags where a `done` local gave
  `test`/`setge` (`RunProgramB0`).
* C-style declarations at the top of the function share stack slots differently from block
  locals: `HLSPatFile::Load` matched with the pattern record's locals declared at function
  scope and the header's highest id read into the same `id` local. Scope does not change
  register choice (`HLSurface::Load`, `CEnmParam::Grow`, `HLSTxtSys::Measure`,
  `Control32::Dash` and `SoundSystem::Pause` were tried).
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
  is a local declared in each branch (`float left = -offset;` in `Control01::Fire`): VC
  hoists the two negations and stores `offset` with `fstp`/`fld`; one local before the `if`
  keeps it with `fst`, and `-offset + x` written in both branches becomes `x - offset`.
* **Same-TU escape analysis; the draw helpers are header inlines (step 34).** VC7.1 lets a
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
  objects. Other probe results: a load through a file-`static` pointer whose address is never
  taken is also hoisted (VC knows every value stored to it), while a pointer read from an
  extern global or member, the same pointer read again (CSE), a `do { } while (0)` macro, a
  reference, a `const` pointer, `register`, `__assume` and `/Oa` are not. A pointer
  *parameter*, or a named local set before the `{ }` block that declares the local, is
  hoisted too (the step-33 finding), but no natural source form needed that. So when the
  original moves a load across stores to a local that a callee only reads, look for that
  callee's body being visible in the original TU (a header inline, or a shared TU).
* **A named copy of a global pointer blocks CSE through it.** `FnOption::Move` keeps
  `g_application.input` in edi and reads `pad(0)` again for `GetPressed()`: the source
  declares `HLInput* input = g_application.input;` right before
  `g_application.input->pad(0)->pressed()` (which still reads the global), uses
  `input->pad(0)->GetPressed()` in the button case and the global again for `set_trigger()`.
  VC then shares the `g_application.input` load with `input` but does not treat
  `input->pad(0)` as the same value as the earlier `g_application.input->pad(0)`. Reading
  `pressed()` through `input`, a `pad` local, or the global everywhere makes VC keep the pad
  (and moves the 0 constant into ebp). The same local does not fix `FnGame::Initialize`.
* **Struct assignment pins base registers (step 35).** `chart_[6] = chart_[0]` (a
  `D3DVECTOR` copy) made VC keep `&chart_[0]` and `&chart_[6]` in registers and address
  every corner store off them (`lea eax, [ecx+0xb8]`, `add ecx, 0x100`, short
  displacements). Copying the three fields one by one, plus a `D3DVECTOR& corner =
  chart_[i]` per pass (which keeps each corner's loads after the previous stores), gave
  the original's `[ecx+disp32]` code: `GameHud::UpdateChart` matched. Look for this
  whenever the original addresses a member array with plain displacements and the draft
  rebases.
* **Where a value is computed changes the whole allocation (step 35).** In
  `HLSurface::Load`, declaring `int width = image.width() / scale_x_;` after `src` instead
  of before `height` took the draft from 172 to 70 differing instructions (the 888 case
  matches now; 12 after step 36, 9 after step 37), and one `int pad;` shared by all cases from 184 to 172. In
  `CEnmParam::Grow`, reading the side from `code` instead of `base`
  (`(code & kSideMask) == 0x80000000`) moved `this` into ebp (85 -> 68), but that value
  differs from the original's when bits 27-28 of `code` are set, so step 38 went back to
  `base`.
  When a draft's registers are permuted, try moving each computation to its first use and
  try reading an equal value from another variable.
* **Callee visibility, checked for every remaining draft (step 35), changed none:**
  `HLSurface::Lock` and `HL3D::SetTexture` (0x407820/0x407880, in obj19 after
  `HLSHitImage`) are not header inlines (their callers come later in link order, so a
  COMDAT would sit in a later object); the `HLImage` constructor, `Load` and `Release`
  live in `hl_image.cpp`'s objects; `Object::Log` is out of line in `object.cpp` (obj09). Moving `Grow`
  above `AllocSlot`/`ChooseWeapon`, and `inline`/`__forceinline`/an `unsigned` id on
  `AddStep`, leave `Grow` and `Plan` unchanged; `SetRating` as a plain `static` or
  external function leaves `RateGame` unchanged; a `Channel::Suspend()` inline (body in
  the header or in the TU) leaves `Pause` unchanged.
* **`SoundSystem::Pause` in a probe TU (step 35).** `tools/fas.py --print` on a copy of
  `Pause` that includes `snd_sys.h` reproduces the draft in a second. Removing any one
  other statement (the list loop, the critical section, `thread_.Suspend()`, the state
  test) turns `or edi, edx` into the original's `or edx, edi`, so the operand choice is
  register pressure, not the setter. VC couples it with the schedule: when `inc edi`
  comes before `and edx, 0xffff0000` the or goes into edi, when it comes after, into
  edx; the original has the inc first and the or into edx. Twenty forms of the
  increment, setter, list access and loop declarations all keep that coupling.
* **Step 38 (2 new matches: `HLSurface::InitTextureFormats`, `HLSTxtSys::Measure`).**
  * **Conditional assignment form moves registers.** `InitTextureFormats` matched with the
    opaque kind chosen by `if (depth == 16) { width_ = format_codes_[0] != k555; } else {
    width_ = 2; }` instead of the ternary: the ternary allocated one more scratch register,
    which shifted the vtable loads of both `EnumTextureFormats` calls (eax/ecx for ecx/edx)
    and the `Log()` arguments by one. With that, restoring `width_` before `height_` gives
    the original's slots. `Measure` matched by building the double-byte code in an
    `if`/`else` (`code = (text[0] << 8) + text[1]` in one arm, `code = *text` in the other)
    instead of assigning the first byte before the test, which had swapped esi/edi
    (`text` and `first_char_`) and eax/edx.
  * **`CEnmParam::Grow` behaviour fix.** The draft read the side from `code`, which differs
    from the original's `(base & kSideMask) == 0x80000000` when bits 27-28 of `code` are set.
    It reads `base` again (87 differing instructions, was 55), with a `char weapon` local
    for the totals. Findings: removing any one of the three option blocks puts `this` in ebp,
    so this is a balance of use weights. Computing `count` and `base` at the top of the
    else block, before the hits check, gives the original's allocation exactly (`this`
    ebp, `code` ecx, `base` ebx, `count`/`index` edi) but moves those instructions before
    the hits check (113). The original's tail keeps the weapon total in edi, so edi is
    pushed in the prologue. Reusing the `kind` parameter or a `kind`/`hits`/`flags` local
    for the hits check, an if/else chain or a ternary for `limit`, the three orders of the
    `slot == NULL`/`flags & 2` chain, `ChildBase()`, a function-scope `base`, an `EnmSideStats&`
    or an `int side` local, and a `short` mode leave it at 87. A 60-minute permuter run found
    only an unnatural `code << 3` temporary before the hits check and semantic changes.
  * Ruled out this round: in `SoundSystem::Pause`, `count + 1` at the declaration with an
    `== 1` test, a flags local, reference or pointer, a named `next`, a `short`/`int` count,
    a buffer local, a `while` loop and a function-scope channel. In `HLSurface::Load`, a
    `BytesPerPixel()`/`RowBytes()`/`RowSkip()` inline helper (every form), a `bits` local and
    `scale_y_ * x - x`: all keep the product order, and every named raw width still flips the
    allocation (191). In `Control32::RunProgramB0`, if/else forms of `off`, of the case
    0x20001 turn and of the final angle. In `CEnmParam::Plan`, if/else forms of the
    `AddStep()` cost index and of the travel clamp, `rate` split into two statements or
    declared at the top, and `/Op` (69). In `FnGame::Initialize`, removing any one case,
    `SetState()` or the HUD add keeps eax for the input pointer. In `FnGame::RateGame`, an
    early `SetRating(0, 0); return;` puts the first case first but with its own tail (28),
    and wrapping the other cases in `if (sides[0].mode != 0 || sides[1].mode != 0)` (the first case as its `else`) changes nothing.
* **Step 39 (drafts only, no new matches).** Mostly probe work; `tools/fas_variants.py`
  runs edit sets on a probe TU in 0.3 s each.
  * **Product operand order is fragile in VC.** VC reassociates `a * b * c` through named
    locals (`int row_bytes = w * bytes; ... (sy - 1) * row_bytes` is also reordered). The
    order it picks depends on the leaf kinds and even on the field offsets: in a
    probe with `image.bits() / 8` written inline, shifting the image's fields by 4 bytes
    flips the order. A narrow local (`short bits = image.bits();` with `(bits / 8)`
    inline, or a `short`/`char` `bytes`) gives the original's `((scale_y_ - 1) * bytes) *
    width` with `sar` scheduled first. That works in a reduced copy of `HLSurface::Load`
    and in the full function with any one 16-bit case removed, but not in the full function
    (9). A `short`/`char` `bytes` gives the order with a `movsx`/`movzx` (22). Ruled out
    together with `short bits`: `bits == 24` tests, every order of the setup lines, a `pad`
    per case (46), and `*out++` in the 24-bit loops (13). So the order is decided by the
    allocator's pressure, as in `Pause`.
  * `CEnmParam::Grow`: the declaration order of `count`, `base`, `index` and `mode` (all 26
    orders, C-style at the top of the else block) changes nothing (87). `base` before
    `count` gives 85, with the same allocation. Also ruled out: a `__forceinline` helper for
    the whole else block (87; inlining is transparent), an inline `GrowOptions()` for the
    three option loops (124/135), an inline totals helper (87), the base expression written
    into each loop for LICM (110), `base` initialized at the function top (122/132), and all
    combinations of six earlier toggles (85 at best). The structural hint is still the
    tail: the original keeps the weapon total in edi, so edi is pushed in the prologue,
    while ebx (`base`) is pushed only in the else block.
  * `FnGame::Initialize`: VC sinks a load into the `if` that uses it (an `input` local
    after the switch gives the same code). A probe of the tail reproduces eax for every
    form tried: a `bool`/`short`/`char` pause, a direct store, an accessor, an
    `Application` member setter, `pause ? 0x8000 : 0` inside the `if` (VC folds it),
    `goto`/`do while (0)`, and a result local. An `input` local before the switch hoists
    the load (edx). A 45-minute permuter run found nothing.
  * `SoundSystem::Pause`: an `unsigned short` getter or setter, `int` in both, an unsigned
    mask enum and a literal `0xffff0000` mask: none fixes it (2 bytes or worse).
  * `Control32::RunProgramB0`: a probe shows VC canonicalizes every expression form of `from
    + off` (casts, `off += from`, a `to` local, a ternary for `off`). Only an int-typed
    `from` adds into from's register. A 45-minute permuter run found only `unsigned int
    from`, which is the known int case (12, with `movsx`).
  * `CEnmParam::Plan`: float helpers (`ToInt(float)`, `Ratio()`, `Average()`, and an
    `AddClampedRate(short*, float)`) leave it at 64.
* **Step 41 (2 new matches: `SoundSystem::Pause`, `Control32::RunProgramB0`).**
  * **A local reference changes the destination of a commutative operation.** Both drafts
    had VC write `a op b` into the right operand's register where the original used the
    left one. `Pause` matched with `const unsigned int& new_count = ++count;
    channel->set_pause_count(new_count);` (a non-const `unsigned int& c = count;`, or a
    pointer to `count`, gives the same code; an inline helper taking `unsigned int&` does
    not). `RunProgramB0` matched with `const short& from = actor->angle;` in case 0x20004
    instead of an `unsigned short` copy (`const unsigned short& to = from += off;` and
    `unsigned short& to = from; to += off;` also give `add ebx, eax`). `StartTurn` takes a
    `short` and `off` is masked, so neither change alters behaviour. References to locals
    that are only read (`const int& p = pause;`) are pure aliases and change nothing.
  * **`CEnmParam::Plan` 64 -> 49** (permuter hint, 835 from 1770): `const EnmSideStats& first
    = state_.sides[0];` before the `state_.flags & 0x10000` test, with `mode0 = first.mode`
    and `first.progress < 65`, keeps `&state_.sides[0]` in ebp up to the mode reads as in the
    original, which frees ebp for the 0x20000 constant shared by the flags test and
    `AddStep(17)`. Reading only the mode through `first` (and the progress off `state_`,
    as the original's `[esi + 0x12]` suggests) loses it again (64), and so does declaring it
    anywhere earlier. `kinds[0] = kinds[1] = 0;` stores `kinds[1]` first like the original
    (`= {0, 0}`, `= {}`, `memset` and a loop do not). Left: `rate` is not spilled and the
    first progress read is `[ebp + 6]` instead of `[esi + 0x12]`. The
    `kinds` loop now steps a `side` pointer like the progress loop (one `side` variable
    for both), which puts `mov eax, ebp` before `xor edx, edx` as in the original (53 -> 51,
    then 49 with the `kinds` stores). What is left is the rate spill and its knock-on
    alignment padding, and the one progress read. The rate spill: VC orders independent x87
    work so that the later-used value sits deeper (every statement order of `travel`/`rate`
    in a probe computes `rate` first and keeps it in st1 across `__ftol2`); the original
    computes the travel quotient first and so has to spill `rate`. References to `rate`,
    `add`, `count` or the members, `/Op` (rounds every conversion) and a `volatile` rate
    (stored into `score`'s slot, after the `_ftol`) do not reproduce it.
  * `CEnmParam::Grow`: `const short& mode = state_.sides[...].mode` (re-read after each
    loop, so wrong code) gives the original's `this` ebp / `base` ebx, and a `const unsigned
    int& flags = slot->flags` at the top of the else block puts `code` in ecx (71) but
    re-reads the flags for the `& 8` test. Pure alias references (`int& count = count_v;`,
    for `slot`, `count`, `base`, `index`, `mode`, `n`, `i`, `weapon`, `limit`), references in
    the tail (`int& parts`, `const int& total = ++parts;`, `part_total_`), an `EnmSlot& s`
    for the else block or the tail, a `CEnmParam& self`, and nine spellings of the side
    index (`(state_.sides + side)->mode`, `this->`, `? 1 : 0`, ...) leave it at 87; a ternary
    picking `state_.sides[1].mode` or `[0].mode` is the original size but 82.
  * `FnGame::Initialize`, `HLSurface::Load`, `FnGame::RateGame`: reference forms (`HLInput*&`
    / `HLInput&` / `Application&` locals, `int& pause`, `const unsigned int& flags`, `const
    short& w = image.width()`, `const HLImage& img`, `const unsigned char&` scale factors)
    change nothing (a reference to `scale_y_` re-reads it: 267). `RateGame`: a
    `SetRating(grade, title); return;` in every branch but the first is not cross-jumped
    (74, 0x113 bytes); swapping the `grade`/`title` assignments or declarations, `char`
    SetRating parameters and a hoisted `title = 0` are 20-31.
* **Step 40 (drafts only, no new matches).** Probes with `tools/fas_variants.py` and
  `tools/multi_variants.py` (and a 1-hour permuter run on `HLSurface::Load` from the
  `short bits` form, which found nothing).
  * **`CEnmParam::Grow`: the `this`/`base` swap is pressure across the hits check.** Any
    value live across the hits block puts `this` in ebp and `base` in ebx, as in the
    original: `int i = 0;` declared at the top of the else block (with `for (; i < count;
    ++i)`), a `const EnmPlanPart* part = slot->part;` there, or `code & kSideMask` /
    `code << 3` computed there. All of them also move that instruction above the hits check
    (99-118). `code` then still sits in edx (the original loads it into ecx right after
    `mov ebp, ecx`), and edi is still pushed only in the else block. `base | (index + i)`
    with `index += n` after each option loop flips it as well but changes the loops (113).
    The tail with `power_total_ +=` before `++part_total_` puts the weapon total in edi
    like the original (81) but reverses the two stores. Ruled out: `int`/`char`/no weapon
    local, `int&`/`int*` to the weapon total, a `weapon_parts_[w] = ... + 1` form, an
    `AddTotals()` inline (slot or weapon/power parameters), reusing `count` or `index` for
    the hits or the limit, a `flags` local, an uninitialized `i`, `base`, or `mode` at the top of the else block, a
    copy of `code`, the three `slot != NULL && (flags & 2)` / `else if (flags & 2)` / nested
    orders of the if chain (126-146), and five option-loop forms (`for (n = ...; n > 0; --n)`,
    `while (n-- > 0)`, `for (i = n; i > 0; --i)`, `while (--n >= 0)`, `++i, ++index`).
  * **`HLSurface::Load`:** rewrites of the 16-bit pixel expressions that compile to the
    same code (operand and `|` orders, groupings, shift/mask forms, alone and in all 31
    combinations of five of them) never flip the product order, even those that renumber
    the compiler temporaries. Only real code changes in a 16-bit case flip it (for example
    the 565 or 4444/5551 32-bit expression replaced by `src[0]`). `int` getters for
    `bits()`, `width()`, `height()` (with and without `short bits`), function-scope `x`/`y`
    loop counters and counting `y` down change nothing (or are much worse).
  * **`SoundSystem::Pause`:** the two-state coupling holds in 34 more combinations: `list`
    local, none, C-style declarations at the top, the buffer pointer set before the channel
    loop; `Suspend()` inline, written out, through a `Thread*` local; `++count`,
    `count++` in the test, `count + 1`, a separate `++count`, a nested `if`, `count + 1` at
    the declaration with `== 1`; the setter written out in six forms (`&=`/`|=`, a
    `flags` local, `0xffff0000`, operand orders, `count += 1`); a `CSndBufList::SuspendAll()`
    inline (like `ReleaseAll()`). It is always inc-first with `or edi, edx`, or
    and-first with `or edx, edi`. `Resume` shows the original's operand choice when the
    count changes in an earlier block (`--count` in the condition). A 20-minute permuter
    run from the and-first form (no `list` local) found nothing.
  * **`CEnmParam::Plan`:** VC never stores `rate` to the stack. It keeps it on the x87
    stack, before or after the `_ftol` (every order of `count`, a float `travel`, `rate`
    and `add`). Ruled out: double-precision forms (`static_cast<double>` on either
    quotient, `* 1.0`, a `double rate`), `rate * 100` at the declaration, a ternary clamp,
    a `Percent(const float&)` helper, `*&rate`, a block around the `add`/`rate` lines,
    `kinds` declared at the top, and `rate` in a one-element array.
  * **`FnGame::RateGame`:** a `do { ... break; } while (0)`, an inline `Grade()` with
    returns (title through a pointer or chosen from the grade), a `goto` from the first
    case, and the grade-1 case as the final `else` (also nested, and with `title = 0`
    hoisted) all keep the shared tail at the end.
  * **`FnGame::Initialize`:** deleting any one statement of the function (19 variants,
    including the `break`s and `MuteGroup()`) keeps eax for the input pointer. `tools/fas.py`
    hangs on `fn_game.cpp`, so use `multi_variants.py` or a clone's `compare.py` with
    `fdiff.py` for it.
  * **`Control32::RunProgramB0`:** a `Wrap(short&)` helper, a `Turn(const unsigned
    short&, const short&)` helper, and `*&from`/`*&off` all canonicalize to `add eax, ebx`.
  * **Leads.** `Grow`: look for a value that is live from the function entry or the top
    of the else block in the original without code there (it would also explain `code` in
    ecx). Start from `int i = 0;` at the top of the else block, which gives the right
    `this`/`base` registers, and look for a form that keeps the zero store after the count
    test. `Pause`/`RunProgramB0`: in both, the original's `or`/`add` destination is the
    left operand. `Resume` gets that when the right operand changed in an earlier block,
    so look for a form where the increment (or `off`) is computed before the merge.
* **Step 37 (drafts only, no new matches).**
  * **Variable sharing changes another case's allocation.** In `HLSurface::Load` the 565 case
    takes the original's `(((src[2] & 0xf8) << 5 | (src[1] & 0xfc)) << 3) | src[0] >> 3` form
    once the four 16-bit cases share one `unsigned short* out` declared next to `pad` (one per
    case made VC load `src[2]` first and spill the width inside the loop): 12 -> 9 differing
    instructions, only the `row_skip` product is left. It was found by ablation: deleting
    whole cases from a copy of the function (compiled with `tools/fas.py`) showed that the 565
    loop compiles right without the 4444 or the 5551 case, so the cases' combined use counts
    decide it. When one case of a switch is off, try cutting the others to find which one
    pushes it.
  * **Operand order of a product.** VC sorts the leaves of `a * b * c` (every grouping and
    order is the same to it): of two named locals, the one assigned later is multiplied first,
    and a CSE'd member read (`image.width()` used twice) counts as the newest. A probe with a
    local `HLImage` reproduces the draft's `((scale_y_ - 1) * width) * bytes`; the original's
    `((scale_y_ - 1) * bytes) * width` needs a named width assigned before `bytes`
    (`int width = image.width();` ... `width /= scale_x_;`, or a separate `src_width`).
    In the full function that gives the right product, but then either the height is divided
    before the width (height computed first in the source, 17 instructions) or, with the width
    divided first, the whole function's allocation flips (`this` to ebp, the result to ebx,
    `scale_x_` read twice; 191). All 294 dependency-respecting orders of the seven setup lines,
    184 orders with a named raw height too, 720 C-style declaration orders, the positions of
    `pad`/`out`, a switch-value local, a `scale_x` copy and `short`/`const`/`unsigned` widths
    keep one of the two (a `short` width loses the product order). Inline `image.bits() / 8`
    instead of `bytes` fixes the product in a probe with a reference parameter but not with a
    local image. Permuter runs from the 20-, 17- and 191-instruction variants (about 45, 70 and 50
    minutes) found only unnatural temporaries (`result = format_;`, the height computed into
    `result`, a copy of `scale_x_` read before `Create()`) and `bytes` computed before `Lock()`,
    which is worse when ported (48).
  * `CEnmParam::Grow`: the original computes the side as `mov eax, ebx; and eax, 0xc0000000;
    sub eax, 0x80000000; neg; sbb; inc` from `base` (ebx), which is what VC emits for
    `(base & kSideMask) == 0x80000000` (the `code` form gives `cmp`/`sete` from a spilled
    `code & kSideMask`), so the original does read the side from `base`. With it `this` goes to
    ebx and `base` to ebp (82 instructions, 0x19 bytes short); a `char weapon` local gives the
    original's load/`inc`/store for `++weapon_parts_[weapon]` (the int local gives `inc dword
    ptr`) but not its edi. Count/base order, `ChildBase()`, operand order and a written-out
    `FindSlot()` (a `NULL` result or a `goto`) do not move `this` back to ebp. A 50-minute
    permuter run on the `base` form found only semantic changes.
  * `SoundSystem::Pause`: the `or` destination follows liveness. In a probe, a use of `count`
    after `set_pause_count(++count)` (a store to a global) gives exactly the original's
    `inc edi; and edx, 0xffff0000; or edx, edi`: VC overwrites the operand that dies, so in the
    original `count` was still live after the store (the permuter's `&count` does the same).
    The original has no later use of it, so look for a construct that keeps it live without
    code: a variable that outlives the loop body, or one the optimizer drops only after
    register allocation. A function-scope `count` (with or without `= 0`), `count + 1` or
    `count += 1` as the argument, a `const unsigned int&` setter parameter, a reference to the
    flags at the top of the loop (flips both), a buffer local and a nested `if` do not; a
    20-minute permuter run found nothing.
  * `CEnmParam::Plan`: a `float travel` local makes VC compute `rate` first and keep it on the
    x87 stack (same 64 instructions). Note that `rate` shares its stack slot with `kinds[0]` in
    the original (the frame is only the 8 bytes of `kinds`).
  * `HLSurface::InitTextureFormats` has two more differences besides the slots: the vtable of
    the two `EnumTextureFormats` calls is loaded into ecx/edx (eax/ecx here) and the `Log()`
    arguments use the registers shifted by one, as if one more temporary had been allocated
    before them. A `kind` local, an `HRESULT` local, `SUCCEEDED()`, `!hr` and the store order
    of `width_`/`height_` do not change it; neither does any order of the two saved locals and
    `result`. A 30-minute permuter run (after hand-fixing the `D3DDEVICEDESC7`/`DDCAPS`
    stubs in `base.c`) found nothing.
  * `Control32::RunProgramB0` and `Pause` look alike: in both, the original puts the result of
    a commutative operation (`from + off`, `(flags & ~mask) | count`) into the left operand's
    register where VC, with both operands dying, reuses the right one's. In `Pause` a later use
    of the right operand flips VC to the left; in `RunProgramB0` a test store of `off` after the
    call rearranges the whole case, so it says nothing there.
  * `RateGame`: an `else if (both modes != 0) { ... } else { grade = 1; }` chain, `grade = 0,
    title = 0` initializers and the stores written out keep the shared tail at the end.
  * The probe setups are cheap: `tools/fas.py --print` on a probe TU takes 0.3 s. Do not run it
    (or any compile) while a `compare.py` in the same tree rewrites `build/src-sjis`: cl then
    hung at 100% CPU twice. A second clone of the repo (with `orig/wf.exe` linked and
    `build/analysis.json`/`build/lib` copied) runs compare and the permuter independently.
* **Step 36 (drafts only, no new matches).**
  * `HLSurface::Load` 70 -> 12 differing instructions: the declaration order `src`, `bytes`,
    `row_skip`, `step`, `width`, `height` (all 120 orders tried), `result = 1` before `Log()`,
    and `*out = ...; src += step; ++out;` (the increment after the source step) in the 24-bit
    4444 and 5551 loops. Left: the original multiplies `row_skip` as
    `((scale_y_ - 1) * bytes) * width`, VC as `((scale_y_ - 1) * width) * bytes` for every
    operand order, grouping, split statement, local copy and type of `bytes`/the width (a probe
    TU gives the same); and the 565 green channel is `src[1] & 0xfc` in the original. That
    form gives the original's code in a probe TU with just the loop, but in the full function
    VC loads `src[2]` first and needs a third temporary (49 instructions), so the draft keeps
    the equivalent `((src[2] >> 3) << 6 | src[1] >> 2) << 5` form. VC canonicalizes every `|`
    order and grouping, `<<`/`*`/`&`-after-shift forms and masks like `~3`/`0xfffc`.
  * `CEnmParam::Grow` 68 -> 55: an `int weapon = slot->weapon;` local for the closing totals
    (the original then pushes edi in the prologue), and `count` declared before `base`. Left:
    `code` in edx instead of ecx (so the flags take ecx and the ChooseWeapon calls are not
    fully tail-merged); the side read from `base` and a signed `base` are worse, `ChildBase()`,
    parameter types, `FindSlot` loop forms and `if` shapes change nothing.
  * `SoundSystem::Pause` probes: a reference to the flags (`unsigned int& flags =
    channel->flags_;`) or a copy of the channel pointer gives the original's `or edx, edi`, but
    then the `inc` moves after the `and`. The coupling also holds under `/G5` and `/G7`.
  * `CEnmParam::Plan`: VC keeps `rate` on the x87 stack across `_ftol` (in st1) where the
    original spills it; `1 << 17`, `1u << id`, a `bit` local in `AddStep`, a flags local, `!= 0`
    and the operand order of the `0x20000` test do not put the constant in a register.
  * `HLSTxtSys::Measure`: Print's forms (function-scope `code`, `code -= first_char_`), a
    `first` local, `*++text`, and a separate iteration pointer change nothing.
    `FnGame::Initialize`: no `pause` local, a `bool`/`unsigned` one, an early return and an
    `input` local inside the `if` change nothing.
  * The permuter's `base.c` can get `int const;` stub members from words in comments, which
    pycparser rejects; rename them by hand. The permuter compiles candidates spliced into the
    live source file, so do not edit that file (or run `multi_variants.py` on it) while it runs.
* `build/obj/src/` keeps objects of deleted or merged sources (`fn_option.obj`, ...);
  `fdiff.py` may read such a stale object, so delete them after renaming a file.

Flags: the game uses `/O2` with its default `/Ob2`. `Object()` is inlined into
`TaskList()`, and `Remove` into `RemoveUpTo`/`RemoveFrom`, which `/Ob1` would not do.
`/G5` vs `/G6` has not been told apart by any function yet.
`WF_EXTRA_CFLAGS` (environment) adds flags to every game compile of `compare.py` and the
permuter, for experiments only; no per-file flag is needed so far. A sweep of the 17 files with
nonmatching functions over `/Ob1 /Oy- /G5 /G7 /Op /J /Oa /Ow /EHa /Os /Zp4 /GR /Oi- /QIfist
/arch:SSE /Ox /Gs0` fixed nothing (`/G5` gives the same code as the default everywhere; `/Zi`
hangs cl under Wine when run in parallel). With `/Ob1` only `enm_param.cpp` still compiles its
other functions the same, and it gains nothing. Moving each draft to the end of its file
(definition order) changes none of them.

Tools for decompiling:
* `tools/disasm.py <addr|name|unit>`: disassembly with symbol, string and float annotations.
* `tools/fdiff.py [-n] <addr|name>`: side-by-side diff of the compiled and the original
  function; `-n` aligns without offsets and branch targets and counts the differing
  instructions, which `try_variants.py` and `perm_lines.py` report as well (a finer score
  than differing bytes, which a shifted block inflates).
* `tools/try_variants.py <src> <addr> [--compile <cpp>]`: applies listed source edits one at a
  time and reports each result (`--compile` tests a header edit through a .cpp); `tools/perm_lines.py <src> <addr>` tries every single-line move inside a block.
* `tools/fas.py [--print] <src>` compiles one file with `/FAs` into `build/fas/<name>.asm` (the
  stack-slot table sits above each PROC); the file may be a standalone probe TU outside `src/`
  (it still includes `src/` headers), and `--print` prints just the PROC bodies; `tools/fas_slots.py <src> <func> < variants` prints that table for
  each try_variants-style variant; `tools/multi_variants.py <src> <addr,...> <variants.py>` applies
  several edits per variant (`V = [[(old, new), ...], ...]`, or `(path, old, new)` for a header)
  and scores several functions; `tools/fas_variants.py <probe.cpp> <variants.py> [--grep RE]`
  applies edit sets (`V = [(label, [(old, new), ...]), ...]`) to a probe TU and prints the
  matching instructions of each variant on one line, for ablation.
* `tools/permuter/`: runs [decomp-permuter](https://github.com/simonlindholm/decomp-permuter) on
  one C++ function through a C rendering that is turned back into C++ and compiled in its real
  TU for every candidate (see `tools/permuter/README.md`).

Code style: explicit braces everywhere and the repo `.clang-format` (Google, left pointer
alignment). Use structs and named fields instead of raw offsets, and recover classes with
their vtables and inheritance. Fields not recovered yet are padding arrays named after their
offset. Name decompiled functions by their MSVC-mangled names in `config/symbols.txt`;
`gensyms.py` keeps manual names.
