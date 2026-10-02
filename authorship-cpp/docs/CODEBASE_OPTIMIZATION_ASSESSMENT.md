# CALSS Codebase Optimization Assessment — 2026-10-02

Read-only assessment of the working tree as of 2026-10-02 (branch `master`,
HEAD `2e94431`, plus a large uncommitted working tree — see §8). The brief
is `docs/CodebaseOptimizationAssessment.md`. No source, header, build
script or configuration file was modified; this document is the only file
added.

Evidence labels used below:

- **Verified (read)** — checked by reading the cited code.
- **Verified (run)** — checked by compiling/running the real code. The
  runtime measurements in §5 come from a throwaway harness built in a
  scratch directory outside the repository that links the repo's own
  object files unchanged and calls `runAnalysisPipeline()` with AI off,
  on copies of `../stress_test_bscs7/Programming1` and `../stress_test`.
- **Inferred** — reasoned from code, not exercised.
- **Not verified** — called out explicitly.

Earlier documents: `docs/ARCHITECTURE.md` (file-by-file reference) and
`docs/CLAUDE_CODE_AUDIT.md` (2026-09-17 audit of formulas/internals) are
still largely accurate for the analysis core; this assessment does not
repeat their formula descriptions. Where they are now stale, §6 says so.

---

## 1. Executive Summary

**Does CALSS need optimization?** Not broadly. The analysis core is small,
deterministic and fast enough for its realistic workload: one section
(553 files, 2,186 pairs) analyzes in **~1.6–2.0 s** on this PC. Paint
performance was profiled earlier at ~2–4 ms/frame. There is no general
performance problem to solve.

**What kind of work it actually needs, in priority order:**

1. **Correctness, not optimization.** The highest-value findings are bugs
   and latent defects, not speed:
   - `generateStudentReports()` looks up profiles by *file key* instead of
     *student name*, so it never writes anything (verified by run: 39 pairs
     ≥90%, zero reports) — it silently burns work every run.
   - Folder/file paths are converted to UTF-8 and then passed to ANSI
     (`*A`) Win32 and narrow `ifstream` APIs, so any path containing a
     non-ASCII character (e.g. **ñ** in a student or Windows user name)
     will fail to load.
   - The OAuth callback listener binds to all network interfaces, has no
     effective timeout, and has no `state`/PKCE.
   - A cross-thread data race on `g_state.progressMsg`.
   - The legacy Gemini summary call still runs every AI-enabled analysis
     but its output is no longer displayed anywhere (its only consumer was
     the batch HTML report removed today) — one wasted API call per run.
2. **Regression protection.** There are **no automated tests** and, since
   CLI mode was removed today, no headless entry point. Before any
   refactor, a golden-output harness is needed (the run showed results are
   bit-for-bit deterministic, so this is straightforward).
3. **Targeted cleanup.** ~250 lines of compiler-confirmed dead GUI code, a
   few duplicated helpers (one of which causes a real CSV inconsistency),
   stale scripts/docs.
4. **Two structural moves with concrete reasons** (not line count):
   pull the analysis pipeline out of `main.cpp` (so tests can link it),
   and extract the self-contained "View source" modal (~1,000 lines) out of
   `gui.cpp`.

**Thesis-critical items to decide as a team, not to "optimize":** the LCS
500-token cap divides by the *untruncated* lengths (identical files over
500 tokens cannot score as duplicates), and the 75% flag threshold flags
32–64% of all pairs on the synthetic datasets that ship with answer keys.
Neither is triggered by a code defect in the usual sense; both affect what
the thesis can claim. See §7.

**What should NOT be optimized:** the scoring formulas, tokenizer,
feature extraction, KNN, thresholds, keyword lists, filename/identity
parsing, the Gemini prompt wording, and the existing GUI file split. See §10.

---

## 2. Current Architecture

### 2.1 Layers and data flow (Verified, read)

```
                 ┌──────────────────────────── GUI thread ───────────────────────────┐
 main.cpp:main() │ runGUI() (gui.cpp)                                                  │
   └────────────►│  WindowProc ─ TitleBarProc ─ SidebarProc ─ ContentProc (dispatcher) │
                 │     │            screens: gui_welcome / gui_overview / gui_flagged / │
                 │     │                     gui_students; modals: source view, help    │
                 │     │                     carousel, sync sheet, assign, AI settings  │
                 │     ▼ onRunAnalysis → std::thread(analysisThread).detach()           │
                 └─────┼────────────────────────────────────────────────────────────────┘
                       ▼ worker thread
   runAnalysisPipeline(folder, useAI)            (main.cpp)
     loadFolder → checkLanguage → AuthorshipScorer::loadSubmission (profiles)
       → group exam files by extractPeriod → analyzePair for every cross-student
         pair (similarity.cpp → tokenizer.cpp + features.cpp)
       → summaryStats / flaggedPairs → scorer.scorePair per flagged pair (knn_model.cpp)
       → build display structs into global g_analysisResults (results_data.h)
       → optional Gemini calls (gemini.cpp) → generateStudentReports
     PostMessage(WM_ANALYSIS_COMPLETE) → GUI reads g_analysisResults
```

