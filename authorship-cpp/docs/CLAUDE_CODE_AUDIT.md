# Claude Code Audit — 2026-09-17

Read-only ground-truth pass against the real repository (not a static
export). Every claim below marked **Verified** was checked by reading the
actual file/line cited or by running the actual toolchain; **Inferred**
means derived from call sites rather than from reading the callee's own
implementation. Where this audit disagrees with a prior AI-written
document that only had headers to work from, this audit is the source of
truth — but see each item for exactly what changed.

No code was modified in this session.

---

## 1. The five open questions from the prior (stale) document

The prior document was built from a static export that predates the
current repo state. All five of its open questions resolve the same way:
**the thing it worried about does not exist in the current repo.**

1. **"`compile.bat` is missing `gui_welcome.cpp`/`gui_overview.cpp`."**
   False as of this repo. `compile.bat` (repo root,
   `authorship-cpp/compile.bat`) lists all 9 translation units — `main.cpp`,
   `tokenizer.cpp`, `features.cpp`, `knn_model.cpp`, `similarity.cpp`,
   `gemini.cpp`, `google_classroom.cpp`, `audit_log.cpp`, `gui.cpp` — plus
   all 7 GUI split files including both `gui_welcome.cpp` and
   `gui_overview.cpp`. **Verified** by reading the file directly.

2. **"`gui_common.cpp` is documented but doesn't exist."**
   Correct that it doesn't exist — but this isn't a discrepancy, it's a
   documented, deliberate decision. `gui_common.h`'s own top-of-file
   comment (lines 20–41) states no `gui_common.cpp` exists and explains
   where each of the 72 shared globals is actually defined instead
   (56 in `gui.cpp`, 16 spread across `gui_help_carousel.cpp` (8),
   `gui_sync_sheet.cpp` (4), `gui_overview.cpp` (3), `gui_welcome.cpp` (1)).
   `CLAUDE.md` documents the same decision as made 2026-08-24 and "still
   final." **Verified** by reading the header comment directly (see §5 for
   one small tally discrepancy inside that same comment).

3. **"`gui_flagged.cpp`/`gui_students.cpp` don't exist; those screens are
   still in `gui.cpp`."**
   False. Both files exist: `gui_flagged.cpp` (1,363 lines) and
   `gui_students.cpp` (1,048 lines), both wired into `compile.bat`.
   **Verified** — file listing + line counts below.

4. **"Line counts have drifted before, don't trust old numbers."**
   Correct caution, and the current numbers (measured fresh this session)
   match what `CLAUDE.md`'s "Current state" table already claims exactly,
   file for file. See the table in §2.

5. **"Full `.cpp` implementations were never available — only headers."**
   Addressed by this audit: every `.cpp` file in `src/` was read in full
   this session (see §3).

**Bottom line: the prior document's concerns describe a repo state that
does not exist here.** Nothing needs fixing in response to items 1–3;
they were never broken in this repo.

---

## 2. Current file inventory (fresh measurement, 2026-09-17)

Build command actually run — `x86_64-w64-mingw32-g++ 16.1.0`, the exact
file list from `compile.bat`, from a clean invocation (not `compile.bat`
itself, but byte-identical arguments):

```
x86_64-w64-mingw32-g++ -std=c++17 -static-libgcc -static-libstdc++ -O2 \
    src/main.cpp src/tokenizer.cpp src/features.cpp src/knn_model.cpp \
    src/similarity.cpp src/gemini.cpp src/google_classroom.cpp \
    src/audit_log.cpp src/gui.cpp src/gui_sync_sheet.cpp \
    src/gui_assign_dialog.cpp src/gui_help_carousel.cpp \
    src/gui_welcome.cpp src/gui_overview.cpp src/gui_flagged.cpp \
    src/gui_students.cpp -I include/ \
    -lwinhttp -lgdi32 -lcomdlg32 -lole32 -lshell32 -lcomctl32 -lws2_32 \
    -lmsimg32 -lgdiplus -o authorship.exe
```

