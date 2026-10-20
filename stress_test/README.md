# CALSS Synthetic Accuracy-Validation Dataset

## What this is (and isn't)

This is a **synthetic classroom dataset with known ground truth**, built to help you
document CALSS's actual detection accuracy — specifically the question you asked:
"how well does the stylometric pipeline work on small per-student sample sizes?"

This is **not** the large-scale O(n²) performance/timing stress test described in
`CALSS_StressTest_Prompt.md` (already in this project) — that one targets raw
pipeline speed at 800–900 exam files *in a single grading period*, run on your
actual machine with the real compiled app. This dataset is smaller and classroom-
realistic (145 students across 4 blocks), and its value isn't scale — it's that
**I know exactly which files are "normal," which are copied, and which have had
their style swapped**, so you can check CALSS's output against a known answer key
instead of eyeballing it.

I generated this in a Linux cloud sandbox — I don't have your compiled `CALSS.exe`,
a Windows environment, or Google Classroom access here, so I could not run this
through the real app myself. You run it locally (folder import, **not** Google
Classroom sync) and report back what CALSS outputs; I can then help you turn that
into the actual accuracy write-up for Chapter 4/5.

## Dataset shape

- 4 blocks: BSCS-3A (35), BSCS-3B (38), BSCS-3C (32), BSCS-3D (40) — **145 students total**
- 4 grading periods: Prelim, Midterm, Semifinal, Final
- Per student per period: 4 Activity submissions (profile-building) + 1 Exam submission
  (comparison-only) — **20 files per student, 2,900 files total**
- Per-period totals: 725 files each (580 activity + 145 exam)
- Filenames follow CALSS's real scheme exactly: `{period}{type}__{studentName}_ASGN{fingerprint}.c`
  (verified against `extractStudentName()`/`categorizeFile()`/`extractPeriod()` in `main.cpp`,
  and spot-checked by hand against that parsing logic before generating the full set)
- Every file was compiled with `gcc -fsyntax-only -std=c99` and confirmed to be
  syntactically valid C (2,900/2,900 pass) — CALSS's `checkLanguage()` heuristic should
  accept all of them as plausible C, none should land in `languageMismatchFiles`.

**How to load it:** point CALSS's folder import at this `stress_test` folder (it will
walk each block subfolder — matches the real two-level scan). Do not use Google
Classroom sync for this run. Block is picked up automatically from the subfolder name.

## What's normal vs. deliberately planted

See `manifest.csv` for the full per-file ground truth (columns: block, student,
period, category, problem, filename, scenario, ground_truth_note).

- **133 students (normal, ~92%):** fully self-consistent. Their own style (variable
  naming convention, brace placement, comment density, loop preference, operator
  spacing) is fixed per student and applied identically across all their Activity
  and Exam files. Their Exam solves the period's problem using one of two genuinely
  different structural approaches (e.g. "one combined loop" vs "three separate
  helper functions") assigned per student — so classmates solving the same exam
  problem don't look artificially identical just from sharing a template.
  **Expected result:** these students' own Exam-vs-own-profile authorship score
  should read Moderate–High, and they should mostly NOT appear in flagged pairs
  against each other (some natural baseline similarity is expected since they're
  solving the same problem, but not at flagged-pair levels).

- **7 copy pairs (14 students, ~10%):** two students — sometimes in the same block,
  sometimes across blocks (block doesn't matter, since CALSS's pairwise comparison
  groups only by period, never by block) — whose Exam file for one specific period
  is a near-verbatim copy of each other, with only a trivial cosmetic difference
  (one added comment line). **Expected result:** this pair should show up as a
  HIGH combined-similarity, FLAGGED pair (≥75%, likely ≥90%/"Exact Duplicate" range)
  in that period's Exam comparisons.

- **11 style-swap cases (~8%):** a single student whose Exam file, for one specific
  period, was rendered using a *different* (randomly chosen) student's style profile
  instead of their own — same problem, same correctness, but written with someone
  else's naming convention/brace style/comment habits/loop preference/spacing. Their
  own Activity files for that period are untouched and still reflect their real
  style. **Expected result:** this student's Exam-vs-own-profile authorship score
  should read Low/Very Low, even though nothing was copied from anyone and no
  pairwise flag is expected — this exercises Stage 2 (stylometric/KNN) independently
  of Stage 1 (token similarity).

Cross-reference `manifest.csv`'s `scenario` and `ground_truth_note` columns against
CALSS's actual output (flagged pairs list + each `AuthorshipDisplay.scorePct`) to get
real numbers: how many of the 7 injected copy pairs actually got flagged, how many of
the 11 style-swap cases actually scored Low/Very Low, and — just as important — how
many of the 133 normal students were *incorrectly* flagged or scored low (false
positives). That gives you an honest, if small-sample, precision/recall picture
instead of an unverifiable "it seems accurate."

## Honest limitations of this synthetic set

- It exercises most, but not all, of the 14 features distinctly. Naming convention
  (camelCase/snake_case/short), comment density, loop preference (for/while), and
  operator spacing all vary meaningfully by student. **ALL_CAPS constant usage,
  inline-comment ratio, blank-line ratio, and return-statement placement are left
  close to a fixed default across all students** — don't claim this dataset validates
  those specific sub-features without checking `features.cpp`'s actual formulas
  against what this generator does or doesn't vary.
- Structural diversity between students on the same exam problem is limited to two
  hand-written variants per problem, not open-ended — real student code varies more
  than that. Treat this as a lower bound on how well the system should perform, not
  an upper bound.
- This is a stress/accuracy test at 145 students, not the 800-900-per-period
  performance-scale scenario already planned separately — don't conflate the two
  when writing up results.

## Files in this folder

- `BSCS-3A/`, `BSCS-3B/`, `BSCS-3C/`, `BSCS-3D/` — the 2,900 `.c` submissions
- `manifest.csv` — full ground truth, one row per file
- `README.md` — this file
- The generator script that produced this (`generate.py`) is not included here;
  ask if you want it for regenerating with different parameters.

## Note: team test accounts

`BSCS-3D/` also contains submissions from five thesis-team test
accounts, synced from a test Google Classroom: `AMBEGUIAJansen_Rence_L`,
`EthanSandoval`, `PARASDivino_Dale_F`, `Rah`, `RanueeeOmega` (with their
`calss_sync_manifest.jsonl` lines). They were not generated, are not in
`manifest.csv`, and are not real students. The golden regression outputs
include them.