### 2.2 Modules

| Module | Files | Responsibility | Depends on |
|---|---|---|---|
| Tokenization / token similarity | `tokenizer.cpp/.h` (386) | clean source, tokens, normalization, Jaccard/LCS, per-line normalization for View source | — |
| Stylometry | `features.cpp/.h` (619) | 14 features, normalization, `describeStyle`, `compareStyles`, per-function style (display only) | — |
| Pair scoring | `similarity.cpp/.h` (137) | `analyzePair` (60/40 token/style), flag ≥75, stats | tokenizer, features |
| Authorship | `knn_model.cpp/.h` (193) | per-student profile, KNN (K=3) score, confidence | features |
| Pipeline + file identity | `main.cpp` (1,843) | load folder, name/period/category parsing, language check, pipeline orchestration, two per-student HTML generators, `main()` | all analysis modules, gemini, results_data |
| Display model | `results_data.h` (176) | GUI-facing structs, `g_analysisResults` | — |
| AI | `gemini.cpp/.h` (820) | 3 prompt builders, WinHTTP POST, hand-rolled JSON | knn_model, results_data, secret_store |
| Classroom | `google_classroom.cpp/.h` (991) | OAuth installed-app flow, Classroom REST, download + manifest | secret_store |
| Secrets | `secret_store.cpp/.h` (161) | Windows Credential Manager | advapi32 |
| Audit | `audit_log.cpp/.h` (170) | append-only log + SHA-256 | — |
| GUI | `gui*.cpp` + `gui_common.h` (~13k) | see §3 | everything above (one direction) |

**Dependency direction is clean** (Verified, read): no analysis module
includes a GUI header. The only upward coupling is that the pipeline
writes the global `g_analysisResults` and calls an optional progress
callback, which is a reasonable pattern for this size of app. There are
no circular dependencies between translation units.

### 2.3 Shared state

- `g_analysisResults` (defined in `main.cpp`) — written only by the worker
  thread during a run; the GUI reads it only when `g_state.analysisComplete`
  is true, and that flag is set only on the GUI thread after
  `WM_ANALYSIS_COMPLETE`. This invariant is what makes the global safe; it
  is not documented anywhere and should be (§9 P1).
- `g_state` (`AppState`, gui.cpp) — GUI-thread state **except**
  `progressMsg`, which the worker also writes (§6.3, data race).
- 79 `extern` GUI globals in `gui_common.h` (the header comment says 73 —
  stale again).

---

## 3. GUI Architecture Assessment

**Verdict: justified, with one remaining pocket of mixed responsibility
inside `gui.cpp`.**

Each split file owns one screen or one modal, keeps most of its state
`static`, and exports a narrow API (Verified, read):

| File | Lines | Owns | Exported surface | Assessment |
|---|---:|---|---|---|
| `gui_overview.cpp` | 1,012 | Overview "report sheet", AI batch-pattern box | 5 functions (`drawStatsDashboard`, `drawAISummary`, `drawResultsHeader`, `handleOverviewClick`, `readableAIError`) | Cohesive |
| `gui_flagged.cpp` | 1,834 | Flagged Pairs list, expand/collapse, side-by-side, pair narratives | 3 functions + drawing entry points | Cohesive; contains ~250 lines of dead code (§6.5) |
| `gui_students.cpp` | 1,308 | Classes roster, per-run cache, student detail | 9 functions | Cohesive |
| `gui_sync_sheet.cpp` | 1,086 | Classroom sync modal (4 states) + its threads | 3 functions | Cohesive |
| `gui_welcome.cpp` | 503 | idle screen, splash, analysis thread + progress | 3 functions | Cohesive (the analysis thread living here is slightly odd but harmless) |
| `gui_help_carousel.cpp` | 644 | help overlay | 7 functions | Cohesive |
| `gui_assign_dialog.cpp` | 348 | unclassified-file dialog | 1 function | Cohesive |
| `gui_ai_settings.cpp` | 307 | AI key dialog + legacy secret migration | 4 functions | Cohesive |