**Result: zero errors, zero warnings on stderr, clean link.** Output
binary: 3,902,713 bytes — identical size to the `authorship.exe` already
committed at repo root, confirming this is genuinely the same build
`compile.bat` produces, not a coincidentally-successful different one.
**Verified** by actually running the compiler, not by reading
`compile.bat` and assuming.

| File | Lines | Owns |
|---|---:|---|
| `src/gui.cpp` | 3,981 | Window chrome, `ContentProc` dispatcher, cross-screen primitives |
| `src/main.cpp` | 2,474 | Pipeline entry point, HTML report, filename/category parsing |
| `src/gui_flagged.cpp` | 1,363 | Flagged Pairs tab |
| `src/gui_sync_sheet.cpp` | 1,045 | Google Classroom sync modal |
| `src/gui_students.cpp` | 1,048 | Students/Classes tab |
| `src/google_classroom.cpp` | 960 | OAuth + Classroom API + submission download |
| `src/gui_overview.cpp` | 785 | Overview tab |
| `src/gemini.cpp` | 754 | Gemini API (legacy + tiered) |
| `src/gui_help_carousel.cpp` | 646 | Quick Start Guide overlay |
| `src/gui_welcome.cpp` | 505 | Idle screen, splash, analysis thread |
| `src/features.cpp` | 468 | 14-feature stylometric extraction |
| `src/gui_assign_dialog.cpp` | 348 | Unclassified-file assignment dialog |
| `src/tokenizer.cpp` | 361 | Tokenization + Jaccard/LCS |
| `src/knn_model.cpp` | 193 | KNN authorship scorer |
| `src/audit_log.cpp` | 170 | Audit log + SHA-256 |
| `src/similarity.cpp` | 137 | Pairwise similarity engine |
| `include/gui_common.h` | 622 | Shared GUI contract |
| `include/results_data.h` | 163 | GUI-facing display structs |
| (+ 8 more small headers) | | |

**Every file `CLAUDE.md`'s "Current state" table and `docs/ARCHITECTURE.md`
describe was confirmed present, at the stated line count, doing the
stated job.** `docs/ARCHITECTURE.md` (committed one commit before HEAD,
`1569a9a`) is itself a 963-line file-by-file reference that already
covers essentially everything this audit's item 2 (below) was asked to
verify — this audit independently re-derived the same facts by reading
the `.cpp` bodies directly (not by reading that doc first) and found it
accurate everywhere it was checked against actual code. Treat
`docs/ARCHITECTURE.md` as current and correct; this document adds the
things it doesn't cover (exact formulas/weights, dead code, one stale
tally) rather than replacing it.

---

## 3. Algorithm internals — verified from the actual `.cpp` bodies

### Tokenization + token similarity (`src/tokenizer.cpp`)

- Cleaning pipeline, in order: strip `/* */` block comments (line count
  preserved via injected `\n`) → strip `//` line comments → replace string
  literals with the sentinel `"STR"` and char literals with `'C'` → strip
  preprocessor lines (`#include`, `#define`, etc., replaced with a blank
  line to preserve line numbers). (`tokenize()`, lines 249–280.)
- Token normalization: C keywords/stdlib names (`printf`, `scanf`,
  `malloc`, etc. — a fixed list, `tokenizer.cpp:18–27`) pass through
  unchanged; every other identifier is replaced with `VAR_0`, `VAR_1`, ...
  in first-seen order; numeric literals become `NUM`; the `"STR"`
  sentinel becomes `STR`. This is what makes token similarity
  variable-name-agnostic. (`normalizeTokens()`, lines 187–220.)
- **Jaccard similarity**: `|intersection| / |union|` over the normalized
  token *sets* (order/frequency discarded). Empty-union edge case returns
  `1.0`. (`jaccardSimilarity()`, lines 282–298.)
