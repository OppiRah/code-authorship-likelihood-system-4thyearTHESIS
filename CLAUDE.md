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

## Current state (verified 2026-08-25)

**The ContentProc/SyncSheetProc split initiative is complete.** Both of
the app's two "one function branching across multiple unrelated
screens/states inline" functions — `ContentProc` (by tab) and
`SyncSheetProc` (by `SHEET_CONNECT/CHOOSE/SYNCING/DONE`) — have had their
per-branch bodies extracted into named helper functions, so each proc
itself is now a thin dispatcher (`if (state == X) helperForX(...);` ...)
rather than a single long function mixing multiple screens' logic. This
was the last item in the file-split Backlog; nothing further is planned
here (see Backlog below — now empty of split work).

Four checkpoints got there:

- **Checkpoint 1** (`2af31e1`): extracted `ContentProc`'s Overview branch
  into `gui_overview.cpp` (`drawStatsDashboard`/`drawAISummary`/
  `handleOverviewClick`, etc.).
- **Checkpoint 2** (`f70740a`): extracted the Flagged Pairs branch into
  `gui_flagged.cpp` (`drawFlaggedPairs`, `handleFlaggedPairsClick`, the
  expand/collapse and copy-findings state).
- **Checkpoint 3** (`3219494`): extracted the Students branch into
  `gui_students.cpp` (block sub-tab bar, student card grid, per-student
  detail page with style DNA strip and flagged-appearance deep-links).
- **SyncSheetProc split** (`4d0143d`): approach (b), in-file extraction
  only (no new file, no `compile.bat` change) — `SyncSheetProc`'s
  `WM_PAINT` 4-way chain became `drawSyncConnectState`/
  `drawSyncChooseState`/`drawSyncSyncingState`/`drawSyncDoneState`, and
  its `WM_LBUTTONDOWN` 4-way chain became `handleSyncConnectClick`/
  `handleSyncChooseClick`/`handleSyncSyncingClick`/`handleSyncDoneClick`,
  all still living in `gui_sync_sheet.cpp`.

Each checkpoint was verified the same way: full
`x86_64-w64-mingw32-g++` link build against `compile.bat`'s exact file
list (clean, zero errors, every time); one definition per moved/extracted
symbol (none left behind in the file it was extracted from); byte/MD5
diff of every extracted function body against the committed original
(all identical, modulo the mechanical dedent and — once — an added
parameter the build caught, see below).

The GUI is now `gui.cpp`, nine split files and the `gui_common.h`
contract, all wired into `compile.bat` (line counts as of 2026-10-02,
after the optimization pass — see that section below):

| File | Lines |
|---|---|
| `src/gui.cpp` | 4,704 |
| `src/gui_flagged.cpp` | 1,341 |
| `src/gui_students.cpp` | 1,306 |
| `src/gui_source_view.cpp` (View source modal, split 2026-10-02) | 1,091 |
| `src/gui_sync_sheet.cpp` (`SyncSheetProc` + `sync*` helpers/threads) | 1,087 |
| `src/gui_overview.cpp` | 1,019 |
| `src/gui_help_carousel.cpp` | 644 |
| `src/gui_welcome.cpp` | ~500 |
| `src/gui_assign_dialog.cpp` | ~350 |
| `src/gui_ai_settings.cpp` | ~300 |

`include/gui_common.h` (~820 lines) holds the shared contract (extern
globals, shared types/constants, function prototypes) for all of the
above. `ContentProc` and `SyncSheetProc` themselves remain in `gui.cpp`
and `gui_sync_sheet.cpp` respectively — only their per-branch bodies
moved; the dispatchers are the intended end state, not a leftover.

**One real bug the split work caught, worth remembering as a pattern:**
extracting `SyncSheetProc`'s `SHEET_CHOOSE` paint branch into
`drawSyncChooseState` initially compiled clean at the syntax level but
failed to link/compile because the extracted body called
`childRectInParent(g_hSyncTree, hwnd)` — `hwnd` was implicitly in scope
in the original nested block (it's `SyncSheetProc`'s own parameter) and
the mechanical extraction missed that it needed to become an explicit
parameter. Fixed by adding `HWND hwnd` to `drawSyncChooseState`'s
signature and its one call site. The general lesson: when extracting an
inline branch into a standalone function, grep the extracted body for
every identifier and confirm each one is either a parameter, a global,
or genuinely file-local — don't assume the enclosing function's own
parameters transfer for free.