These boundaries follow real responsibilities (one screen/modal each), not
line counts. **Do not merge them** — merging would recreate the
5,000-line single-file problem the team already paid to solve. **Do not
convert the 79 globals into a state object** — it would touch every
file for little practical gain in a single-window Win32 app (DO NOT
RECOMMEND).

Business logic in GUI files is minimal and display-only: the Students
cache groups files via `main.cpp`'s exported `studentNameForFile()`
(correct); the source-view "style change" highlight computes its own
statistics but is explicitly never fed back into scoring (Verified, read:
comment and data flow agree). One genuine leak of business logic is
`extractStudentNameGC()` in `gui.cpp`, a partial re-implementation of
`main.cpp`'s student-name parser, used by CSV export (§6.1).

The remaining issue is `gui.cpp` itself (§4).

---

## 4. Large File Analysis

| File | Lines | Main responsibilities | Cohesion | Refactor needed? | Risk |
|---|---:|---|---|---|---|
| `src/gui.cpp` | 5,780 | app shell (WindowProc, custom title bar, sidebar, layout, menus, fonts), `ContentProc` dispatcher + input routing (keyboard focus, scroll, mouse), shared widgets (toasts, app modal, dropdown, search box, eyebrow, DNA strip, focus ring), **View source modal** (~1,000 lines: line-matching LCS, style-change highlight, geometry, eased scroll, legend, drawing), Flagged-tab toolbar/CSV export/copy findings (~260 lines) | Mixed: shell + shared widgets are cohesive "app frame" code; the View source modal is a separate subsystem with its own state and algorithm | **Partially** — extract the View source modal only | LOW (same mechanical procedure as Checkpoints 1–3) |
| `src/main.cpp` | 1,843 | entry point; filename identity/category/period parsing; language check; folder loading; pipeline orchestration; two per-student HTML report generators (~900 lines, mostly HTML/CSS literals) | Low — three unrelated jobs | **Yes** — see below | LOW (mechanical move) |
| `src/gui_flagged.cpp` | 1,834 | Flagged Pairs tab | High | No (just delete dead code) | SAFE |
| `src/gui_students.cpp` | 1,308 | Classes tab | High | No | — |
| `src/gui_sync_sheet.cpp` | 1,086 | sync modal | High (already split into per-state helpers) | No | — |
| `src/gui_overview.cpp` | 1,012 | Overview | High | No | — |
| `src/google_classroom.cpp` | 991 | OAuth + REST + download | High | No (fix security items in place) | — |
| `src/gemini.cpp` | 820 | AI prompts + HTTP | High | No (remove legacy path once decided) | — |

**`main.cpp` — proposed boundaries** (concrete reasons: (a) a regression
harness cannot link the pipeline without also linking `main()`; (b) HTML
templating has nothing to do with analysis):

```
Current:                    Proposed:
main.cpp                    main.cpp                ~10 lines: main() → runGUI()
                            analysis_pipeline.cpp   loadFolder, extractStudentName,
                                                    categorizeFile, extractPeriod,
                                                    checkLanguage, runAnalysisPipeline,
                                                    g_analysisResults
                            student_report.cpp      generateStudentSimilarityReport
                                                    (+ generateStudentReports if kept)
```

**`gui.cpp` — proposed boundary:**

```
gui.cpp              shell, ContentProc, shared widgets (unchanged)
gui_source_view.cpp  View source modal: SourceViewSide state,
                     computeSourceLineMatches, style-change highlight,
                     geometry/scroll/legend/draw (~1,000 lines)
```

The Flagged toolbar/CSV/copy-findings block could follow into
`gui_flagged.cpp`, but it shares hit-rect state with `ContentProc`; the
benefit is small — optional.

Splitting the rest of `gui.cpp` (sidebar, title bar, dropdown) is **DO NOT
RECOMMEND**: each is a few hundred lines tightly bound to the main window,
and moving them would mostly add extern globals.

---

## 5. Performance Findings

### 5.1 Measured baseline (Verified, run — this PC, `-O2`, AI off)

| Dataset | Files | Pairs | Flagged | Profiles | Time (2 runs) | Checksum of all combined scores |
|---|---:|---:|---:|---:|---|---|
| `stress_test_bscs7/Programming1` (1 section) | 553 | 2,186 | 705 | 35 | 1,595 / 2,043 ms | 152602.702558 (identical both runs) |
| `stress_test` (4 sections loaded together) | 2,973 | 43,816 | 27,991 | 150 | 47,154 / 57,576 ms | 3661065.384703 (identical both runs) |