- **LCS similarity**: `2 * LCS_length / (lenA + lenB)`, i.e. a Dice-style
  normalization of longest-common-subsequence length. The LCS DP is
  space-optimized (two rolling rows, not a full matrix) and **caps each
  side's token count at 500** for performance — beyond 500 tokens, LCS is
  computed only over each sequence's first 500 tokens.
  (`lcsLength()`/`lcsSimilarity()`, lines 223–243, 300–307.)
- **Combined token score**: `0.40 * jaccard + 0.60 * lcs` — LCS is
  weighted higher because it's order-sensitive (structural copying, not
  just "uses the same keywords"). (`computeSimilarity()`, line 313.) This
  matches what `CLAUDE.md`/`docs/ARCHITECTURE.md` state; confirmed against
  the literal constants, not inferred from a comment.
- Classification thresholds (`classifySimilarity()`, lines 321–328, used
  for both token-only and final combined scores): ≥95% Exact Duplicate,
  ≥80% High, ≥60% Moderate, ≥40% Low, else Minimal.

### Pairwise combined score (`src/similarity.cpp`)

- `analyzePair()` (lines 33–77) is the actual per-pair entry point used by
  `main.cpp` (not the batch `analyzeAll()` — see §4, dead code).
- Token half: `tokenize()` both files → `computeSimilarity()` → `* 100`
  for a percentage.
- Style half: `extractFeatures()` both files → `styleSimilarity()` on the
  normalized 14-vectors → `* 100`. Only computed if both extractions
  succeeded and both normalized vectors are non-empty; otherwise style
  contributes `0.0`.
- **Final combined score: `tokenPct * 0.60 + stylePct * 0.40`** — i.e.
  the top-level pair score weights token similarity higher (60/40), which
  is the *opposite* emphasis from the token-internal Jaccard/LCS split
  (40/60 favoring LCS *within* the token score). Two different 60/40
  splits exist in this codebase at two different levels — worth being
  precise about which one a reader means. (Line 58.)
- **Flag threshold: `combined >= 75.0`** (percentage scale, i.e. 75%).
  (Line 73.) `main.cpp`'s pipeline uses this same `analyzePair()` — the
  75% threshold is not duplicated/re-implemented elsewhere.

### KNN authorship scoring (`src/knn_model.cpp`)

- **Distance metric: plain Euclidean distance** over the 14-dimensional
  normalized (0–1) feature vectors — `sqrt(sum((a_i - b_i)^2))`.
  (`euclidean()`, lines 23–30, reused as `features.cpp`'s
  `euclideanDistance()` for the pairwise style score — same formula, two
  call sites.)
- **Neighbor selection**: for a `score(student, file)` call, distance is
  computed from the test submission to *every* submission already in that
  student's profile (not to other students — this is a one-class
  per-student model, not a multi-class KNN classifier). The `useK =
  min(k_, submissions.size())` closest distances are averaged.
  Construction default is `k_ = 3` (`AuthorshipScorer(int k=3)` per
  `knn_model.h`; `CLAUDE.md`'s "K=3" claim is confirmed here, not just in
  a header comment). (Lines 78–147.)
- **Score conversion**: `rawScore = max(0, 1 - avgDist/maxDist)` where
  `maxDist = sqrt(FEATURE_COUNT)` (the theoretical max Euclidean distance
  between two vectors that are each individually bounded in `[0,1]` per
  dimension — `sqrt(14) ≈ 3.742`).
- **Confidence boost**: `conf = min(1.0, 0.75 + submissions.size() * 0.05)`,
  applied multiplicatively: `finalScore = rawScore * conf`. A 1-submission
  profile caps at `0.80` confidence; a 5+-submission profile reaches the
  `1.0` ceiling. This is a genuine algorithmic step, not just a display
  label — it changes the numeric score, not merely the "reliability"
  badge text (`getReliability()` in `main.cpp:933–946` is a *separate*,
  purely cosmetic High/Moderate/Low label derived from `profileSize`
  alone — the two systems don't share a formula, they're two independent
  presentations of "how much data backs this score").