**No `gui_common.cpp`, and none is coming (decided 2026-08-24, still
final).** Earlier revisions of this file described an approved 9-file
split boundary naming `gui_assign_dialog.cpp` specifically and saying
`ContentProc` would eventually live in a `gui_common.cpp`. That plan is
superseded, not pending — `gui_common.cpp` consolidation is skipped.
Shared globals stay defined in whichever `.cpp` file already owns that
subsystem (`gui.cpp` for the majority, or the relevant split file) rather
than being centralized into one new file. `gui_common.h` itself documents
this breakdown directly (see its top-of-file note and the `SHARED
GLOBALS` section header).

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

None open. The two listed here before were fixed on 2026-10-01 at the
user's request (the Classes roster rewrite depended on #1):
1. ~~`g_studentCardRects` y without `+g_scrollY`~~ — rects now
   content-space like every other hit rect.
2. ~~`g_backButtonRect` set from a fixed point in `drawStudentsPage`~~ —
   now set from the actually-drawn back link inside
   `drawStudentDetailPage`.

Still flagged, not fixed: Overview "High similarity" (token label "High
Similarity", all pairs) and the Flagged tab's "High" filter (flagged,
combined ≥85%) count different things. (The working-directory bug once
listed here — file dialogs changing the cwd and files opened by
relative path — was fixed 2026-10-02, D-2.)

## UI pass 2026-10-01 (uncommitted at time of writing)

Overview redesigned as a "report sheet" (headline + stat row + risk
bar, no cards); AI summary "Pair N" links (prompt now requires
"Pair N", plus a "NameA vs NameB" fallback) that jump to and auto-scroll
to the pair (`g_pendingScrollPairIdx`). Classes grid is now roster rows.
Shared dropdown component in `gui.cpp` (Classes sort, Flagged filter).
Search box + Export live only in the Flagged Pairs fixed toolbar
(`FLAGGED_TOOLBAR_HEIGHT`); search filters pairs. Sidebar is
drag-resizable (240–400 design px, `g_sidebarW`). Minimum fonts raised
(label/mono-small 14, mono 15). Content/sidebar repaint fully on resize.

---

## Backlog

- **The current file structure is the plan going forward** — the original
  9-file/`gui_common.cpp`-hosts-`ContentProc` sketch is superseded, not
  something to force the codebase back toward. Nine split files exist
  today (`gui_sync_sheet.cpp`, `gui_assign_dialog.cpp`,
  `gui_help_carousel.cpp`, `gui_welcome.cpp`, `gui_overview.cpp`,
  `gui_flagged.cpp`, `gui_students.cpp`, `gui_ai_settings.cpp`,
  `gui_source_view.cpp`), plus `gui.cpp` and the
  `gui_common.h` contract. Decided 2026-08-24: `gui_common.cpp`
  consolidation of the shared globals is skipped — they stay defined
  across `gui.cpp` and whichever split file already owns that subsystem
  (see Current State above for the exact breakdown).
- **The ContentProc/SyncSheetProc split is done** (see Current State
  above) — both procs are now thin dispatchers over per-branch helper
  functions. Every other "long function" in the file was reviewed
  earlier and found to be one cohesive job at length, not multiple jobs
  tangled together — not worth splitting further.
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

## Secrets (2026-10-02)

The Gemini API key and the Google Classroom OAuth token are stored in
**Windows Credential Manager** (per Windows user, this PC only) via
`secret_store.h/.cpp` — targets `CALSS/GeminiApiKey` and
`CALSS/GoogleClassroomToken`. No plaintext `api_key.txt` /
`calss_gc_token.json` anymore; on first launch the app imports any such
file, verifies the stored copy, deletes the file and shows a notice
(`migrateLegacySecretsEarly()` runs before any window is created — the
sidebar's `aiKeyConfigured()` check would otherwise trigger the silent
CLI-path import first). Each installation enters its own Gemini key via
**File > AI Settings…** (`gui_ai_settings.cpp`, with a live "Test key");
no key → "Include AI summary" unticked + disabled and "Set up AI…" links
in the sidebar, Overview and per-pair AI notes. The Gemini key is sent
in the `x-goog-api-key` header, not the URL.
`calss_gc_embedded_credentials.json` (the app's OAuth client ID, not a
personal secret) still ships as a file. `make_release.bat` builds
`release/CALSS/` with only the exe + required assets.

Known, not changed: the sync sheet goes straight to browser sign-in when
the saved access token is expired (it doesn't try the refresh token
first).

## Batch report.html + CLI mode removed (2026-10-02)