Per-operation cost (same PC, 2,973 files, 20,000 sampled pairs):

| Operation | Cost |
|---|---|
| `tokenize()` (includes file read) | 0.103 ms / file |
| `extractFeatures()` (includes file read) | 0.079 ms / file |
| `computeSimilarity()` (Jaccard + LCS) | 0.043 ms / pair |
| `styleSimilarity()` | 0.00002 ms / pair |

### 5.2 Findings

**PF-1. `analyzePair` re-reads, re-tokenizes and re-extracts both files
for every pair.** `similarity.cpp:33–77`. Each file is processed
2·(n−1) times per period group instead of once. From the numbers above,
~0.36 ms of the ~0.41 ms per pair is this redundant file work — about
16 s of the 47–58 s four-section run. For one section it is ~0.8 s.
*Impact:* noticeable only when several sections are loaded at once.
*Action:* memoize `TokenResult`/`FeatureResult` per path for the
duration of one run (pure functions of file content, so outputs are
identical; confirm with the golden diff). *Risk:* LOW. *Priority:* only if
multi-section loads are a real use case; otherwise optional.

**PF-2. Unattributed remainder of the four-section run (~30 s).** Not
profiled further. Likely candidates (Inferred): `scorer.scorePair` for
27,991 flagged pairs (re-extracts features of both files and runs
`compareStyles` → `parseStyleNote` string parsing per feature),
display-struct construction, and allocation churn. *Action:* profile
before changing anything. *Risk:* n/a.

**PF-3. Legacy Gemini call whose result is discarded.** `main.cpp`
pipeline: `generateAISummary()` still runs whenever AI is on, and it is
the only call wrapped in the "Generating AI summary" progress phase, but
`aiSummaryText` is read nowhere since today's report removal (Verified,
grep). Costs one network round trip and API quota per run. *Action:*
remove the call and wrap the two tiered calls in the progress phase
instead. *Risk:* LOW (AI text shown to the user is unchanged; progress
timing changes). Also a correctness/cost item.

**PF-4. `generateStudentReports()` runs every analysis and produces
nothing** (see §6.3). It re-scores both students for every ≥90% pair
before failing silently. Cheap today, but pure waste.

**PF-5. LCS on `std::string` tokens.** 500×500 string comparisons per
pair. Interning tokens to integers would make it several times faster
with identical results. *Impact:* ~0.043 ms/pair is already small.
*Risk:* LOW technically, but it touches `tokenizer.cpp`, a thesis-critical
file. **DO NOT RECOMMEND** unless PF-1 proves insufficient.

**PF-6. Parallelizing the pair loop.** Would help multi-section runs but
adds threading to a deterministic pipeline (result ordering, progress,
cancellation). **DO NOT RECOMMEND** for a thesis build.

**PF-7. AI prompt size scales with flagged-pair count.** All flagged pairs
go into one prompt; the four-section dataset produced 27,991 flagged
pairs. No WinHTTP timeouts are set (defaults: 30 s receive), and Tier 1
fails all pairs if the returned array length mismatches. *Impact:* AI
summaries will fail on large runs. *Action:* cap the number of pairs sent
(e.g. top N by score) and set explicit timeouts. *Risk:* MODERATE (changes
AI content).

**GUI rendering:** earlier profiling (`profile.log`) shows 2–4 ms/frame;
the Classes tab already has a per-run cache and visibility culling.
Per-paint GDI brush/pen creation is balanced (create/delete counts match
in every paint path) and cheap. **No action.** *Not verified:* scrolling
the Flagged tab with tens of thousands of flagged pairs.

---

## 6. Code Quality Findings

### 6.1 Duplication