- Classification thresholds (`classifyAuthorship()`, lines 15–21): ≥80%
  High Likelihood, ≥60% Moderate, ≥40% Low, else Very Low.
- `scorePair()` picks whichever of the two students has the *lower*
  authorship score as `likelyCopier` — a tie (`>=`) favors calling A the
  original. This is used (confirmed live call sites: `main.cpp:630` and
  `main.cpp:2121`), not dead code, despite not being wired into the GUI's
  own display language (the GUI's own "likely original/copier" framing is
  described in `docs/ARCHITECTURE.md`; this audit did not re-verify every
  GUI display string against it).

### `describeStyle` / `compareStyles` (`src/features.cpp`)

- `describeStyle(vec)` (lines 375–411) takes one 14-vector and returns
  human-readable one-line `StyleNote`s per dimension group — naming
  length (short/long/medium via `vec[0]` at 2/8 char thresholds),
  single-letter-variable heaviness (`vec[1] >= 0.5`), camelCase vs
  snake_case dominance (`vec[2]`/`vec[3]` at 0.4), comment frequency
  (`vec[4]` at 0/0.05/0.20), bracket placement (`vec[6]` at 0.3/0.7),
  loop preference (`vec[7]` at 0.2/0.8), operator spacing (`vec[8]` at
  0.3/0.7), and blank-line frequency (`vec[10]` at 0.05/0.2). This is what
  populates a student's profile-level "style notes" — **not** a
  per-submission comparison; it describes one vector in isolation.
