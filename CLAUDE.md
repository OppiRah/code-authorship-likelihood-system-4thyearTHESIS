# CALSS — Project Context

## What this is
CALSS (Code Authorship Likelihood Scoring System) — a Windows desktop tool for
professors at University of Luzon, College of Computer Studies, to detect
authorship patterns and similarity across student C-language submissions.
Fourth-year CS thesis project. Branding: maroon/gold/gray, tied to UL/CCS
identity.

**Stack:** C++17, MinGW-w64 g++ (`x86_64-w64-mingw32-g++`), Win32 + GDI+,
no external UI framework. Single static Windows executable, no third-party
runtime deps. External integrations: Google Gemini API (AI summaries),
Google Classroom API (OAuth 2.0 installed-app flow, localhost:8080 callback).

**Pipeline:** tokenization/token similarity → 14-feature stylometric
extraction → KNN classification (K=3).

---

## Current state (verified 2026-08-24)

The lineage split described in earlier revisions of this file (a file-split
effort vs. a cleanup/bugfix session, diverged and unreconciled) has been
resolved — both lines of work are present and merged on disk. Verified by
grepping for markers unique to each lineage (`getULLoadingLogo`,
`SPLASH_GOLD`, dead-code symbol names) and by a full compile:

- **Dead code removal confirmed gone**: `drawAllPairsTable`, `drawTableRow`,
  `GCSettingsDlgProc`, `GCPickerDlgProc` (+ `GCPickerData` struct),
  `createSimpleDialog`, `fmtDouble`, `toWideGC` + `toWide` macro,
  `struct MDLine` + `parseAISummaryMarkdown`, `showLegacyHelpTextModal`,
  dead globals `g_hProgressBar`, `g_hSyncProgressLbl` — zero hits except one
  comment noting the removal.
- **Named color constants** `SPLASH_GOLD` and `COLOR_SUCCESS_BG_ALT` are
  defined in `include/gui_common.h` (kept deliberately separate from
  `GOLD_500` / `COLOR_SUCCESS_BG` — still do not silently merge these).
  `SPLASH_GOLD` is used from `gui_welcome.cpp`; `COLOR_SUCCESS_BG_ALT` is
  still used from `gui.cpp`.
- **`easeOutCubic(t)`** is centralized once in `include/gui_common.h`,
  used from both `gui.cpp` and `gui_overview.cpp`.
- **Splash screen flicker/perf fixes and `getULLoadingLogo()`** landed in
  `gui_welcome.cpp` as part of the split, not left behind in `gui.cpp`.

**File split is further along than previously documented.** Five split
files now exist and are all wired into `compile.bat`, alongside
`gui.cpp` itself:

| File | Lines |
|---|---|
| `src/gui.cpp` | 6,226 |
| `src/gui_sync_sheet.cpp` (`SyncSheetProc` + `sync*` helpers/threads) | 1,009 |
| `src/gui_overview.cpp` | 676 |
| `src/gui_help_carousel.cpp` | 646 |
| `src/gui_welcome.cpp` | 505 |
| `src/gui_assign_dialog.cpp` | 348 |

`include/gui_common.h` holds the shared contract (extern globals, shared
types/constants, function prototypes) for all of the above.

`ContentProc` (~1,008 lines) is still unsplit, living in `gui.cpp` — this
matches the already-approved plan to leave it (and `SyncSheetProc`'s
`WM_PAINT`) for a dedicated future session (see Backlog).