| Duplicate | Where | Matters? | Recommendation |
|---|---|---|---|
| Student-name parsing | `main.cpp:extractStudentName` vs `gui.cpp:extractStudentNameGC` (partial copy: no legacy `act1`/`finalsexam` prefix or `_N` suffix stripping) | **Yes** — CSV export uses the partial copy, so for files like `finalsexamCarlos.c` the CSV says `finalsexamCarlos` while the GUI says `Carlos` | Use `studentNameForFile()` in CSV export, delete the copy. LOW RISK |
| Period parsing | `main.cpp:extractPeriod` vs `gui.cpp:extractPeriodGC` (byte-identical logic) | Drift risk only | Export one, delete the other. SAFE |
| UTF-8 ⇄ UTF-16 converters | `wideToUtf8` ≡ `toNarrowGC` (gui.cpp); `s2w` ≡ `utf8ToWide` (gui.cpp / gui_welcome.cpp) | Minor | Keep one of each in `gui_common.h`. SAFE |
| `readFile` | `tokenizer.cpp`, `features.cpp` | No | Leave |
| C keyword lists | `tokenizer.cpp:C_KEYWORDS` vs `features.cpp:C_KW` (differ: tokenizer includes `include`, `define`) | Thesis-sensitive | **DO NOT RECOMMEND** merging — it would change feature values |
| WinHTTP request code | 2× in `gemini.cpp`, GET/POST in `google_classroom.cpp` | Moderate | Optional Phase 4; touches two external integrations for little gain |
| Two per-student HTML generators | `generateStudentReports` vs `generateStudentSimilarityReport` (shared CSS/structure) | Moot if the first is removed | Decide §9 P1-4 first |

### 6.2 Dead / obsolete code (Verified: compiler `-Wunused` and grep)

- `gui_flagged.cpp`: `drawAuthorshipCard`, `estimateAuthorshipCardHeight`
  (~190 lines, lines ~405–593), `estimatePairNarrativeHeight`; unused local
  `startY`.
- `gui.cpp`: `g_exportMenuOpen`. `gui_students.cpp`: unused local `hovered`.
- `google_classroom.cpp`: `nameFromEmail`, `toNarrow`.
- `similarity.cpp`: `analyzeAll` (and its prototype) — already flagged
  in the 2026-09-17 audit.
- `tokenizer.cpp`: commented-out self-test `main`.
- `gemini.cpp`: `generateAISummary`/`buildPrompt` become dead once PF-3
  is acted on; `results_data.h` `aiSummary*` fields are already write-only.
- View > "View Student Profiles": a stub toast pointing users to the HTML
  report that no longer exists.
- `run.bat`: still runs `authorship.exe data` (CLI mode, removed today) —
  now just opens the GUI and then waits on `pause`.
- Repo-root clutter: `features.o`, `knn_model.o`, `similarity.o`,
  `authorship_fixD.exe`, `backup include + src/`, `*.log`, old
  `similarity_report_*.html` (all gitignored; harmless, but confusing).

### 6.3 Correctness and resource safety

Confirmed problems:

- **C-1 `generateStudentReports()` never produces output.** `main.cpp`
  ~line 560: `scorer.score(pair.studentA, pair.pathA)` passes the
  file key (`midtermexam__CastroBryan_ASGNa7c46f`) where the scorer
  expects the student name (`CastroBryan`); every lookup fails, the map
  stays empty, and the function returns before writing. Verified (run):
  39 pairs ≥90% on Programming1, no `student_reports/` folder created.
- **C-2 Non-ASCII paths.** `gui_welcome.cpp` converts the chosen folder to
  UTF-8 (`wideToUtf8`); `loadFolder` then uses `FindFirstFileA` and the
  analysis modules use narrow `std::ifstream`, both of which interpret
  the bytes in the ANSI code page. A folder or file name containing
  ñ/é/etc. will not be found or read (Inferred from API semantics; not
  run). Very plausible in the Philippines (student names, Windows user
  names in `C:\Users\...`). Classroom-synced files are safe because their
  names are sanitized to ASCII; manually imported files and parent
  folders are not.
- **C-3 OAuth callback listener** (`google_classroom.cpp:440–502`):
  binds `INADDR_ANY` (reachable from the LAN while waiting, should be
  `127.0.0.1`); the 3-minute timeout uses `SO_RCVTIMEO`, which does not
  apply to `accept()`, so an abandoned sign-in blocks its thread forever;
  no `state` parameter (CSRF) and no PKCE, both recommended for
  installed-app OAuth (RFC 8252). The success page's `\u2014` is UTF-8
  served without a charset (cosmetic mojibake).
- **C-4 Data race on `g_state.progressMsg`** (`std::wstring`): written by
  the worker thread (`gui_welcome.cpp:~99`) while the sidebar paints it on
  the 40 ms animation timer (`gui.cpp:~1554`). Rare, but undefined
  behavior. `g_analysisPhase*` ints are also written cross-thread
  (benign on x64, still technically a race).
- **C-5 HFONT leak**: `gui_assign_dialog.cpp:203` creates a font per dialog
  open and never deletes it (small, bounded by user actions).
