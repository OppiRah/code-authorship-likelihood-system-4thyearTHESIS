# CALSS Architecture

CALSS (Code Authorship Likelihood Scoring System) is a Windows desktop tool
for professors at University of Luzon, College of Computer Studies, to
detect authorship patterns and similarity across student C-language
submissions. C++17, MinGW-w64 g++, Win32 + GDI+, no external UI framework —
a single static executable with no third-party runtime dependencies. Two
external integrations: Google Gemini (AI summaries) and Google Classroom
(OAuth 2.0, submission download). Pipeline: tokenization/token similarity →
14-feature stylometric extraction → KNN classification (K=3).

This document covers every `.h` in `include/` and every `.cpp` in `src/` —
26 files total, ~16,500 lines. Grouped by layer, not alphabetically.

---

## Files at a glance

| File | Layer | Purpose |
|---|---|---|
| `main.cpp` | Pipeline | Entry point, HTML report generation, the full analysis pipeline |
| `tokenizer.h`/`.cpp` | Pipeline | Token extraction + Jaccard/LCS similarity |
| `features.h`/`.cpp` | Pipeline | 14-feature stylometric extraction |
| `knn_model.h`/`.cpp` | Pipeline | Per-student KNN authorship scorer |
| `similarity.h`/`.cpp` | Pipeline | Pairwise similarity engine (combines tokenizer + features) |
| `gemini.h`/`.cpp` | Integration | Gemini API calls (AI summaries, evidence narratives) |
| `google_classroom.h`/`.cpp` | Integration | OAuth + Classroom API + submission download |
| `results_data.h` | Data | GUI-facing display structs; the main↔GUI contract |
| `audit_log.h`/`.cpp` | Data | Timestamped audit log + SHA-256 |
| `gui_common.h` | GUI | Shared contract binding all `gui_*.cpp` files together |
| `gui.cpp` | GUI | Original monolith; window chrome, sidebar, ContentProc dispatcher |
| `gui_sync_sheet.cpp` | GUI | Google Classroom sync modal (CONNECT→CHOOSE→SYNCING→DONE) |
| `gui_assign_dialog.cpp` | GUI | Unclassified-file assignment dialog |
| `gui_help_carousel.cpp` | GUI | Multi-step Quick Start Guide overlay |
| `gui_welcome.cpp` | GUI | Idle/welcome screen, splash screen, analysis thread |
| `gui_overview.cpp` | GUI | Overview tab (stats dashboard, AI summary) |
| `gui_flagged.cpp` | GUI | Flagged Pairs tab (largest split file) |
| `gui_students.cpp` | GUI | Students/Classes tab |
| `gui.h` | GUI | One-function public entry point (`runGUI`) |

---

## Analysis pipeline

### `main.cpp`

**Purpose.** Entry point and orchestrator. Owns the analysis pipeline
(`runAnalysisPipeline`), the legacy HTML report generator, per-student
similarity reports, and the file-categorization/student-name-extraction
logic every other layer depends on to make sense of a folder full of `.c`
files. At 2,474 lines it's the second-largest file in the project after
`gui.cpp`.

**Key types/globals.**
- `FileCategory` (`Exam`/`Quiz`/`Activity`/`Unclassified`) — the priority
  hierarchy `categorizeFile()` implements (Classroom title signal → filename
  keyword → unclassified).
- `LanguageCheck` — result of `checkLanguage()`'s "is this actually C" scan.
- `g_analysisResults` (`AnalysisResults`, defined here, `extern`-declared in
  `gui_common.h`) — the single global results object every GUI screen reads
  from. This is the load-bearing bridge between the pipeline and the GUI.
- `g_analysisProgressCb` (static) — the registered `AnalysisProgressCallback`
  (type from `results_data.h`), set by the GUI before a run via
  `setAnalysisProgressCallback()`.

**Key functions.**
- `extractStudentName` — parses a filename (Classroom-synced or manual) down
  to the student's name, stripping the `{period}{type}__` prefix, the
  `_ASGN######` fingerprint, and multi-attachment `_N` suffixes.
- `categorizeFile` / `extractPeriod` / `isExamFile` / `isProfileBuildingFile`
  — the categorization/period hierarchy; only Activity/Quiz files build a
  student's style profile, only Exam files get pairwise-compared, and
  comparison is period-scoped (a Midterm never compares against a Final).
- `checkLanguage` — lightweight text-pattern scan (not a parser) that flags
  Java/Python/JavaScript/C++ submitted with a `.c` extension, so a wrong-
  language file doesn't silently corrupt a style profile or produce a
  misleading "Low Likelihood" score.