At the group's request, the batch `report.html` is gone: `htmlReport()`
(and its `scoreColor`/`simColor` helpers) in `main.cpp`, View > "Open
Latest Report in Browser" (`ID_VIEW_REPORT`, `onViewReport`), the
post-analysis auto-open, `AppState::lastReport`,
`AnalysisResults::reportPath`, and the report file/hash lines in the
"ANALYSIS COMPLETE" audit entry. `runAnalysisPipeline(dataFolder, useAI)`
no longer takes an output path. CLI mode is also removed: `main()` only
launches the GUI. Still kept: the per-student "Generate Similarity
Report" button (`similarity_report_<name>.html`). (The automatic
`generateStudentReports` was removed too in the optimization pass —
it never produced output.)

## Optimization pass (2026-10-02, branch `optimization-pass`)

Implemented from `docs/CODEBASE_OPTIMIZATION_ASSESSMENT.md`, one
checkpoint per commit. Thesis results unchanged: every checkpoint passed
the golden check byte-identically.

- **Regression check — run before committing anything in `src/`:**
  `authorship-cpp\tests\run_golden.bat` runs the real pipeline (AI off)
  on the committed synthetic datasets (`stress_test/`,
  `stress_test_bscs7/`) and compares SHA-256 hashes of every score
  against `tests/golden/expected/`. `--update` only for a change that is
  *meant* to alter results. Manual GUI smoke checklist in
  `tests/README.md`. `.gitattributes` keeps those files byte-exact.
- **Structure:** `main.cpp` is just `main()`; the pipeline is in
  `analysis_pipeline.cpp`, the per-student report in
  `student_report.cpp`; the View source modal in `gui_source_view.cpp`.
- **Paths (`app_paths.h`):** every `std::string` path is **UTF-8**;
  open files with `pathFromUtf8()`, call wide (`*W`) Win32 APIs with
  `widenUtf8()`. Assets load from the exe's folder; Classroom settings go
  to `%LOCALAPPDATA%\CALSS`; similarity reports to
  `Documents\CALSS Reports`. Non-ASCII folders/files (e.g. "Peña") now
  load.
- **AI:** only Tier 1/Tier 2 calls remain (legacy summary removed), on the
  50 highest-scoring flagged pairs (`GEMINI_MAX_PAIRS`); explicit WinHTTP
  timeouts.
- **Security/correctness:** OAuth listener bound to 127.0.0.1 with a real
  3-minute timeout and a checked `state`; `progressMsg` race fixed; CSV
  export uses the pipeline's student-name parser.
- **Performance:** each compared file is tokenized/feature-extracted once
  per run (four-block dataset ~28 s → ~7.4 s; one section ~1.5 s →
  ~0.55 s).
- **Open, not done:** `autoTagManualBlock` re-reads the whole manifest
  per file (first load of the four-block dataset ~41 s); View > "View
  Student Profiles" still shows a stale placeholder message. Thesis
  questions T-1 (LCS 500-token cap vs. full-length denominator) and T-2
  (flag rate vs. `manifest.csv` answer keys) are team decisions.

## Build

```
x86_64-w64-mingw32-g++ -std=c++17 -O2 -static-libgcc -static-libstdc++ ...
-lwinhttp -lgdi32 -lcomdlg32 -lole32 -lshell32 -lcomctl32 -lws2_32 -lmsimg32 -lgdiplus -ladvapi32
```

`compile.bat` uses an explicit file list — every new translation unit must
be added manually. (2026-10-02: added `src/gui_ai_settings.cpp`,
`src/secret_store.cpp`, and `-ladvapi32` for Credential Manager; then
`src/app_paths.cpp`, `src/analysis_pipeline.cpp`,
`src/student_report.cpp`, `src/gui_source_view.cpp` — 22 units.)

## Key files
`gui.cpp`, `gui_source_view.cpp`, `gui_sync_sheet.cpp`,
`gui_assign_dialog.cpp`, `gui_help_carousel.cpp`, `gui_welcome.cpp`,
`gui_overview.cpp`, `gui_flagged.cpp`, `gui_students.cpp`,
`gui_ai_settings.cpp`, `gui_common.h`, `main.cpp`,
`analysis_pipeline.cpp`, `student_report.cpp`, `app_paths.h`,
`results_data.h`, `gemini.h`/`gemini.cpp`, `compile.bat`,
`tests/run_golden.bat`.

## Assets
`ulccslogo.png` — in-app seal (sidebar). `ulcclogoloadingscreen.png` —
loading-screen-only seal (new, added this session). Both load via a
load-once/fallback-to-drawn-circle helper (`getULLogo()` /
`getULLoadingLogo()` respectively) and must ship next to the `.exe`
(loaded from the exe's folder, not the working directory).