- **C-6 Working-directory-relative files** (already flagged in CLAUDE.md):
  `ulccslogo.png`, `ulcclogoloadingscreen.png`,
  `calss_gc_embedded_credentials.json`, `calss_gc_settings.json`,
  `similarity_report_*.html`; `exportPairsToCsv` lacks
  `OFN_NOCHANGEDIR`. Blocks the installer plan discussed today.

Possible concerns (not confirmed):

- **C-7** Closing the window mid-analysis: the analysis thread is detached;
  process exit runs static destructors while it may still be using
  globals. Possible crash-on-exit; no data loss expected.
- **C-8** `tokenizer.cpp` calls `std::isspace/isdigit/isalpha/isalnum` on
  plain `char`; bytes ≥0x80 are negative → undefined behavior.
  `features.cpp` correctly casts to `unsigned char`. Comments and string
  literals are stripped first, so this only affects non-ASCII bytes in
  code proper. Fixing it could change tokens for such files →
  thesis-sensitive (MODERATE; must pass the golden diff).
- **C-9** `loadFolder` key collisions across subfolders (documented in the
  code as a known edge case) — leave.

### 6.4 C++ practices

Generally sound for the project's size: RAII containers everywhere in the
analysis core, no manual `new` outside two load-once GDI+ images, WinHTTP
handles closed on every path, GDI create/delete balanced. Raw pointers
are non-owning views into stable vectors. Nothing here justifies a
"modernization" pass (DO NOT RECOMMEND smart-pointer or `std::optional`
sweeps).

Warnings (`-Wall -Wextra`, Verified run): 21 unknown-pragma (MSVC
`#pragma comment`), 6 unused parameters, 3 missing-field-initializer,
3 function-cast (`GetProcAddress`) — all benign; plus the unused
functions/variables listed in §6.2. `compile.bat` builds without any
warning flags.

### 6.5 Documentation drift

- `CLAUDE.md` "Current state" line-count table (says `gui.cpp` 3,981 /
  `gui_common.h` 613; actual 5,780 / 789).
- `gui_common.h` header comment: "73 cross-screen globals"; actual 79.
  Also says "no gui_common.cpp exists *yet*" although the decision is final.
- `docs/ARCHITECTURE.md` still documents `htmlReport` and CLI mode.

---

## 7. Thesis-Critical Areas

Treat these as frozen unless the team explicitly decides otherwise, and
re-run the golden harness after any change anywhere near them:

| Area | Location | Why |
|---|---|---|
| Source cleaning + token normalization | `tokenizer.cpp` (`strip*`, `extractTokens`, `normalizeTokens`, `C_KEYWORDS`) | defines token similarity |
| Jaccard/LCS and 0.40/0.60 weighting, LCS 500 cap | `tokenizer.cpp:lcsLength/lcsSimilarity/computeSimilarity` | token score |
| 14 features, `FEATURE_MAX`, `C_KW`, `compareStyles` tolerances | `features.cpp` | style score, MATCH/MISMATCH lines, AI input |
| 0.60/0.40 combine, ≥75 flag, label thresholds | `similarity.cpp`, `classifySimilarity` | what gets flagged |
| KNN K=3, distance→score, confidence boost | `knn_model.cpp` | authorship likelihood |
| Student identity / category / period parsing | `main.cpp:extractStudentName/categorizeFile/extractPeriod/isExamFile/isProfileBuildingFile` | which files form profiles and which pairs are compared |
| Pair selection | `runAnalysisPipeline` (exam-only, per-period, cross-student) | experiment population |
| Gemini prompt wording | `gemini.cpp` prompt builders | ethical framing of AI output |

**Two items the team should decide on (not code-cleanup decisions):**

- **T-1 LCS cap vs. denominator.** `lcsLength` truncates each side to 500
  tokens, but `lcsSimilarity` divides by the *full* lengths. Two
  identical 1,000-token files get LCS similarity 2·500/2000 = **0.50**,
  token score 0.70, and can never reach "Exact Duplicate". Not triggered
  by any dataset here (largest file: 383 normalized tokens, Verified run),
  but real final-exam submissions can exceed 500 tokens. Options: document
  it as a limitation in the paper, or change the denominator / raise the
  cap. Any change is HIGH RISK for result comparability.
- **T-2 Flag rate on the validation datasets.** At the 75% threshold, 32%
  (Programming1) and 64% (four sections) of all compared pairs are
  flagged. The datasets are synthetic and share problem templates, so
  some of this is expected; but `manifest.csv` holds the answer key, and
  precision/recall against it should be measured before the thesis makes
  accuracy claims. No code change recommended from this assessment.