- `loadFolder` — two-level `.c` file discovery (root + one level of
  subfolders, the subfolder name becoming that file's "block").
- `htmlReport` — builds the legacy standalone HTML report (own inline
  CSS, own color scheme, independent of the GUI's dark theme).
- `generateStudentReports` / `generateStudentSimilarityReport` — per-student
  HTML reports for severe (≥90%) flagged pairs; the latter is `extern`-
  declared in `gui_common.h` for the Students tab's "Generate Similarity
  Report" button.
- `runAnalysisPipeline` — the actual pipeline: load → language-check →
  build profiles (with cold-start fallback for students with no
  Activity/Quiz submissions) → pairwise-compare exam files (period-grouped)
  → AI summary (legacy single-blob + tiered) → write HTML → populate
  `g_analysisResults`. Reports progress via `reportProgress()`/the
  registered callback at real checkpoints (not simulated).
- `main` — dispatches to `runGUI()` (no args, the normal path) or CLI mode
  (a data-folder arg, "kept for testing and scripting").

**Depends on:** `tokenizer.h`, `features.h`, `knn_model.h`, `similarity.h`,
`gemini.h`, `gui.h`. Does **not** include `gui_common.h` or `results_data.h`
directly for its own struct definitions — `AnalysisResults` et al. come
from `results_data.h` transitively via `gemini.h`.

**Depended on by:** `gui_common.h` forward-declares `runAnalysisPipeline`,
`generateStudentSimilarityReport`, `g_analysisResults`, and
`setAnalysisProgressCallback` as `extern` — every `gui_*.cpp` file reaches
into `main.cpp` through those four names. `gui_welcome.cpp`'s
`analysisThread()` is the actual caller of `runAnalysisPipeline` and
`setAnalysisProgressCallback` in the running app.

---

### `tokenizer.h` / `tokenizer.cpp`

**Purpose.** Turns a `.c` source file into a normalized token stream and
scores two token streams' similarity. This is the "does the code look
copy-pasted" half of the combined score (60% weight in `similarity.cpp`).

**Key types (header).** `TokenType` enum (`KEYWORD`/`IDENTIFIER`/`NUMBER`/
`OPERATOR`/`PUNCTUATION`/`STRING_LIT`/`UNKNOWN`); `Token`; `TokenResult`
(raw + normalized token vectors, keyword frequency map, extracted
identifiers); `SimilarityScore` (jaccard/lcs/combined + label/color).

**Key functions (impl).**
- `tokenize` — strips comments/preprocessor lines, replaces string/char
  literals with placeholders, extracts a raw token stream, then normalizes
  it (`VAR_0`, `VAR_1`, ... for user identifiers; keywords/operators kept
  literal) via `normalizeTokens`.
- `jaccardSimilarity` — set-overlap similarity over normalized tokens.
- `lcsSimilarity` — longest-common-subsequence similarity (space-optimized
  DP, capped at 500 tokens per side for performance).
- `computeSimilarity` — combines both (40% jaccard + 60% lcs) into one
  `SimilarityScore`, classified via `classifySimilarity`.
- `classifySimilarity` — score → label/color bucket (Exact Duplicate / High
  / Moderate / Low / Minimal Similarity), thresholded at 95/80/60/40%.

**Depends on:** nothing project-internal — pure `<string>`/`<vector>`/
`<map>` logic, no Windows dependency.

**Depended on by:** `similarity.cpp` (the token-similarity half of
`analyzePair`), `main.cpp`'s legacy CLI self-test block (commented out).

---

### `features.h` / `features.cpp`

**Purpose.** Extracts the 14 coding-style features that make up a
student's "style DNA" — naming conventions, comment habits, bracket
placement, spacing, function/line length, etc. This is the "does this
match how this student normally writes" half of both the pairwise
similarity score (40% weight) and the KNN authorship scorer.

**Key types/constants (header).** `FEATURE_COUNT` (14, `#define`);
`FEATURE_MAX[14]` / `FEATURE_LABELS[14]` (per-feature normalization
ceiling + display label, defined in the `.cpp`); `FeatureResult` (raw +
0–1-normalized value vectors); `StyleNote` (feature/observation pair).

**Key functions (impl).**
- `extractFeatures` — the 14-feature extraction itself: identifier naming
  stats (length, single-letter/camelCase/snake_case/ALL_CAPS ratios),
  comment density/style, bracket-same-line ratio, for-vs-while preference,
  operator spacing, line/function length, blank-line ratio, return-at-end
  ratio. All hand-rolled text scans, no AST.
  Nothing here executes student code — see `main.cpp`'s language-check
  rationale for why that's a deliberate constraint, not an oversight.
- `normalizeVector` / `averageVectors` — scale raw values to 0–1 via
  `FEATURE_MAX`; average a set of normalized vectors into one profile.
- `euclideanDistance` / `styleSimilarity` — distance between two feature
  vectors, converted to a 0–1 similarity (`1 - dist/maxPossibleDist`).
- `describeStyle` — turns a raw feature vector into human-readable
  `StyleNote`s ("Prefers long descriptive variable names", etc.) for
  profile display.
- `compareStyles` — per-feature MATCH/MISMATCH classification (a submission
  against a profile, threshold per feature), the source of every
  `StyleNoteDisplay` the GUI renders.

**Depends on:** nothing project-internal, no Windows dependency.

**Depended on by:** `knn_model.cpp` (feature extraction + comparison for
scoring), `similarity.cpp` (the style half of `analyzePair`), `main.cpp`
(profile-vector capture for `StudentProfileSummary.featureVector`, the
Style DNA strip's data source).

---

### `knn_model.h` / `knn_model.cpp`

**Purpose.** The K=3 nearest-neighbor authorship likelihood scorer:
given a student's accumulated style profile and a new submission, how
likely is it the same person wrote both.

**Key types (header).** `AuthorshipResult` (score/label/color/profileSize/
per-feature `styleMatch` notes); `PairVerdict` (two `AuthorshipResult`s +
a likely-original/likely-copier guess); `StudentProfile` (name +
submissions + averaged `profileVector` + `styleNotes`); `AuthorshipScorer`
class (owns a `map<string, StudentProfile>`, K configurable at
construction, default 3).

**Key functions (impl).**
- `AuthorshipScorer::loadSubmission` — extracts features from a file,
  appends to that student's profile, rebuilds the profile's averaged
  vector and style notes (`rebuildProfile`).
- `AuthorshipScorer::score` — the KNN itself: euclidean-distances the test
  submission against every known submission in the profile, averages the
  K nearest, converts to a 0–1 score, applies a confidence boost scaled by
  profile size (more submissions on file = more reliable), classifies via
  `classifyAuthorship`.
- `AuthorshipScorer::scorePair` — scores both sides of a pair and picks
  the lower-scoring one as "likely copier" (a guess, not an accusation —
  see the GUI's careful "likely original/copier" framing).
- `classifyAuthorship` — score → label/color bucket (High/Moderate/Low/
  Very Low Likelihood).

**Depends on:** `features.h` (feature extraction, `styleSimilarity`,
`compareStyles`).

**Depended on by:** `main.cpp` (the pipeline's per-pair authorship scoring
step, plus the HTML report and student-report generators), `gemini.h`/
`gemini.cpp` (`buildPrompt` takes an `AuthorshipScorer&` for the legacy
single-blob AI summary path).

---

### `similarity.h` / `similarity.cpp`

**Purpose.** The pairwise comparison engine that combines `tokenizer.cpp`
and `features.cpp` into one similarity score per submission pair, and
runs that comparison across every pair in a batch.

**Key types (header).** `PairResult` (token/style/combined scores +
label/color + `flagged` bool); `SummaryStats` (aggregate counts across a
result set).

**Key functions (impl).**
- `analyzePair` — tokenizes and extracts features for both files, computes
  token similarity (`computeSimilarity`) and style similarity
  (`styleSimilarity`), combines 60% token + 40% style, classifies, flags
  if combined ≥ 75%.
- `analyzeAll` — all-pairs comparison over a name→path map, sorted flagged-
  first then by score descending.
- `flaggedPairs` — filters a result set to just the flagged ones.
- `summaryStats` — aggregate totals (flagged count, exact-duplicate count,
  high-similarity count, average/max score) over a result set.

**Depends on:** `tokenizer.h`, `features.h`.

**Depended on by:** `main.cpp` (the pipeline's pairwise-comparison step —
though `main.cpp` actually calls `analyzePair`/period-grouped logic
directly rather than the batch `analyzeAll`, since it needs period-scoped
grouping `analyzeAll` doesn't know about), `gemini.h`/`gemini.cpp`
(`PairResult` is the legacy AI-summary prompt's input type).

---

## External integrations

### `gemini.h` / `gemini.cpp`

**Purpose.** Google Gemini API integration for AI-generated instructor-
facing summaries. Two generations of API live here side by side: a legacy
single-blob summary (`generateAISummary`, still feeding the standalone
HTML report) and a newer tiered restructure (`generatePairNarratives` /
`generateCrossPairPatterns`, feeding the GUI's Overview and Flagged Pairs
tabs). Uses WinHTTP directly — no HTTP client library dependency.

**Key types (header).** `AISummary` (text/success/error/skipped) — the
legacy path's result type. The tiered path's result types
(`AIPairNarrative`, `AIBatchPatterns`) are deliberately declared in
`results_data.h` instead, to avoid a circular include (`AnalysisResults`
needs them as real members, and `results_data.h` is included by this
header).

**Key functions (impl).**
- `loadApiKey` — reads `api_key.txt`, treats the placeholder string as
  "no key configured" (graceful skip, not an error).
- `buildPrompt` / `generateAISummary` — legacy path: builds one big prompt
  covering every flagged pair with explicit "don't accuse, don't name a
  cheater" rules, calls Gemini once, returns free text.
- `callGeminiAPI` (static) — the actual WinHTTP POST to
  `generativelanguage.googleapis.com`, shared by every public entry point
  in this file.
- `buildPairNarrativePrompt` / `generatePairNarratives` — Tier 1: one
  batched call returning a JSON array of per-pair evidence narratives
  (2–3 sentences each), parallel to the `flagged` vector passed in.
- `buildCrossPairPrompt` / `generateCrossPairPatterns` — Tier 2: cross-pair
  pattern detection (shared deviating-feature clusters vs. independent
  incidents), skipped outright when fewer than 2 pairs are flagged.
- `extractTextFromJson` / `parseJsonStringLiteral` / `extractJsonStringArray`
  — hand-rolled JSON string/array extraction (no JSON library dependency,
  consistent with the rest of the project).

**Depends on:** `similarity.h`, `knn_model.h`, `results_data.h`; WinHTTP
(`_WIN32`-gated).

**Depended on by:** `main.cpp` (`runAnalysisPipeline` calls all three
generation functions), `gui_common.h` transitively includes this header
(so every GUI file sees `AISummary`, `AIPairNarrative`, etc.), though the
GUI never calls into `gemini.cpp` directly — it only reads the results
`main.cpp` already put into `g_analysisResults`.

---

### `google_classroom.h` / `google_classroom.cpp`

**Purpose.** Google Classroom OAuth 2.0 (installed-app flow) and API
integration: authenticate, list courses/assignments, download `.c`
submissions with category/period metadata embedded in the filename. At
960 lines, the largest non-`main.cpp`/non-GUI file.

**Key types (header).** `GCourse`, `GAssignment`, `GCToken`,
`GCCredentials`, `GCResult`; `SyncManifestEntry` (title/block, for
display). `#define`s for settings/token file names and the fixed OAuth
callback port (8080).

**Key functions (impl).**
- `gc_loadCredentials` / `gc_saveSettings` / `gc_loadSettings` — credential
  persistence, falling back to embedded project-default credentials
  (`loadEmbeddedDefaults`, from a gitignored JSON file) so an instructor
  never has to hunt down their own OAuth client — "Sync just works."
- `gc_authenticate` — opens the system browser to Google's consent screen,
  then `listenForCode()` blocks on a local Winsock listener (port 8080)
  for the redirect, exchanges the code for tokens (`exchangeCode`).
- `gc_hasValidToken` / `gc_refreshIfNeeded` — token expiry check + silent
  refresh via the stored refresh token.
- `gc_getCourses` / `gc_getAssignments` — thin wrappers over
  `classroom.googleapis.com` REST calls, parsed with hand-rolled
  `jsonGet`/`jsonArray` (no JSON library).
- `gc_downloadSubmissions` — the core sync operation: for each turned-in
  submission, resolves the student's real name (falling back to a stable
  ID-derived name if the roster-read scope isn't granted, silently
  triggering `gc_signOut()` so the *next* sync prompts for the missing
  scope), downloads each `.c` Drive attachment, and names it
  `{period}{type}__{studentName}_ASGN{fingerprint}.c` — the exact encoding
  `main.cpp`'s `categorizeFile`/`extractStudentName`/`extractPeriod`
  decode on the read side. Also writes a `calss_sync_manifest.jsonl` entry
  per file (assignment title + course/"block") for display purposes only.
- `gc_loadSyncManifest` — reads that manifest back, used by the Students
  tab to group cards by class block.

**Depends on:** WinHTTP + Winsock2 (`_WIN32`-gated), `<filesystem>`.
No dependency on any other project file.

**Depended on by:** `main.cpp` (indirectly, via the filename convention
this file writes and `main.cpp` parses — no direct call), `gui_sync_sheet.cpp`
(the actual caller of every `gc_*` function, via its three background
worker threads), `gui_students.cpp` (`gc_loadSyncManifest`, for block
grouping).

---

## Data / logging

### `results_data.h`

**Purpose.** Header-only. Defines every struct used to hand analysis
results from `main.cpp` to the GUI. This is the contract between the
pipeline and the presentation layer — nothing here does any computation.

**Key types.**
- `AnalysisPhase` enum + `AnalysisProgressCallback` — the progress-
  reporting hook shared verbatim between `main.cpp` (which calls it) and
  the GUI (which registers it), declared once here specifically so the
  two translation units can't drift into incompatible copies.
- `StyleNoteDisplay` / `AuthorshipDisplay` / `PairAnalysisDisplay` — the
  GUI-ready, per-feature/per-student/per-pair display structs (as opposed
  to `knn_model.h`'s raw scoring structs).
- `AIPairNarrative` / `AIBatchPatterns` — the tiered AI-summary result
  types (defined here rather than in `gemini.h` to avoid a circular
  include — see `gemini.h`'s note).
- `StudentProfileSummary` — GUI-facing student profile, including the raw
  `featureVector` the Style DNA strip renders directly as bar heights.
- `PairRow` — lightweight row for the (currently minimally-used) "all
  pairs" table.
- `AnalysisResults` — the big one: every field the GUI's three tabs read,
  including `unclassifiedFiles` and `languageMismatchFiles` for the two
  "needs instructor attention" notices on the Overview tab.

**Depends on:** `<string>`, `<vector>`, `<functional>` only.

**Depended on by:** `gemini.h` (includes it directly), `gui_common.h`
(includes it, making every struct here visible to every GUI file),
`main.cpp` (populates `AnalysisResults` via `g_analysisResults`).

---

### `audit_log.h` / `audit_log.cpp`

**Purpose.** Append-only audit trail for academic-integrity defensibility
("who ran what analysis when"), plus a self-contained SHA-256
implementation for report tamper-evidence hashing.

**Key types/constants (header).** `AUDIT_LOG_FILE` (fallback filename
only — the real path is resolved at runtime, see below).

**Key functions (impl).**
- `audit_log` — appends a timestamped, indented entry under
  `%PROGRAMDATA%\CALSS\audit.log`, not next to the executable — deliberately
  outside a student's easy reach, since modifying `ProgramData` needs
  elevated permissions.
- `audit_sha256File` / `audit_sha256String` — hash a file or string via a
  hand-rolled, public-domain-algorithm SHA-256 (`SHA256` struct in an
  anonymous namespace) — no external crypto library dependency, consistent
  with the project's "no third-party runtime deps" constraint.
- `audit_getLogPath` — resolves (and caches) the full log path via
  `SHGetFolderPathA(CSIDL_COMMON_APPDATA)`, falling back to the local
  folder if `ProgramData` is unavailable.

**Depends on:** `<windows.h>`, `<shlobj.h>` — Windows-only, no other
project file.

**Depended on by:** `gui_sync_sheet.cpp` (logs every Classroom sync
attempt), `gui_assign_dialog.cpp` (logs manual file assignment). Declared
in `gui_common.h`'s include prologue so every split GUI file can call it.

---

## GUI

The GUI is Win32 + GDI+, hand-rolled — every screen paints itself to an
offscreen buffer then `BitBlt`s once (see "Key patterns" in the project's
`CLAUDE.md`). It started as one file (`gui.cpp`) and has since been split
into `gui.cpp` plus seven `gui_*.cpp` files, each owning one screen or
subsystem. `include/gui_common.h` is what makes that split possible.

### `gui_common.h`

**Purpose.** The shared contract binding `gui.cpp` and every `gui_*.cpp`
split file into one coherent program. This is **not** a normal "shared
utilities" header — it's the only thing the split files share, and it
exists specifically so the multi-file split could happen at all without a
`gui_common.cpp` (a consolidation that was explicitly considered and
rejected — see below).

Every `gui_*.cpp` includes this file first, before anything else, so all
translation units see an identical preprocessor state — critical because
`UNICODE`/`_UNICODE`/`WINVER` must be defined *before* `<windows.h>` loads,
and getting that wrong causes silent ANSI/wide-string mismatches (the
project hit exactly this bug once: tree-view rows rendering as single
letters because `commctrl.h` macros silently resolved to `*_A` variants).

**What it carries, in order:**
1. The include prologue (analysis headers *before* Windows headers,
   because Windows macros like `ERROR`/`IDENTIFIER` collide with project
   enum values).
2. App-wide `#define`s — timer IDs, control IDs, custom `WM_USER+N`
   messages (`WM_SYNC_AUTHDONE` etc.).
3. The design-system palette (`MAROON_*`/`GOLD_*`/`GRAY_*`/`RISK_*`
   `COLORREF` constants, plus a legacy-alias layer like `UL_MAROON`/
   `BG_MAIN` for call sites not yet migrated to the new names) and layout
   constants (`SIDEBAR_WIDTH`, `TAB_BAR_HEIGHT`, etc.).
4. Shared types referenced in any cross-file signature or extern global —
   `AppState`, the toast/modal system (`AppToast`, `AppModal`,
   `ModalKind`), `FocusKind`/`FocusableItem` (the keyboard-focus system),
   `ResultsPage`, and every per-screen hit-rect struct (`StudentCardRect`,
   `CollapsedPairRowRect`, `BlockTabRect`, etc.).
5. Four `extern` declarations reaching into `main.cpp`
   (`runAnalysisPipeline`, `generateStudentSimilarityReport`,
   `g_analysisResults`, `setAnalysisProgressCallback`) — `main.cpp` itself
   doesn't include this header, so these are one-directional.
6. `extern` declarations for every cross-screen global (~68 as of the
   Students-tab split) — each is *defined exactly once*, but **not**
   centralized in one file. As of the most recent split checkpoint, most
   stay defined in `gui.cpp`; the rest are defined in whichever split file
   owns that subsystem (e.g. `g_syncFocusedIndex` in `gui_sync_sheet.cpp`,
   `g_helpCarouselState` in `gui_help_carousel.cpp`). Adding an `extern`
   here without a matching definition somewhere is a linker error that a
   single-file syntax check will never catch — this is why every split
   checkpoint's verification step was a full link build, not just a
   compile check.
7. Two `inline` helpers (`S()` — DPI-scale a design pixel value; and
   `easeOutCubic()` — the shared animation-easing function) kept `inline`
   in the header rather than moved to a `.cpp`, preserving their original
   codegen at roughly 50 call sites per paint.
8. Prototypes for every cross-screen function (~59) — `addFocusable`,
   `drawCard`, `drawDnaStrip`, `showToast`, `showAppModal`, and each split
   file's public entry points (`drawStatsDashboard`/`handleOverviewClick`
   from `gui_overview.cpp`, `drawFlaggedPairs`/`handleFlaggedPairsClick`
   from `gui_flagged.cpp`, `drawStudentsPage`/`handleStudentsClick` from
   `gui_students.cpp`, `activateSyncFocused`/`onGoogleClassroom` from
   `gui_sync_sheet.cpp`, etc.) — but not every function in a split file:
   e.g. `gui_sync_sheet.cpp`'s own per-state helpers
   (`handleSyncConnectClick` and siblings) are `static` and never appear
   here, because nothing outside that file calls them.

**No `gui_common.cpp`, and none is coming.** An earlier plan called for a
ninth file hosting `ContentProc` itself plus the consolidated ~68 shared
globals. That plan was superseded: globals stay defined across `gui.cpp`
and whichever split file already owns that subsystem, matching how the
rest of the split already works, rather than centralizing into a new file
that would just become another monolith.

**Depends on:** `tokenizer.h`, `features.h`, `knn_model.h`, `similarity.h`,
`gemini.h`, `results_data.h`, `google_classroom.h`, `audit_log.h`, `gui.h`
(which pulls in `<windows.h>`), plus `<commctrl.h>`, GDI+, `<thread>`,
`<atomic>`, etc.

**Depended on by:** every one of the eight GUI `.cpp` files (`gui.cpp` and
all seven splits) — it is their only shared include.

---

### `gui.cpp`

**Purpose.** The original monolith. Still the largest file in the project
(3,981 lines) even after four extraction checkpoints pulled entire
screens out of it. What's still here, deliberately:

- **Window chrome and infrastructure that has no single "owning tab"**:
  `WindowProc` (the main frameless window — custom `WM_NCCALCSIZE`/
  `WM_NCHITTEST` frame, DPI awareness setup, menu/status-bar creation),
  `TitleBarProc` (custom title bar + File/View/Help menu), `SidebarProc`
  (the left-nav sidebar: logo, folder/import/sync/run buttons, its own
  `WM_PAINT`).
- **`ContentProc`** — still here (~753 lines) as a thin dispatcher: routes
  `WM_PAINT`/`WM_LBUTTONDOWN`/`WM_MOUSEMOVE`/etc. to whichever tab's own
  draw/click functions apply (`drawStatsDashboard`+`drawAISummary` for
  Overview, `drawStudentsPage`+`handleStudentsClick` for Students,
  `drawFlaggedPairs`+`handleFlaggedPairsClick` for Flagged Pairs), plus
  genuinely page-agnostic shared overlay drawing (toasts, the app modal,
  the help carousel) that doesn't belong to any one tab.
- **The cross-screen primitives `gui_common.h` prototypes but doesn't
  define**: `addFocusable`, `drawCard`, `drawDnaStrip`, `drawFocusRing`,
  `showToast`/`showAppModal`/`dismissAppModal`, `truncateMiddle`,
  `makeRoundRectPath`, the font/color helpers (`s2w`, `fmtPct`,
  `pairSeverityColor`), the keyboard-focus system's core (`focusMove`,
  `focusableAt`, `activateFocused`).
- **App lifecycle**: `runGUI` (DPI awareness → splash screen → font/window
  creation → message loop), `createFonts`/`destroyFonts`,
  `createSidebarButtons`/`layoutSidebarControls`/`layoutChildren`.
- Most (52 of ~68) of the shared globals `gui_common.h` declares `extern`.

**Key types/globals.** Owns the bulk of the shared globals: font handles
(`g_hFontH1` etc.), `g_hMainWindow`/`g_hContent`/`g_hSidebar`, `g_state`
(`AppState`), `g_dpiScale`, `g_scrollY`, `g_currentPage`,
`g_contentFocusables`, `g_appModal`, and every hit-rect vector not owned
by a specific split file.

**Key functions.** In addition to the four window procedures: `runGUI`
(entry from `main.cpp`), `onRunAnalysis`/`onSelectFolder`/`onImportFiles`/
`onViewReport` (sidebar button handlers), `handleMenuCommand`, `drawTabBar`/
`drawContextStrip` (the fixed chrome above every tab's content — shared,
not tab-specific, which is why they didn't move with any one tab split).

**Depends on:** `gui_common.h` only (as every split file does).

**Depended on by:** every split file calls back into `gui.cpp` for the
primitives above; `main.cpp` calls `runGUI` (declared in `gui.h`).

---

### `gui_sync_sheet.cpp`

**Purpose.** The Google Classroom sync modal — a single window that
changes *state* (`SHEET_CONNECT → SHEET_CHOOSE → SHEET_SYNCING →
SHEET_DONE`) rather than closing and reopening, replacing what used to be
a chain of native `MessageBox` popups. Owns three background worker
threads (`syncAuthThread`, `syncFetchThread`, `syncRunThread`) so network
calls never block the UI thread, and its own parallel keyboard-focus
system (deliberately separate from the main content's, since the main
window is disabled — `EnableWindow(FALSE)` — the whole time this sheet is
open, so the two systems never need to interleave).

**Why split first / why this boundary.** This was the first file pulled
out of `gui.cpp` — a self-contained state machine with its own window
procedure (`SyncSheetProc`), its own globals, and business logic
(Prelim-exclusion, per-block manifest tagging, drop-zone folder creation)
that never touches any other tab. It was also the subject of the project's
*final* split checkpoint: `SyncSheetProc`'s `WM_PAINT` and
`WM_LBUTTONDOWN` handlers were themselves originally one 4-way
if/else-if chain per message (mirroring `ContentProc`'s pre-split shape at
smaller scale) and have since been extracted into eight named helpers —
`drawSyncConnectState`/`drawSyncChooseState`/`drawSyncSyncingState`/
`drawSyncDoneState` and `handleSyncConnectClick`/`handleSyncChooseClick`/
`handleSyncSyncingClick`/`handleSyncDoneClick` — so both handlers are now
thin dispatchers on `g_syncState`, the same shape `ContentProc` itself
ended up in. This stayed a single-file, in-file-only extraction (no new
translation unit, no `compile.bat` change) since none of it needed to be
reachable from any other file.

**Key types/globals.** `SyncSheetState` enum; `SyncAssignmentRow`/
`SyncBlockNode` (the tree's backing data — one node per Classroom
course/section, rows per assignment); `g_syncState`, `g_syncBlocks`,
`g_syncCreds`, the four accumulator globals mirroring a completed run's
results (`totalFilesDownloadedG` etc.). `g_hSyncSheet`/`g_hSyncTree`/
`g_syncSheetFocusables`/`g_syncFocusedIndex` are the four globals promoted
to the `gui_common.h` contract (needed elsewhere: the main message loop
routes input to this sheet by checking `msg.hwnd == g_hSyncSheet`).

**Key functions.** `openSyncSheet`/`closeSyncSheet` (lifecycle);
`syncSheetSetState`/`syncSheetLayout` (state transitions resize/reposition
the window and show/hide its two native child controls — a checkbox
`SysTreeView32` for CHOOSE, a `LISTBOX` for SYNCING); the three thread
functions; the eight `drawSync*State`/`handleSync*Click` helpers described
above; `activateSyncFocused` (Enter-key activation, mirroring `gui.cpp`'s
`activateFocused` but against this sheet's own focus list);
`onGoogleClassroom` (the one function `gui_common.h` exposes — `gui.cpp`'s
sidebar button handler calls this and nothing else).

**Depends on:** `gui_common.h`; calls into `google_classroom.h`'s `gc_*`
functions for every actual network operation.

**Depended on by:** `gui.cpp` (`onGoogleClassroom` from the sidebar
button), the main message loop (routes `WM_LBUTTONUP` focus-reclaim and
Tab/Enter keyboard handling to this sheet when it's the active window).

---

### `gui_assign_dialog.cpp`

**Purpose.** The modal dialog that lets an instructor manually attach a
student/category/period to a file the pipeline couldn't auto-categorize
(unknown type, unknown student, or both — see `main.cpp`'s
`categorizeFile` priority hierarchy), plus the file-rename/delete
operations that follow from it. Renames the file to match the exact
`{period}{type}__{name}_ASGN{fingerprint}.c` convention Classroom sync
already uses, so it becomes indistinguishable from a normally-synced file
to every downstream function — no special-casing needed anywhere else in
the pipeline.

**Why this boundary.** A self-contained native-Win32-styled dialog (not
the dark custom theme used elsewhere — deliberately, since it's a
lightweight one-off utility, not main app chrome) with its own
`AssignDialogProc`, its own eight globals, and exactly one public entry
point. Clean to isolate because nothing else in the app needs its
internals — only the one function `gui_common.h` declares.

**Key types/globals.** `AssignChoice` (confirmed/studentName/category/
period — the dialog's result). All eight of `g_assignResult`,
`g_assignListBox`, the six radio-button handles, and `g_assignDialogOpen`
stay private to this file.

**Key functions.** `AssignDialogProc` (the dialog's own message loop —
runs its own nested `GetMessageW`/`IsDialogMessageW` loop rather than
returning to the main message loop, standard modal-dialog pattern);
`showAssignDialog` (builds the dialog's native controls — listbox,
radio-button groups for category and period); `applyFileAssignment` (the
rename); `deleteUnclassifiedFile`; `handleUnclassifiedAction` — the one
function exposed via `gui_common.h`, called identically from both the
mouse-click handler and `activateFocused()`'s Enter-key path so the two
input methods can never drift apart.

**Depends on:** `gui_common.h` (for `showAppModal`/`showToast`,
`g_analysisResults`, `s2w`); `audit_log.h`'s `audit_log()` for logging
manual assignments.

**Depended on by:** `gui_overview.cpp`'s `handleOverviewClick` (the
Unclassified-file Assign/Delete row clicks on the Overview tab call
`handleUnclassifiedAction`).

---

### `gui_help_carousel.cpp`

**Purpose.** The multi-step "CALSS Quick Start Guide" walkthrough overlay
— ten steps, each a screenshot with annotation rings pointing at specific
UI elements, navigated via Next/Prev/Skip/dot-row.

**Why this boundary.** A visually and behaviorally self-contained overlay
system, structurally similar to `AppModal` (same card/scrim/"draw after
the main blit" conventions) but with different enough semantics (multi-
step position, per-step image+rings, Skip/Next/Done rather than
primary/secondary accept/cancel) that it was deliberately built as its
own system rather than shoehorned into `AppModal`'s shape. Mutually
exclusive with `AppModal` by construction (`openHelpCarousel` no-ops if a
modal is already open).

**Key types/globals.** `RingSpec` (one annotation ring's position + optional
label tag); `CarouselStep` (title/caption/image filename/rings — the
`HELP_STEPS[10]` static table is the entire walkthrough's content,
data-driven rather than hardcoded per-step drawing code).
`g_helpCarouselState`/`g_helpCarouselStep`/`g_helpCarouselFocusOnNext` are
the three globals promoted to `gui_common.h`; the image bitmap cache
(`g_helpStepBitmaps[10]`) and the five hit-rect globals
(`g_helpCarouselCloseRect` etc.) stay effectively file-owned even though
some are technically declared non-`static` for the `extern` contract.

**Key functions.** `openHelpCarousel`/`closeHelpCarousel`/
`helpCarouselGoToStep`/`helpCarouselNext`/`helpCarouselPrev`/
`helpCarouselSkip` (navigation); `helpStepBitmap` (lazy-loads and caches
each step's PNG from an exe-relative `assets\help\` folder, resolved once
via `GetModuleFileNameW` rather than the working directory, which shifts
depending on launch method); `drawHelpCarouselRing` (the annotation ring's
two-phase animation: a 400ms clockwise "draw-in" sweep via
`GraphicsPath::Flatten` + cumulative-length walking — GDI+ has no native
partial-path-stroke primitive — then a continuous alpha-pulse loop, the
one place in the app allowed to animate indefinitely); `drawHelpCarousel`
(the main draw entry point, called from `ContentProc`'s `WM_PAINT`
alongside `drawToasts`/`drawAppModal`).

**Depends on:** `gui_common.h`; GDI+ (`Gdiplus::Bitmap`,
`GraphicsPath::Flatten`) for both the screenshot rendering and the ring
sweep animation.

**Depended on by:** `gui.cpp`'s `handleMenuCommand` (Help → Quick Start
Guide calls `openHelpCarousel`), `ContentProc` (calls `drawHelpCarousel`
every paint, `closeHelpCarousel`/navigation on the relevant key/click
events).

---

### `gui_welcome.cpp`

**Purpose.** Three things bundled together because they share state: the
idle/welcome screen (empty state with three "load submissions" option
cards, or the live analysis-progress display once a run starts), the
startup splash screen, and the background analysis thread whose live
phase/progress state the welcome screen renders. `drawWelcomeScreen`
*is* the analysis-progress display — it reads
`g_analysisPhase`/`g_analysisPhaseCurrent`/`g_analysisPhaseTotal`, which
only `analysisThread()` writes. Keeping producer and consumer in one file
keeps those three globals `static` (fully private) rather than needing to
join the `gui_common.h` contract.

**Key types/globals.** `g_analysisCancelRequested`
(`std::atomic<bool>`, the one global here promoted to the shared
contract — the cooperative-cancellation flag the pairwise-comparison loop
checks). `g_analysisPhase`/`g_analysisPhaseCurrent`/`g_analysisPhaseTotal`
stay `static`.

**Key functions.**
- `analysisThread` — runs on a `std::thread`, registers a progress
  callback (via `setAnalysisProgressCallback`, declared in `gui_common.h`
  reaching into `main.cpp`) that updates the three progress globals and
  `PostMessage`s `WM_ANALYSIS_PROGRESS` back to the main window, then
  calls `runAnalysisPipeline` and finally posts `WM_ANALYSIS_COMPLETE`.
  This is the one function that actually invokes the analysis pipeline in
  the running GUI app.
- `drawWelcomeScreen` — three states in one function: analysis running
  (phase chain + determinate progress bar + Cancel button), data loaded
  but not yet analyzed ("N submissions ready"), or the empty state (three
  option cards: Select a folder / Import files / Sync Classroom).
- `showSplashScreen` — a separate top-level popup window with its own
  inline `WNDPROC` lambda, shown before the main window exists, animating
  a loading bar for ~1.8s. Caches its offscreen GDI buffer as `static`
  across repaints (created once, reused every animation frame) rather
  than reallocating per frame — a documented perf fix.
- `getULLoadingLogo` — loads `ulcclogoloadingscreen.png` (a separate asset
  from the in-app sidebar seal), falling back to a drawn gold circle if
  the file is missing.

**Depends on:** `gui_common.h`; `main.cpp`'s `runAnalysisPipeline`/
`setAnalysisProgressCallback` (via the `extern` contract).

**Depended on by:** `gui.cpp`'s `onRunAnalysis` (starts `analysisThread`
on a detached `std::thread`); `ContentProc`'s `WM_PAINT` (calls
`drawWelcomeScreen` whenever no analysis is complete yet);
`WindowProc`/`runGUI` (calls `showSplashScreen` at startup).

---

### `gui_overview.cpp`

**Purpose.** The Overview tab: the hero/secondary stat cards (with their
once-per-run count-up animation), the Unclassified-files and
language-mismatch notices, and the AI summary box with its inline
"Pair N" deep-links into Flagged Pairs.

**Why this boundary — Checkpoint 1 of the ContentProc split.** The first
of `ContentProc`'s three tabs to be extracted. Assembled from four
previously-scattered regions of `gui.cpp`; `drawResultsHeader` and
`handleOverviewClick` were added in this same checkpoint as the pattern
established for every later tab split: a `handleXClick` function
consolidating that tab's `WM_LBUTTONDOWN` hit-testing into one
`bool`-returning entry point `ContentProc` calls at the point the first
of the original inline blocks used to sit.

**Key types/globals.** `PairRefMatch`/`TextPiece` (private — the inline
"Pair N" reference-detection state machine). `g_statsAnimStartTick`/
`g_statsAnimActive` (the count-up animation trigger, set exactly once per
analysis run from `WM_ANALYSIS_COMPLETE`) and `g_aiSummaryLinkRects` are
the three globals promoted to the shared contract.

**Key functions.**
- `drawStatsDashboard` — hero row (Pairs flagged / High similarity / Max
  score, 44px numerals, severity-colored left accent, staggered count-up
  via `statCountupProgress`) + secondary row (Total pairs / Exact
  duplicates / Average) + the two attention notices.
- `findPairReferences` — scans AI-generated finding text for "Pair 7" /
  "pairs 1, 2, and 3"-style references (word-boundary-guarded so
  "pairwise"/"pairings" don't false-match) without any special prompt
  instruction asking for it — detected from the rendered prose directly.
- `layoutFindingText` — the shared measure/draw function for AI findings:
  word-wraps text within a max width, rendering detected pair-references
  as clickable gold-underlined numbers; called once with `draw=false` to
  measure total height, once with `draw=true` to actually render, so the
  two passes can never disagree about line breaks.
- `drawAISummary` — renders Tier 2's cross-pair findings as discrete
  bulleted rows (not one wrapped paragraph), or the skipped/failed
  fallback states.
- `drawResultsHeader` — the "ANALYSIS REPORT" header card with timestamp
  and data-folder path.
- `handleOverviewClick` — Exact-Duplicates-card click (deep-links to
  Flagged Pairs with the filter applied), AI-summary pair-reference
  clicks, language-notice "Show files" toggle, Unclassified Assign/Delete
  clicks (delegates to `gui_assign_dialog.cpp`'s
  `handleUnclassifiedAction`).

**Depends on:** `gui_common.h`.

**Depended on by:** `gui.cpp`'s `ContentProc` (calls `drawResultsHeader`+
`drawStatsDashboard`+`drawAISummary` for `PAGE_OVERVIEW`'s `WM_PAINT`, and
`handleOverviewClick` for its `WM_LBUTTONDOWN`).

---

### `gui_flagged.cpp`

**Purpose.** The Flagged Pairs tab — the largest split file (1,363
lines). Owns pair comparison cards (feature table, Deviations block,
authorship cards, AI evidence narrative), collapsed/expanded row
rendering with its 240ms expand/collapse animation, and the
severity/exact-duplicate filter chrome.

**Why this boundary — Checkpoint 2 of the ContentProc split.** `gui.cpp`
already had a "STAGE 5B — Flagged Pairs and All Pairs Table" section
comment marking most of this content as one contiguous block, extracted
verbatim. Nine additional private symbols that lived scattered elsewhere
in `gui.cpp` (near `g_pairSeverityFilter`/`g_dnaCellHits`/a duplicate
`scoreColor` helper) were found by compiling this file standalone and
resolving every "not declared in this scope" error — not by re-reading
the original audit, which only covered the contiguous block. Two globals
(`g_expandedPairs`, `g_pairSeverityFilter`) were promoted to
`gui_common.h` but their *definitions* stayed in `gui.cpp`, since code
there still needs them too — this file is the one split file that owns
zero of its own promoted globals.

**Key types/globals.** `FeatureRowCols` (shared column geometry so the
full feature table and the Deviations block's columns never drift apart);
`SEVERITY_CHIP_BAR_HEIGHT`/`PAIR_COLLAPSED_ROW_HEIGHT` (private layout
constants).

**Key functions** (selected — this file has ~20):
- `drawFeatureRow`/`drawFeatureTableHeader`/`drawFeatureCategory` — the
  full 14-feature comparison table, PROFILE/THIS mono-aligned columns
  plus a zero-centered delta bar per row.
- `drawDeviationsBlock`/`estimateDeviationsBlockHeight` — pulls just the
  *mismatching* features above the fold (spec: "the two rows that matter
  shouldn't be buried under twelve that don't"); the estimate function
  exists purely so viewport-culling math never has to actually draw.
- `drawAuthorshipCard`/`estimateAuthorshipCardHeight` — one student's full
  card: score, progress bar (width-animated on expand), reliability note,
  compact Style DNA strip, the Deviations block, and the "Show all 14
  features" disclosure.
- `drawComparativeBox` — the maroon-tinted "COMPARATIVE AUTHORSHIP
  ANALYSIS" box with its Copy-findings clipboard button.
- `resolvePairNarrativeText`/`drawPairNarrative` — the Tier-1 AI evidence
  note per pair, three-state (success/skipped/failed) handling mirroring
  `drawAISummary`'s pattern.
- `drawFlaggedPairCollapsedRow` — the 96px collapsed summary row: rank,
  score, names, period, severity badge, two Style DNA strips side by
  side, "N features deviate" (unioned across both sides).
- `drawFlaggedPairBlock`/`estimateFlaggedPairBlockHeight` — the fully-
  expanded pair.
- `drawFlaggedPairs` — the tab's top-level entry point: header, severity-
  filter chips, exact-duplicates filter banner, then every pair
  (viewport-culled, filtered, blended between collapsed/expanded heights
  during its in-flight animation via a `HRGN` clip region — GDI has no
  height-interpolated layout primitive, so clipping the fully-drawn
  content is the practical equivalent).
- `handleFlaggedPairsClick` — the six click targets on this tab
  (collapsed-row expand, expanded-header collapse, "Show all 14
  features" toggle, Copy findings, severity chip, clear-filter),
  consolidated from what were already contiguous `WM_LBUTTONDOWN` blocks
  in `gui.cpp` — unlike Checkpoint 1's Overview blocks, no reordering was
  needed here.

**Depends on:** `gui_common.h`.

**Depended on by:** `gui.cpp`'s `ContentProc` (`drawFlaggedPairs` for
`PAGE_FLAGGED`'s `WM_PAINT`, `handleFlaggedPairsClick` for its
`WM_LBUTTONDOWN`); `gui_overview.cpp`'s `handleOverviewClick`
(`jumpToFlaggedPair`, declared in `gui_common.h` but defined in `gui.cpp`,
deep-links here); `exportPairsToCsv` (defined in `gui.cpp`) reads
`g_expandedPairs`/severity data this file also touches.

---

### `gui_students.cpp`

**Purpose.** The Students/Classes tab: the block sub-tab bar, the student
card grid, and the per-student detail page (Style DNA strip, full feature
comparison, flagged-appearance deep-links).

**Why this boundary — Checkpoint 3 of the ContentProc split, the last of
the three tabs.** Three functions (`drawBlockTabBar`, `drawStudentProfiles`,
`drawStudentDetailPage`) were already `extern`-prototyped in
`gui_common.h` but still physically defined in `gui.cpp`, non-contiguous
with each other (shared chrome functions `drawTabBar`/`drawContextStrip`
sat between two of them in the original file) — moved here verbatim, each
cut separately. Four *new* functions then consolidated this tab's logic
that was scattered across **four different `ContentProc` handlers**, two
of which were easy to miss because their Students-specific chunk sat
outside that handler's normal per-page dispatch: `handleStudentsClick`
(WM_LBUTTONDOWN's four Students blocks — block sub-tabs, sort control,
flagged-appearance rows, card/back/report), `drawStudentsPage`
(WM_PAINT's dispatch, including button-rect setup for the detail view),
`handleStudentsDetailMouseMove` (WM_MOUSEMOVE's DNA-hover chunk, mixed
into an otherwise page-agnostic handler), and `drawStudentDnaTooltip`
(WM_PAINT's DNA hover tooltip, drawn in the shared-overlay section *after*
the main per-page dispatch, not inside it). `g_dnaTooltipPos` (previously
`static` in `gui.cpp`) was the one global promoted to the shared contract
for this checkpoint, needed because `handleStudentsDetailMouseMove` and
`drawStudentDnaTooltip` now live in different files but both touch it.

**Key types/globals.** `StudentFlagSummary` (per-student flagged-
appearance count + max score, computed for the card footer).
`g_blockTabNames`/`g_backButtonRect` are the two globals defined here
(promoted, since the original audit only found three functions as the
extern surface — actual closure needed two globals too, discovered the
same "compile standalone, resolve every error" way as the Flagged Pairs
checkpoint).

**Key functions.**
- `determineStudentBlock` — resolves which Classroom section/"block" a
  student belongs to, by finding one of their files in the sync manifest.
- `drawBlockTabBar` — the "All" + one-pill-per-block sub-tab row, fixed
  position so it stays visible while the grid below scrolls.
- `drawStudentProfileCard`/`drawStudentProfiles` — the card grid: name,
  submission count, compact Style DNA strip, condensed style notes, and a
  flagged-appearance footer (the signal the *old* card design was
  missing — you previously had to open a student to discover they
  appeared in eight flagged pairs). Grouped by block, sortable by
  name/submissions/flagged-count, live-filtered by the search box.
- `drawStudentDetailPage` — the per-student detail: full Style DNA strip,
  every style note, and a Flagged Appearances list that deep-links to
  Flagged Pairs (auto-expanding the target pair).
- `handleStudentsClick`/`drawStudentsPage`/`handleStudentsDetailMouseMove`/
  `drawStudentDnaTooltip` — the four consolidated dispatcher functions
  described above.

**Known pre-existing bugs, deliberately untouched by this split** (see
project `CLAUDE.md`): `g_studentCardRects` stores its `y` without the
`+g_scrollY` convention every other hit-rect vector in the app uses, so
student-card clicks mis-hit once the grid is scrolled; `g_backButtonRect`
is set from a fixed reference point in `WM_PAINT` but the button draws at
a scroll-adjusted position, so they drift apart once the detail page is
scrolled. Both are flagged inline in the source and left alone per the
project's "pre-existing bugs stay untouched unless explicitly asked"
discipline.

**Depends on:** `gui_common.h`; `google_classroom.h`'s
`gc_loadSyncManifest` for block grouping.

**Depended on by:** `gui.cpp`'s `ContentProc` (`drawStudentsPage`/
`handleStudentsClick` for `PAGE_STUDENTS`'s `WM_PAINT`/`WM_LBUTTONDOWN`,
`handleStudentsDetailMouseMove`/`drawStudentDnaTooltip` for
`WM_MOUSEMOVE`/the shared-overlay `WM_PAINT` section).

---

### `gui.h`

**Purpose.** The GUI's entire public surface, as seen from outside the
GUI subsystem: one function.

**Key functions.** `runGUI(HINSTANCE hInstance)` — initializes and runs
the whole application, returns the message-loop exit code.

**Depends on:** `<windows.h>`, `<string>` (all `_WIN32`-gated — the
declaration compiles away entirely on a non-Windows build).

**Depended on by:** `main.cpp` (the only external caller — `main()` calls
`runGUI(GetModuleHandle(nullptr))` when launched with no arguments);
`gui_common.h` includes it too (transitively pulling in `<windows.h>` for
every split file, which is the actual reason it's included there rather
than for `runGUI` itself — no split file calls `runGUI`).