- `compareStyles(test_vec, profile_vec)` (lines 413–469) is the actual
  per-submission-vs-profile comparison that feeds every `[MATCH]`/
  `[MISMATCH]` line the GUI and the Gemini prompts show. It runs a fixed
  table of 14 checks (`Check checks[]`, lines 425–443) — **not** all 14
  features use the same tolerance threshold; each has its own
  hand-tuned `threshold` (e.g. variable-name length tolerance `1.5`
  characters, camelCase ratio tolerance `0.15`, bracket placement `0.30`,
  loop preference `0.25`). A feature is `[MATCH]` if
  `|test_vec[i] - profile_vec[i]| <= threshold`, else `[MISMATCH]`. The
  function returns one string per check plus a trailing "`N matched / M
  inconsistent`" summary line — this exact list of strings (with
  `[MATCH]`/`[MISMATCH]` tags left in) is what `gemini.cpp`'s legacy
  `buildPrompt()` sends to the model verbatim (line 197–199, 211–213),
  while the newer tiered path re-parses these same strings back into
  structured `StyleNoteDisplay` fields in `main.cpp` (`parseStyleNote()`,
  seen around line 920) before re-formatting them for the Gemini prompt —
  i.e. the string format gets serialized then re-parsed rather than the
  structured data being threaded through directly. Not a bug (it works),
  but a real inefficiency/indirection worth knowing about if this code is
  touched again.

### Gemini integration (`src/gemini.cpp`)

- **No JSON library** — `jsonEscape`/`extractTextFromJson`/
  `parseJsonStringLiteral`/`extractJsonStringArray` are all hand-rolled
  string scanning. `extractTextFromJson` specifically finds the *first*
  `"text"` field after the *first* `"parts"` in the response — this only
  works because Gemini's response shape is known/fixed; it is not a
  general JSON parser and would silently return an empty string on any
  unexpected response shape (handled as "Could not parse API response",
  not a crash).
- **Model endpoint**: `gemini-2.5-flash-lite` via
  `generativelanguage.googleapis.com/v1beta/models/...:generateContent`,
  API key passed as a URL query parameter (`?key=...`), not a header.
  (Line 261.)
- **Three separate prompt-building functions, three separate call sites**,
  not one prompt reused three ways:
  1. `buildPrompt()` (legacy, still wired — see below) — one big prompt
     covering *every* flagged pair at once, asking for a 4-section
     instructor briefing (Priority Cases / Pattern Analysis / Style
     Inconsistency Details / Recommended Actions) as free-form text,
     capped at "under 400 words total" by prompt instruction (not
     enforced programmatically). Explicit rules baked into the prompt:
     never name a "cheater"/"copier", no definitive accusations, but *do*
     name the student whose submission shows lower style consistency as
     "worth investigating." (Lines 137–221.)
  2. `buildPairNarrativePrompt()` (Tier 1) — batches *all* flagged pairs
     into one call, asks for a JSON array of one 2–3-sentence narrative
     per pair, order-matched to the input vector. Explicitly told to
     treat Layout-category deviations (comments/spacing) as weaker signal
     than Lexical+Syntactic combinations, and to flag reduced confidence
     when a student's reliability is Low/Moderate. Response is validated
     for array-length match against `flagged.size()` — a mismatch is
     treated as a hard failure for *all* pairs in that call (line 682–689:
     `setAll(false, false, ...)`), not a partial-success fallback.
  3. `buildCrossPairPrompt()` (Tier 2) — cross-pair pattern detection
     (shared deviating-feature clusters vs. independent incidents), only
     invoked when ≥2 pairs are flagged (`generateCrossPairPatterns()`
     line 709 early-returns `skipped=true` below that). Asks for 2–4
     JSON-array findings, not free text.
  - All three explicitly instruct the model never to use the words
    "cheated"/"plagiarized".
- **Rate-limit/failure handling**: no retry logic anywhere in this file —
  a failed HTTP call, a non-200 status, an empty response, or a
  JSON-array-length mismatch all just set `success=false` with a
  descriptive `error` string and return immediately. The GUI is
  responsible for rendering that failure state; nothing in `gemini.cpp`
  itself waits, backs off, or retries.
- **The legacy path is still live, not dead code**: `main.cpp` calls
  `generateAISummary()`/`buildPrompt()` at two separate sites (lines
  ~2201 and ~2434 — both inside `runAnalysisPipeline`'s branches), *and*
  also calls the newer `generatePairNarratives()`/
  `generateCrossPairPatterns()` (line ~2220/2222). This confirms
  `docs/ARCHITECTURE.md`'s claim that both generations run side by side
  rather than the legacy one being vestigial — verified by direct grep of
  call sites, not inferred from the header.

### Google Classroom OAuth (`src/google_classroom.cpp`)

- **Token storage**: a flat JSON file (`GC_TOKEN_FILE`, gitignored) with
  `access_token`/`refresh_token`/`expires_at` (absolute Unix timestamp,
  not a duration). Written by `saveToken()` (lines 125–132) via raw
  `<<` string concatenation — not JSON-escaped. If a token or client
  secret ever contained a literal `"` this would produce invalid JSON;
  in practice these are opaque base64/URL-safe tokens from Google, so
  this isn't a realistic failure mode, just worth noting as a latent
  fragility if the token format ever changes.
- **Validity check** (`gc_hasValidToken()`, line 134): `now < expiresAt -
  60` — a 60-second safety margin before actual expiry, not just
  `now < expiresAt`.
- **Refresh flow** (`gc_refreshIfNeeded()`, lines 391–416): only attempts
  a refresh if the current token is *not* already valid and a
  `refreshToken` is present; POSTs `grant_type=refresh_token` to
  `oauth2.googleapis.com/token`; on success, overwrites `accessToken` and
  `expiresAt` and re-saves — **the refresh token itself is never rotated
  or re-saved** (Google's response for this grant type normally doesn't
  return a new refresh token anyway, so this is standard, not a bug).
