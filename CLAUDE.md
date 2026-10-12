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
| `src/gui_workspace.cpp` (Continue previous session, 1.3.0) | ~370 |

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
  per file (first load of the four-block dataset ~41 s) — known
  performance debt, deliberately not fixed in Phase 4. Thesis
  questions T-1 (LCS 500-token cap vs. full-length denominator) and T-2
  (flag rate vs. `manifest.csv` answer keys) are team decisions.

## Release readiness (Phase 4, 2026-10-02)

`authorship-cpp/docs/RELEASE_READINESS.md` is the current release record:
what ships, where the app reads/writes, known issues, thesis-methodology
notes and facts for the installer phase. Manual GUI smoke checklist:
`authorship-cpp/tests/README.md`.

## Post-release UI fixes (2026-10-04, branch `optimization-pass`)

From thesismates' feedback on the first release, one commit each:
sync sheet no longer `WS_EX_TOPMOST` (browser can cover it during
sign-in; refocused on auth done); AI settings Save/Enter tests the key
first (rejected → not saved, dialog stays open, text selected;
unreachable → second Save keeps it unverified; `gemini_testKey` has an
optional `rejected` out-param and WinHTTP timeouts); sync sheet hover
highlight + hand cursor on every painted button (`syncHitTest`,
per-state) and on tree rows (subclass + `NM_CUSTOMDRAW`); inline red
message when Google sign-in doesn't complete. Not fixed: if fetching
courses fails after sign-in, the Choose screen just shows an empty tree.

## Build

```
x86_64-w64-mingw32-g++ -std=c++17 -O2 -mwindows -static -static-libgcc -static-libstdc++ ...
-lwinhttp -lgdi32 -lcomdlg32 -lole32 -lshell32 -lcomctl32 -lws2_32 -lmsimg32 -lgdiplus -ladvapi32
```

`-static` (added 2026-10-02, Phase 4) is required: without it the exe
imports MinGW's `libwinpthread-1.dll` (pulled in by `std::thread`) and
fails to start on a PC without MinGW (`0xC0000135`). Check with
`objdump -p authorship.exe | findstr "DLL Name"` — only Windows system
DLLs and `api-ms-win-crt-*` (UCRT, built into Windows 10/11) should appear.
`-mwindows` (also Phase 4) builds a GUI-subsystem exe: before it, a
double-click opened an empty console next to CALSS (a leftover of the
removed CLI mode), and closing that console killed the app. Entry is
still `int main()`. stdout/stderr go nowhere — don't rely on them.

`compile.bat` uses an explicit file list — every new translation unit must
be added manually. (2026-10-02: added `src/gui_ai_settings.cpp`,
`src/secret_store.cpp`, and `-ladvapi32` for Credential Manager; then
`src/app_paths.cpp`, `src/analysis_pipeline.cpp`,
`src/student_report.cpp`, `src/gui_source_view.cpp` — 22 units;
1.3.0: `src/workspace_state.cpp`, `src/gui_workspace.cpp` — 24 units.)
Since Phase 5 `compile.bat` first runs `windres -I include -O coff
res/calss.rc -o calss_res.o` and links `calss_res.o` (icon, manifest,
VERSIONINFO).

## Key files
`gui.cpp`, `gui_source_view.cpp`, `gui_sync_sheet.cpp`,
`gui_assign_dialog.cpp`, `gui_help_carousel.cpp`, `gui_welcome.cpp`,
`gui_overview.cpp`, `gui_flagged.cpp`, `gui_students.cpp`,
`gui_ai_settings.cpp`, `gui_common.h`, `main.cpp`,
`analysis_pipeline.cpp`, `student_report.cpp`, `app_paths.h`,
`results_data.h`, `gemini.h`/`gemini.cpp`, `compile.bat`,
`tests/run_golden.bat`, `workspace_state.h`/`.cpp`,
`gui_workspace.cpp`, `tests/run_workspace_tests.bat`.

