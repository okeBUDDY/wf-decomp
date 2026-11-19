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
* With the 5 seed functions linked from source, the relink is unchanged.

A unit with `src=` whose object has been built is linked from that object directly (`link.py`).
That only gives a matching layout once the unit's data (vtables, strings, EH tables) also
matches.

## Decompiled code

645 of 678 game functions match (83.95% of the 128586 game code bytes; `tools/compare.py`),
and 122 of the 151 compiler-generated EH stubs (the `.text$x` unwind funclets and
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
| Application, globals, stages | `win_main.cpp` 1/1, `application.cpp` 15/16, `cglobal.cpp` 26/27, `game.cpp` 10/10, `fn_game.cpp` 28/32 (with FnOption), `fn_title.cpp` 8/8, `game_hud.cpp` 9/9, `game_hud_game_over.cpp` 8/9, `game_hud_name_entry.cpp` 4/5, `game_hud_result.cpp` 3/4, `graphics.cpp` 13/13 | 125/134 |
| HL library (device, input, 3D, camera, images, surfaces) | `hl_device.cpp` 21/21, `hl_device_init.cpp` 5/5, `hl_window.cpp` 13/13, `hl_sync.cpp` 17/17, `hl_input.cpp` 19/19, `hl_3d.cpp` 16/16, `hl_camera.cpp` 6/6, `hl_camera_init.cpp` 1/1, `hl_math.cpp` 5/5, `hl_image.cpp` 23/23, `hl_surface.cpp` 12/14, `sprite.cpp` 8/8, `line_sprites.cpp` 10/10 | 156/158 |
| HL sprite systems | `hls_anm.cpp` 16/16, `hls_pat.cpp` 34/35, `hls_txt.cpp` 15/18, `hls_txt_print.cpp` 2/2, `hls_hit_image.cpp` 8/8 | 75/79 |
| Sound | `snd_sys.cpp` 28/29, `snd_buf.cpp` 27/27, `snd_channel.cpp` 7/7, `snd_strm.cpp` 12/12, `snd_file_wav.cpp` 14/14, `snd_file_ogg.cpp` 21/21, `snd_file_seq.cpp` 35/36 | 144/146 |
| Actors and controls | `actor_system.cpp` 32/33, `actor_control.cpp` 66/68, `core_controls.cpp` 37/37, `boss_controls.cpp` 14/17, `boss_helpers.cpp` 4/4, `effect_controls.cpp` 5/5, `explosion_controls.cpp` 11/11, `player.cpp` 7/10, `background.cpp` 7/8 | 183/193 |
| Enemy generator (boss design) | `enm_param.cpp` 23/28, `enm_plan.cpp` 18/18, `enm_tactics.cpp` 17/17, `enm_tactic_helpers.cpp` 4/4, `enm_battle.cpp` 2/2, `enm_title.cpp` 3/3 | 67/72 |
| Scores and replays | `score_table.cpp` 20/20, `replay.cpp` 19/19, `record.cpp` 3/5 | 42/44 |

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
  try both. The branch order of `if (a <= 0) ... else ...` decides which side falls through.
* A value the original keeps in a register across a test (for example a constant used by a
  later ternary's other arm) shows the source: `mov eax, 2` before a test was the `: 2`
  arm of `HLSurface::InitTextureFormats`' kind choice.
* `tools/compare.py -v` lists the offsets of the first differing bytes.
* Several classes sit in one original object: obj66 holds both FnGame and FnOption (and
  `HL3D::DrawTexels`), obj04 holds WinMain with the definitions of `g_application` and
  `g_pat_sys`.
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
* `mov eax, ecx` instead of `mov ax, cx` for a returned `unsigned short`: the value was
  already an `unsigned short` local, returned through a ternary (`Actor::AngleTo`).
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

Flags: the game uses `/O2` with its default `/Ob2`. `Object()` is inlined into
`TaskList()`, and `Remove` into `RemoveUpTo`/`RemoveFrom`, which `/Ob1` would not do.
`/G5` vs `/G6` has not been told apart by any function yet.

Tools for decompiling:
* `tools/disasm.py <addr|name|unit>`: disassembly with symbol, string and float annotations.
* `tools/fdiff.py <addr|name>`: side-by-side diff of the compiled and the original function.
* `tools/try_variants.py <src> <addr>`: applies listed source edits one at a time and reports
  each result; `tools/perm_lines.py <src> <addr>` tries every single-line move inside a block.
* `tools/permuter/`: runs [decomp-permuter](https://github.com/simonlindholm/decomp-permuter) on
  one C++ function through a C rendering that is turned back into C++ and compiled in its real
  TU for every candidate (see `tools/permuter/README.md`).

Code style: explicit braces everywhere and the repo `.clang-format` (Google, left pointer
alignment). Use structs and named fields instead of raw offsets, and recover classes with
their vtables and inheritance. Fields not recovered yet are padding arrays named after their
offset. Name decompiled functions by their MSVC-mangled names in `config/symbols.txt`;
`gensyms.py` keeps manual names.