- **Every API call implicitly refreshes**: `getAccessToken()` (lines
  568–572) is the single choke point every `gc_get*`/`gc_download*`
  function calls through — it loads the token if not already loaded, and
  calls `gc_refreshIfNeeded()` whenever the current token isn't valid,
  before returning the access token string. There is no separate
  "ensure I'm logged in" step the GUI has to call — every API function
  self-heals an expired token transparently, one HTTP round-trip before
  the real request.
- **Scope-downgrade self-healing**: if a Classroom user-profile lookup
  (`/v1/userProfiles/{userId}`) returns 401/403 — meaning the previously
  granted token predates the roster-read scope being added to this app —
  `gc_downloadSubmissions()` calls `gc_signOut()` (clears the token file
  entirely) so the *next* sync attempt is forced through a fresh consent
  screen that will include the missing scope. This happens at most once
  per sync call (`scopeWarningShown` guard) and does **not** abort the
  in-progress sync — files still download using the ID-derived fallback
  name (`Student_<last-8-chars-of-userId>`) for the rest of that run.
  (Lines 844–855.)
- **Category/period inference priority hierarchy** (lines 705–769,
  `gc_downloadSubmissions`) — confirmed exactly matching
  `docs/ARCHITECTURE.md`'s description:
  1. Period is detected first, checked in the specific order
     `semifinal → midterm → final → prelim`, with `"semifinal"` checked
     *before* `"final"` specifically because `"semifinal"` contains
     `"final"` as a substring (an explicit inline comment flags this).
  2. Type is detected independently of period: `quiz` → `activity`/
     `seatwork`/`lab` (with an attempt to extract a trailing digit for
     `"Activity 2"` → `"activity2"`) → `exam` (matched either by the
     literal word "exam" *or* by period being non-empty with no other
     type keyword — i.e. a title that's just "Finals" defaults to being
     treated as that period's exam).
  3. `categoryTag = period + type` (e.g. `"midtermexam"`,
     `"finalactivity2"`) becomes the filename prefix before the `"__"`
     delimiter, plus a 6-character alphanumeric fingerprint derived from
     the Classroom `assignmentId` to disambiguate same-category
     assignments from different real assignments.
  - This is genuinely a two-signal encoding (period *and* type both
    captured), not just a type classifier — the explicit rationale in the
    code (lines 710–716) is that comparing a Midterm Exam against a Final
    Exam would be "statistically meaningless noise" even though both are
    "exam" type, which is why `main.cpp`'s pairwise comparison loop groups
    by period (`extractPeriod()`) *in addition to* filtering by category
    (`categorizeFile()`).

### Audit log (`src/audit_log.cpp`)

- Self-contained SHA-256 (public-domain reference algorithm, in an
  anonymous namespace) — no crypto library dependency, consistent with
  the project's "no third-party runtime deps" constraint.
- Log path resolves to `%PROGRAMDATA%\CALSS\audit.log` via
  `SHGetFolderPathA(CSIDL_COMMON_APPDATA)`, creating the directory if
  needed, falling back to the local working folder (`AUDIT_LOG_FILE`,
  the `.h`'s `#define`) only if `ProgramData` is genuinely unavailable.
  Appends are timestamped (`YYYY-MM-DD HH:MM:SS`, local time) with each
  detail line indented two spaces.

---

## 4. Additional findings not in the prior document

- **`analyzeAll()` (`similarity.cpp:79–108`) is dead code.** Grepped
  every `.cpp` in `src/` for call sites — none. `main.cpp` builds its own
  period-grouped pairwise loop directly against `analyzePair()` instead
  (as `docs/ARCHITECTURE.md` already correctly notes), which makes the
  batch convenience function `analyzeAll()` unreachable from any current
  code path. `flaggedPairs()` and `summaryStats()` (same file) **are**
  both still called from `main.cpp` (lines ~2096–2097, ~2395–2396) — only
  `analyzeAll()` itself is unused. Low-stakes (137-line file, ~30 lines of
  dead code), flagged per instructions rather than removed.
- **`gui_common.h`'s own shared-globals tally is off by one.** The
  top-of-file comment (lines 13, 22) — updated by the most recent commit,
  `7f3eca8`, specifically titled "Fix stale globals tally" — states "72
  cross-screen globals." A direct count of `^extern ` declarations that
  are variables rather than function prototypes (i.e. excluding the 3
  `extern` function declarations reaching into `main.cpp`) comes to
  **73**, not 72. This is a minor discrepancy in a comment that was *just*
  fixed for exactly this kind of drift, so it's worth a fresh manual
  recount rather than trusting either number blindly — automated
  line-based counting is fragile against multi-line declarations
  (`runAnalysisPipeline`'s prototype spans two lines, for instance) and
  could itself be off. Flagged, not corrected.
- **`saveToken()` (`google_classroom.cpp:125–132`) doesn't JSON-escape
  its output.** Noted above in §3; realistically inert given Google's
  token formats, but a latent fragility if that ever changes.
- **No retry/backoff anywhere in the Gemini integration.** A transient
  network hiccup or a rate-limit response surfaces as a hard failure to
  the GUI on the first attempt — by design per the code (no retry loop
  exists to remove), just worth having explicit for anyone evaluating
  robustness for the thesis writeup.
- **`docs/ARCHITECTURE.md` is already accurate and current.** Worth
  stating plainly: this audit's independent read of every `.cpp`
  implementation did not surface any correctness error in that document
  (only the tally note above, which is inside `gui_common.h` itself, not
  `ARCHITECTURE.md`). It was evidently written by reading the real
  implementations, unlike the prior document this session started from.

---

## 5. What this audit did *not* re-verify

- GUI-layer behavior claims in `docs/ARCHITECTURE.md` (click handlers,
  paint order, animation timing, the two known pre-existing bugs in
  `gui_students.cpp`) were read but not independently re-derived line by
  line the way the pipeline/integration files were — that document's own
  detail level there already looked reliable and matching what a skim of
  `gui.cpp`/`gui_students.cpp` showed, but this audit's primary focus per
  the task brief was the non-GUI `.cpp` implementations.
- The actual live behavior of a real Gemini API call or a real Google
  OAuth round-trip was not exercised (no network calls made this
  session) — everything above about those integrations is verified from
  reading the code that constructs/parses the requests and responses,
  not from observing a live call succeed.
- `api_key.txt`, `calss_gc_settings.json`, `calss_gc_token.json`,
  `calss_gc_embedded_credentials.json` were confirmed to exist on disk
  and confirmed gitignored (`.gitignore` at repo root lists all four
  explicitly) — their *contents* were not read, since they're plausibly
  live credentials and reading them wasn't necessary to answer any
  question asked.

---

## Questions for Rah

1. **`analyzeAll()` dead code** (§4) — remove it, or leave it as a
   documented public API of `similarity.h` for potential CLI/test use
   later? It's small, but it's the one clearly actionable item this audit
   found.
2. **The 72-vs-73 global tally** (§4) — want an exact manual recount
   committed to `gui_common.h`'s comment, or is "~72" close enough that
   it's not worth another commit purely for the number?
3. Should `docs/CLAUDE_CODE_AUDIT.md` (this file) stay as a standing
   document, or was it meant as a one-time sync-up to fold into
   `CLAUDE.md`/`ARCHITECTURE.md` and then be deleted? It duplicates some
   of `ARCHITECTURE.md`'s territory by design (independent verification),
   which could go stale relative to it over time if kept long-term.
4. No functional bugs were found in the pipeline/integration algorithms
   themselves during this pass (as opposed to the two already-known,
   already-documented GUI hit-rect bugs). Is there a specific subsystem
   you suspect has a real bug that prompted this audit, or was this
   purely a "get everyone's notes back in sync" pass?