## Assets
`ulccslogo.png` — in-app seal (sidebar). `ulcclogoloadingscreen.png` —
loading-screen-only seal (new, added this session). Both load via a
load-once/fallback-to-drawn-circle helper (`getULLogo()` /
`getULLoadingLogo()` respectively) and must ship next to the `.exe`
(loaded from the exe's folder, not the working directory).

## Portable thesis-test package (2026-10-05)

`make_release.bat` now builds `release\CALSS\` (16 files: exe, logos,
help PNGs, OAuth client file, `README.txt`, `TESTER_CHECKLIST.txt`,
generated `BUILD_INFO.txt`), checks the file list, and zips it to
`release\CALSS_thesis-test_<date>_<commit>.zip` plus a separate
`CALSS_SampleData_SYNTHETIC.zip`. User docs live in
`authorship-cpp/release_docs/`. Evidence and what is still untested:
`docs/RELEASE_READINESS.md` §6. Cross-PC testing of this package by the
thesis mates was reported successful by the developer on 2026-10-07
(overall gate passed; individual checklist items not recorded).

## Phase 5 — installer, CALSS 1.0.0 (2026-10-07)

Record: `docs/RELEASE_READINESS.md` §7. In short:
- **Version** lives only in `include/version.h` (1.0.0 "Thesis
  Release"); About, the exe VERSIONINFO and the installer read it.
- **Resources:** `res/calss.rc` + `res/calss.manifest` (asInvoker,
  supportedOS). Common Controls v6 is deliberately OFF (themed buttons
  ignore the sidebar's WM_CTLCOLOR* colours) and DPI stays in
  `runGUI()` — see the manifest comment before changing either.
  `ulccslogo.ico` is generated by `tools/make_icon.py`.
- **Installer:** Inno Setup 6 (dev PC: `%LOCALAPPDATA%\Programs\Inno
  Setup 6`). `compile.bat` → `make_installer.bat` →
  `release\CALSS_Setup_<v>.exe` + `_SHA256.txt`. Script
  `installer/calss.iss`; never change its AppId. All users (Program
  Files, admin once) by default, "only for me" offered. Unsigned.
- **Audit log:** opened append-only (`FILE_APPEND_DATA`); the all-users
  installer grants Users `(R,AD)` on `%PROGRAMDATA%\CALSS\audit.log`.
- **Import .c files** now copies to `Documents\CALSS Imports` (was
  cwd-relative `imported_data`, broken under Program Files).
- **Uninstall** (1.0.0) removed program + shortcuts only.
- **Status:** the 1.0.0 installer passed cross-PC testing (developer
  report, 2026-10-07).

## CALSS 1.1.0 (2026-10-07)

Record: `docs/RELEASE_READINESS.md` §8.
- **Classroom sync** always downloads to `appClassroomDirU8()` =
  `%LOCALAPPDATA%\CALSS\Classroom` (no exe-folder fallback, never the
  loaded folder). The uninstaller deletes exactly that subfolder
  (`[UninstallDelete]`); everything else in `%LOCALAPPDATA%\CALSS`
  (`calss_gc_settings.json`, `gemini_model.txt`) is kept.
- **Gemini:** no hard-coded model. `gemini_checkKey()` = models.list;
  anchored ID policy (flash-lite > flash > *-latest, newest first; never
  pro/preview/tts/image); per-key session cache under a mutex; one retry
  on model-unavailable / 503; structured JSON output
  (`responseSchema` ARRAY of STRING) on both calls. Prompt wording
  untouched. AI Settings checks on a worker thread
  (`WM_AI_CHECK_DONE`) and has a model dropdown (Automatic default,
  stored in `%LOCALAPPDATA%\CALSS\gemini_model.txt`).
- Google limits the 2.5 models to accounts that used them before —
  that is why a new tester key failed on 1.0.0's `gemini-2.5-flash-lite`.
- **Status:** superseded by 1.2.0 below.

## CALSS 1.2.0 (2026-10-08)

Record: `docs/RELEASE_READINESS.md` §9. Professor feedback ("organize
the analyses by subject or class"), Overview only; the "continue where
I left off" history idea is NOT implemented.
- **Grouping = the manifest `block`** (same as the Classes tab chips):
  `PairAnalysisDisplay::blockA/B` + `AnalysisResults::flaggedGroups`
  built in `analysis_pipeline.cpp` (`readManifestBlocks`,
  `buildFlaggedGroups`). Same-block pair -> that block / "General";
  two blocks -> "Across classes". Labels only, never scoring.
- **Tier 2 Gemini** (still 1 request; 2 per run) returns
  `{overall:[1-3], groups:[{id:"G1",summary}]}`; ids assigned by CALSS,
  matched back by id; <= 10 classes and <= 50 pairs in the prompt.
- **Overview** (`gui_overview.cpp`): overall findings (old block,
  renamed `drawAIOverall`), NOTABLE CASES (top 5 flaggedPairs, only
  with 2+ classes), FLAGGED PAIRS BY CLASS (collapsible; rows reuse
  `g_aiSummaryLinkRects` -> `jumpToFlaggedPair`). Deterministic parts
  render with AI off/failed.
- **Status:** superseded by 1.3.0 below.

## CALSS 1.3.0 (2026-10-12)

Record: `docs/RELEASE_READINESS.md` §10. Two changes, no methodology
change (golden byte-identical):
- **Continue previous session** (professor: "continue where I left
  off"; team scope: the ONE most recent session, no history list).
  `%LOCALAPPDATA%\CALSS\workspace.json` (`workspace_state.h/.cpp`,
  strict bounded parser, atomic `.tmp`+`MoveFileExW` save, format
  version 1) holds only the folder path, file count and navigation BY
  NAME (page; Classes block chip + student; last opened flagged pair's
  two file names). No scores/results/code/keys. `gui_workspace.cpp`:
  empty-state card (Continue / Start new); Continue checks the folder,
  loads it and runs the normal analysis via `startAnalysisRun(useAI =
  false)` (never Gemini, never Classroom/OAuth), then
  `workspaceOnAnalysisComplete` re-applies navigation by name and lists
  anything missing ("Session partly restored"). Missing folder /
  missing Classroom download -> modal with Select folder… / Sync
  Classroom. Saved by `workspaceSaveIfChanged()` after each content
  paint + on exit, only when changed and only for a finished valid run
  of the loaded folder. Unit tests: `tests\run_workspace_tests.bat`.
- **Flagged Pairs:** the maroon "Comparative Authorship Analysis" box
  and its Copy findings button are removed (and the now-unreachable
  copy-findings code). AI Evidence Note kept;
  `PairAnalysisDisplay::interpretation` still computed, not shown.
- Uninstall keeps `workspace.json` (settings policy); the uninstall
  message now lists `%LOCALAPPDATA%\CALSS` as kept.
- Known, not changed: `g_expandedPairs` isn't cleared between runs.
- **Status:** tested and accepted by the developer on their own PC
  (2026-10-12); superseded by 1.3.1 below.

## CALSS 1.3.1 — laboratory deployment (2026-10-12) — DEVELOPMENT FROZEN

Record: `docs/RELEASE_READINESS.md` §11. Final release before the
defense; after this, only fixes the team explicitly asks for.
- Lab head administrator approved installing CALSS on the
  instructor/admin PC of each approved programming lab (team report).
- Overview "Flagged pairs by class" sections all start collapsed
  (`overviewGroupOpen` default `false`).
- Installer: Inno's built-in `InfoBeforeFile` page "Before You Install
  CALSS" (`installer/before_you_install.rtf`, `[Messages]` title).
  Nothing else in `calss.iss` changed.
- Package: `compile.bat` → `make_installer.bat` →
  `make_lab_package.bat` (`tools/make_lab_package.py`, needs reportlab;
  sources in `release_docs/lab/`) → `release\CALSS_1.3.1_Lab_Deployment\`
  + `.zip`: installer, `SHA256SUMS.txt` (written last),
  `INSTALLATION_GUIDE.pdf`, `README.txt`, `RELEASE_NOTES.txt`,
  synthetic sample ZIP.
- `CALSS_Setup_1.3.1.exe` 3,736,846 bytes, SHA-256
  `947C8E2CA0A357117D756A44538798C1A7DCE12C4760879FAEBC0B001BA0BABA`;
  unsigned; Defender: no threats. 1.0.0–1.3.0 installers kept.
- Remaining checks are the administrator's (guide §17): all-users
  install with UAC, SmartScreen handling, real lab PC.

GUI test driving (Claude Code): launch the exe, post WM_LBUTTONDOWN/UP
and WM_KEYDOWN to the `AuthorshipContent` child, capture with
`PrintWindow`, and make the driving process DPI-aware (this PC is 125%:
content client coords = screenshot − (375, 50)). Never post Run
analysis (5004) without first unticking "Include AI summary" — the dev
PC has a real Gemini key. Uninstalling deletes
`%LOCALAPPDATA%\CALSS\Classroom`, which may hold the developer's real
synced submissions: back it up first.

Claude Code note: in this environment `cmd /c compile.bat` fails with
"not recognized"; run batch files by full quoted path
(`cmd /c "\"<full path>\make_release.bat\""`) or run `compile.bat`'s
g++ line directly.