**Open question, not resolved by this audit:** earlier revisions of this
file described an approved 9-file split boundary, naming `gui_assign_dialog.cpp`
specifically and saying `ContentProc` would eventually live in a
`gui_common.cpp` (which doesn't exist yet — only `gui_common.h` does).
`gui_help_carousel.cpp`, `gui_welcome.cpp`, and `gui_overview.cpp` are not
mentioned in that original boundary description at all, so either the plan
evolved past what was documented, or these three don't map onto the
originally-approved 9. Flagging rather than guessing — confirm with
whoever ran those checkpoints, or treat the 9-file boundary plan as stale.

**Build verification (2026-08-24):** full `x86_64-w64-mingw32-g++` syntax
check across all 14 translation units, then a full link build using
`compile.bat`'s exact file list and libs — both clean, zero errors.
`compile.bat` on disk already lists all 5 split `.cpp` files; no changes
were needed.

---

## Working discipline (non-negotiable, established over many sessions)

- **Flag over fix.** Surface decisions and ambiguities explicitly. Don't
  silently resolve something that could go two ways — ask, or note it and
  wait.
- **One category of change per checkpoint.** Don't mix e.g. dead-code
  removal with duplication-factoring in the same edit. Each is independently
  verified so a regression's cause is obvious.
- **Compile after every checkpoint — not just a balance check.** For the
  multi-file split specifically: brace/paren balance checks are NOT
  sufficient verification. The failure modes there (missing `extern`,
  duplicate definitions, type-visibility gaps) are linker errors across
  translation units, not syntax errors within one file. Reproduce the real
  MinGW build (`x86_64-w64-mingw32-g++`) and actually link.
- **Symbol ledger discipline for the split.** Each checkpoint maintains a
  verified ledger: one definition per extern symbol, byte-identical
  move verification, a behavior-change audit, structural COMDAT check.
- **Full `compile.bat` delivered every time a checkpoint adds a new
  translation unit** — not a diff, not just the new line. `compile.bat`
  uses an explicit source file list (no glob), so this must be exact and
  copy-pasteable.
- **Pre-existing bugs stay untouched** unless explicitly asked to fix them
  (see list below). Flag them if rediscovered; don't fix silently.
- **Additive wiring preferred** over replacing working code, until a
  replacement is confirmed correct.
- **Read-only audit before major changes**, report back, wait for
  authorization on what to act on and in what order.
- **Terse, output-focused communication.** Match pace — direct outputs,
  minimal narration unless asked.
- **File delivery discipline** (was Claude.ai-specific, may not apply the
  same way in Claude Code, but the underlying rule matters): never assume
  an edit is "delivered" just because a tool ran — confirm the actual file
  on disk reflects it.

---

## Known pre-existing bugs — DO NOT FIX unless explicitly asked

1. `g_studentCardRects` stores its `y` without the `+g_scrollY` convention
   every other content hit-rect vector uses → student-card clicks mis-hit
   once the grid is scrolled.
2. `g_backButtonRect` is set from a fixed reference point in `WM_PAINT`,
   but the button is drawn later at a scroll-adjusted position → they
   drift apart once the detail page is scrolled.

---

## Backlog

- **The current file structure is the plan going forward** — the original
  9-file/`gui_common.cpp`-hosts-`ContentProc` sketch is superseded, not
  something to force the codebase back toward. Five split files exist
  today (`gui_sync_sheet.cpp`, `gui_assign_dialog.cpp`,
  `gui_help_carousel.cpp`, `gui_welcome.cpp`, `gui_overview.cpp`), plus
  `gui.cpp` and the `gui_common.h` contract.
- **`gui_common.cpp` still gets created eventually, but scoped down**: it's
  for the *definitions* backing `gui_common.h`'s extern globals (67
  variables, verified 2026-08-24 — not 22 or any smaller earlier estimate)
  plus any leftover shared logic that doesn't belong to one screen. It is
  explicitly **not** a catch-all destination for `ContentProc` — confirmed
  `gui_common.h` carries no `ContentProc` prototype or extern reference of
  any kind (it's `static`, defined only in `gui.cpp:3274`), so there's
  nothing there assuming otherwise.
- **Future dedicated session (not started): split `ContentProc`
  (~1,008 lines) AND `SyncSheetProc`'s `WM_PAINT`** — both have the same
  shape (one function branching across multiple unrelated
  screens/states inline: `ContentProc` by tab, `SyncSheetProc` by
  `SHEET_CONNECT/CHOOSE/SYNCING/DONE`). Same extraction technique applies
  to both; do them together. Every other "long function" in the file was
  reviewed and found to be one cohesive job at length, not multiple jobs
  tangled together — not worth splitting.
- System requirements documentation for the thesis paper (min/recommended
  specs drafted; RAM figures need empirical verification before committing).
- Deferred, no urgency: `WM_DPICHANGED` live rescaling, full keyboard-focus
  system for painted controls, PDF/print export.

---

## Key patterns to preserve

- **Offscreen-buffer-then-`BitBlt` is the universal paint pattern.** Every
  `WM_PAINT` in this file draws to an offscreen `HDC`/`HBITMAP` first, then
  one `BitBlt` to the screen. Any direct-to-screen drawing is a bug (see
  splash-screen flicker fix above).
- **Cache per-frame GDI allocations as `static`.** Anything created inside
  a paint handler that's called repeatedly (animation loops especially)
  should be allocated once and reused, not recreated every call (see
  splash-screen perf fix above).
- **`easeOutCubic(t)`** is the shared easing helper — use it for any new
  0..1 animation progress rather than reimplementing the cubic formula.

---

## Build

```
x86_64-w64-mingw32-g++ -std=c++17 -O2 -static-libgcc -static-libstdc++ ...
-lwinhttp -lgdi32 -lcomdlg32 -lole32 -lshell32 -lcomctl32 -lws2_32 -lmsimg32 -lgdiplus
```

`compile.bat` uses an explicit file list — every new translation unit must
be added manually.

## Key files
`gui.cpp`, `gui_sync_sheet.cpp`, `gui_assign_dialog.cpp`,
`gui_help_carousel.cpp`, `gui_welcome.cpp`, `gui_overview.cpp`,
`gui_common.h`, `main.cpp`, `results_data.h`, `gemini.h`/`gemini.cpp`,
`compile.bat`.

## Assets
`ulccslogo.png` — in-app seal (sidebar). `ulcclogoloadingscreen.png` —
loading-screen-only seal (new, added this session). Both load via a
load-once/fallback-to-drawn-circle helper (`getULLogo()` /
`getULLoadingLogo()` respectively) and must ship next to the `.exe`.