- Also confirm intent: pairs are grouped by period **across all loaded
  sections** (students in different blocks are compared). Question only.

---

## 8. Testing / Regression Protection

**What exists:**

- No unit, integration, regression or GUI tests; no test target in
  `compile.bat`.
- Build verification by full link (established practice).
- Manual: the help carousel screenshot tool (`tools/capture_help_screenshots.ps1`).
- Two synthetic datasets with answer keys **outside the repo and
  untracked** (`../stress_test`, 2,973 files; `../stress_test_bscs7`, 960
  files + `generate.py`).
- CLI mode, the only headless way to run the pipeline, was removed today.

**What is protected:** nothing automatically.

**Additionally:** the working tree has 25 modified tracked files plus
several new source files (`secret_store.*`, `gui_ai_settings.cpp`,
`make_release.bat`) that have never been committed. Any refactor started
from here has no clean point to diff or roll back to.

**Needed before refactoring (in order):**

1. A commit of the current working tree.
2. A golden-output harness: a small console program, built separately from
   the app, that runs `runAnalysisPipeline(folder, false)` on the two
   datasets and writes every pair (keys, token, style, combined, label,
   flagged), every profile vector, and the flagged-pair authorship scores
   to CSV. Commit the expected CSVs; any refactor must reproduce them
   byte-for-byte. Results were verified deterministic across runs, so
   exact comparison is appropriate. (This assessment proved the approach
   works by compiling `main.cpp` with `-Dmain=…`; after the §4 `main.cpp`
   split, no trick is needed.)
3. A one-page manual GUI smoke checklist: load folder, run with/without AI,
   each tab, expand a pair, View source compare, CSV export, student
   detail + report button, Classroom sync, AI settings.

---

## 9. Recommended Improvements

Each item is one checkpoint (one category of change), build-verified and,
from Phase 1 on, golden-diffed.

### Phase 0 — Baseline / Verification

| ID | Item | Class |
|---|---|---|
| P0-1 | Commit the current working tree (including today's report/CLI removal) | SAFE |
| P0-2 | Golden-output harness + committed expected CSVs (§8) | SAFE (no app code touched) |
| P0-3 | Record timing baseline (§5.1) next to the harness | SAFE |
| P0-4 | Manual GUI smoke checklist (§8) | SAFE |
| P0-5 | Commit or archive the dataset generators/answer keys so the baseline is reproducible | SAFE |

### Phase 1 — Safe Cleanup

| ID | Item | Class |
|---|---|---|
| P1-1 | Remove the legacy `generateAISummary` call; move the "Generating AI summary" progress phase onto the tiered calls; then remove `buildPrompt`/`aiSummary*` (PF-3) | LOW RISK |
| P1-2 | Delete compiler-confirmed dead code (§6.2) | SAFE |
| P1-3 | Fix stale artifacts: `run.bat`, "View Student Profiles" stub, `gui_common.h` tally, CLAUDE.md table, ARCHITECTURE.md | SAFE |
| P1-4 | **Decide** `generateStudentReports`: remove it (consistent with removing the batch report; it outputs nothing today) **or** fix C-1 (which would start writing files into professors' data folders) | Remove: SAFE · Fix: MODERATE |
| P1-5 | CSV export → `studentNameForFile`; delete `extractStudentNameGC`; dedupe `extractPeriodGC` and string converters | LOW RISK (CSV names change for legacy filenames — that is the fix) |
| P1-6 | Free the assign-dialog font (C-5) | SAFE |
| P1-7 | Document the `g_analysisResults` threading invariant in `results_data.h` | SAFE |

### Phase 1b — Correctness fixes (separate checkpoints, not refactoring)

| ID | Item | Class |
|---|---|---|
| D-1 | `progressMsg` race: pass the result string via the completion message (or set it on the GUI thread) | LOW RISK |
| D-2 | Exe-relative reads + `%LOCALAPPDATA%`/Documents writes + `OFN_NOCHANGEDIR` (C-6; prerequisite for the installer) | LOW RISK |
| D-3 | OAuth listener: bind loopback, real accept timeout (`select`), add `state` (+ PKCE if time allows) (C-3) | LOW–MODERATE (needs a real sign-in test) |
| D-4 | Wide-character file I/O end to end (C-2); first reproduce with a folder named e.g. `Peña` | MODERATE |
| D-5 | Cap pairs sent to Gemini + explicit WinHTTP timeouts (PF-7) | MODERATE |

### Phase 2 — Controlled Refactoring

| ID | Item | Class |
|---|---|---|
| R-1 | Split `main.cpp` → `main.cpp` / `analysis_pipeline.cpp` / `student_report.cpp` (§4); full `compile.bat` update; symbol ledger | LOW RISK |
| R-2 | Extract the View source modal → `gui_source_view.cpp` (§4), same checkpoint procedure as the ContentProc split | LOW RISK |
| R-3 | Optional: Flagged toolbar/CSV/copy-findings → `gui_flagged.cpp` | LOW RISK, low value |

### Phase 3 — Performance Optimization

| ID | Item | Class |
|---|---|---|
| PF-1 | Per-run memoization of `tokenize`/`extractFeatures` results — only if multi-section loads matter | LOW RISK (golden diff must be identical) |
| PF-2 | Profile the unattributed ~30 s on the four-section dataset before any further change | SAFE |

### Phase 4 — Optional Future Improvements

- Shared WinHTTP helper for Gemini + Classroom.
- `-Wall -Wextra` in a dev build script; `.rc` file (icon, version info,
  manifest for comctl32 v6/DPI) — naturally part of the installer work.
- C-7 orderly shutdown of the analysis thread.
- C-8 `unsigned char` casts in `tokenizer.cpp` (thesis-sensitive; only
  with golden diff and a team decision).
- Token interning for LCS (PF-5) — DO NOT RECOMMEND now.

---

## 10. Do Not Touch

- **Every item in §7** — the measured behavior *is* the thesis result.
- **Keyword-list unification** (tokenizer vs features) — would silently
  change feature vectors.
- **GUI file boundaries and the `gui_common.h` globals model** — recently
  and deliberately established; working; consolidation was decided against
  on 2026-08-24.
- **`SyncSheetProc`/`ContentProc` dispatchers** — already the intended end
  state.
- **Hand-rolled JSON in `gemini.cpp`/`google_classroom.cpp`** — fragile in
  theory, correct for the fixed response shapes, and a JSON library would
  break the no-third-party-deps constraint.
- **`secret_store.cpp`, `audit_log.cpp`** — small, recent, correct.
- **Parallelism, data-structure rewrites, "modern C++" sweeps** — no
  measured need.

---

## 11. Proposed Refactoring Order

```
P0-1 commit working tree
  ↓
P0-2..P0-5 golden harness + timing baseline + smoke checklist + datasets
  ↓
Phase 1 cleanup, one checkpoint each (P1-2 dead code and P1-3 stale
artifacts first: zero behavior change; then P1-1, P1-4, P1-5 …)
  ↓
Phase 1b correctness fixes, one at a time (D-1, D-2 before the installer;
D-3; D-4 after reproducing; D-5)
  ↓
Phase 2 structural moves (R-1 first: it lets the harness drop the
-Dmain trick; then R-2)
  ↓
Phase 3 only if the timing baseline shows multi-section runs matter
  ↓
golden diff + build + smoke checklist after every step
```

In parallel, and independent of code: the team decides T-1 (LCS cap) and
measures T-2 (flag rate vs. `manifest.csv`) for the paper.

Rationale for this order: the harness must exist before anything
touches the pipeline; zero-behavior-change deletions come first so later
diffs are smaller; correctness fixes come before structural moves so each
fix lands in code the developers already know; structural moves come
before performance work so memoization lands in the new pipeline file
rather than being moved twice.

---

## 12. Final Recommendation

If this were my codebase, before calling CALSS finished I would:

1. **Commit what exists and build the golden-output harness.** Everything
   else depends on being able to prove "results unchanged".
2. **Fix the real defects:** delete or repair `generateStudentReports`
   (it does nothing today), stop the discarded Gemini call, fix the CSV
   name inconsistency, close the `progressMsg` race, make file locations
   exe-/AppData-relative (needed for the installer anyway), and harden the
   OAuth listener. Reproduce and then fix the non-ASCII path failure —
   it is the one most likely to embarrass the tool in front of a real
   professor.
3. **Delete the confirmed dead code and fix the stale scripts/docs.**
4. **Make two structural moves:** pipeline out of `main.cpp`, View source
   out of `gui.cpp`. Nothing else structural.
5. **Leave performance alone** unless professors will routinely load
   several sections at once; if so, add per-file memoization and nothing
   more.
6. **Not touch the algorithms**, but write up T-1 and T-2 honestly in the
   thesis — they matter more to the defense than any refactor.

The codebase is not in poor shape: the dependency direction is clean, the
GUI split is sound, the analysis core is small and deterministic. The work
that remains is correctness, regression protection, and a little
housekeeping, not optimization.
