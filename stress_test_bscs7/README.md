# CALSS Demo / Validation Dataset — BSCS-7

A synthetic class of **30 students (section BSCS-7)** across **two subjects**,
Programming 1 and Programming 2. Some cheating cases are planted, and the answer
key is known, so you can check what CALSS reports against the truth instead of
judging it by eye.

This dataset is separate from `../stress_test/` (145 students, 4 blocks). That one
is about accuracy with small per-student samples. This one is for a realistic
single-section demo, and it's the dataset used to check the **View source gold/red
highlights**.

## Shape

- 2 subjects × 4 grading periods (prelim, midterm, semifinal, final)
- Per student, per period: **3 activities** (these build the student's profile) + **1 exam**
- 16 files per student per subject → **480 files per subject, 960 total**
- Filenames use CALSS's real scheme: `{period}{activity1..3|exam}__{StudentId}_ASGN{hash}.c`
- All 960 files pass a C syntax check (`gcc -fsyntax-only`)

| Period | Programming 1 exam | Programming 2 exam |
|---|---|---|
| Prelim | Grade Computation | Primes in a Range |
| Midterm | Simple Calculator | String Analyzer |
| Semifinal | Number Analyzer | Sort, Search and Median |
| Final | Array Statistics | Student Records |

**How to load:** use folder import on **one subject folder at a time**
(`Programming1/` or `Programming2/`). CALSS reads the `BSCS-7` subfolder as the
section. Don't load the whole `stress_test_bscs7/` folder at once, because the two
subjects would be mixed into one run.

## How students differ

Each student has one fixed coding style, used in every file they write. It's built
from one of 7 style types, plus small personal variations:

| Style type | Braces | Operator spacing | Indent | Naming | Comments |
|---|---|---|---|---|---|
| neat | same line | spaced | 4 spaces | camelCase | mixed |
| allman | next line | spaced | 4 spaces | snake_case | own line |
| compact | same line | **unspaced** | 2 spaces | short (`a`, `n`) | none |
| commenter | same line | spaced | 4 spaces | snake_case | **inline** |
| linux | function braces on next line | spaced | tab | snake_case | own line |
| messy | mixed | half spaced | 2 spaces | short | few |
| tabcamel | same line | spaced | tab | camelCase | own line |

Each exam is built from randomly chosen structural options (which loop, helper
functions or not, and so on), so honest classmates solving the same exam don't
end up with identical tokens.

## Planted cases (answer key)

The full answer key is in `manifest.csv`, one row per file. Planted files have a
non-`normal` value in `scenario`, and their `note` says what CALSS should show.

| Subject | Period | Kind | Students (source → copier) | What was done |
|---|---|---|---|---|
| P1 | midterm | verbatim | Bryan Castro → Czarina Dizon | Same file, only the name in the header changed |
| P1 | semifinal | disguised | Francis Galang → Gwen Hernandez | Same solution, retyped in Gwen's own style and variable names |
| P1 | final | ghost | Harvey Ilagan + Isabel Jimenez | Both submitted one outside writer's solution |
| P1 | prelim | solo_swap | Kyla Lacson | Exam written by someone outside the class; no partner |
| P2 | prelim | verbatim | Mikaela Navarro → Nathan Ocampo | Same file, only the name changed |
| P2 | midterm | disguised | Patricia Pangilinan → Rhea Ramirez | Same solution, retyped in Rhea's style |
| P2 | semifinal | partial | Sean Salazar → Trisha Tolentino | Trisha pasted Sean's sort and search functions; the rest is her own |
| P2 | final | ghost | Ulysses Umali + Vanessa Valdez | Both submitted one outside writer's solution |

## Measured results

These were measured outside the GUI by small harnesses. They call the app's own
`analyzePair()` (`similarity.cpp`) and `measureFunctionStyles()` (`features.cpp`),
and they reproduce `computeStyleChanges()` in `gui.cpp` with the same thresholds.
KNN authorship scores were **not** measured here.

### Flagged pairs (each exam has 30 students → 435 pairs)

| Plant | Combined | Token | Style | Flagged | Rank in that exam |
|---|---|---|---|---|---|
| P1 verbatim | 100.0 | 100.0 | 100.0 | yes | **#1** |
| P1 disguised | 82.3 | 93.0 | 66.4 | yes | #57 |
| P1 ghost | 98.2 | 100.0 | 95.4 | yes | **#1** |
| P2 verbatim | 100.0 | 100.0 | 100.0 | yes | **#1** |
| P2 disguised | 81.5 | 89.8 | 69.0 | yes | #15 |
| P2 partial | 84.3 | 79.6 | 91.4 | yes | #15 |
| P2 ghost | 99.0 | 100.0 | 97.4 | yes | **#1** |

All 7 planted pairs are flagged. Direct copies (verbatim and ghost) rank first.
Disguised and partial copies rank lower because retyping changes the style and
the tokens. That's realistic: a disguised copy is the hardest case for any
similarity tool.

### Red highlight (View source: "this function doesn't look like its owner")

Red marks a function when **at least 2** of its formatting features (braces,
operator spacing, inline comments, line length) move outside the student's usual
range. The usual range comes from the student's activities, and a change counts
only if it's bigger than 2× that student's normal spread, with a minimum size per
feature (**12 characters for line length**).

| Plant | Expected | Result |
|---|---|---|
| P1 verbatim: Czarina's file | red | ✅ red (braces + spacing) |
| P1 disguised | no red | ✅ no red |
| P1 ghost: Harvey's file | red | ✅ red (spacing + line length) |
| P1 ghost: Isabel's file | red | ❌ **known miss** |
| P1 solo_swap: Kyla's file | not visible | red is computed, but View source only opens from a flagged pair, so it never appears in the app |
| P2 verbatim: Nathan's file | red | ❌ **known miss** |
| P2 disguised | no red | ✅ no red |
| P2 partial: Trisha's pasted functions | red | ❌ **known miss** |
| P2 ghost: both files | red | ✅ red on both |
| **All 225 honest exams** | no red | ✅ **zero false reds** |

**Why the 3 misses happen:** in each one, only **one** feature clearly changes.

- **Isabel:** the outside writer's braces and spacing already match her own style.
  Her lines are only ~10 characters longer, under the 12-character minimum.
- **Nathan:** the copied code is split into short functions. Operator spacing
  changes in one function and line length (~11 characters) in another, so no
  single function has 2 changes.
- **Trisha:** the pasted functions clearly change her operator spacing (0.91–0.94
  vs. her usual 0.12), but line length changes by only ~10–12 characters.

These cases were kept on purpose rather than tuned to pass. They show the red
highlight's real limit: a copy whose formatting differs from the owner's in only
one way isn't marked red. The gold highlight and the flagged-pair list still catch
all three.

### Honest pairs above the flag line

The flag line is a combined score of **75%**. On this dataset, a large share of
**honest** exam pairs also score 75% or higher:

| Exam | Honest pairs flagged | Highest honest pair |
|---|---|---|
| P1 prelim | 141 / 435 (32%) | 99.6 |
| P1 midterm | 181 / 434 (42%) | 98.4 |
| P1 semifinal | 178 / 434 (41%) | 95.5 |
| P1 final | 181 / 434 (42%) | 95.9 |
| P2 prelim | 137 / 434 (32%) | 98.5 |
| P2 midterm | 82 / 434 (19%) | 88.4 |
| P2 semifinal | 146 / 434 (34%) | 88.9 |
| P2 final | 293 / 434 (68%) | 94.8 |

For short beginner exams that all solve the same problem, honest solutions end up
close together. `../stress_test/` shows the same pattern (~61%). **Ranking** works
well: direct copies are #1 every time. The **fixed 75% line** marks many honest
pairs. This is recorded as a finding and the scorer is unchanged. Whether to
change it is an open decision.

## Honest limits of this dataset

- It's synthetic. Styles are generated from 7 types, and real students vary more,
  and less neatly.
- 3 activities per period is a small history. Red needs at least 3 earlier
  submissions per feature, which this meets with 12 activities per subject, but
  thin histories make the "usual range" noisy.
- The inline-comment feature is only judged for students who write comments, so
  students with no comments (`compact`) are never judged on it.
- KNN authorship scores, including Kyla's solo_swap, still need to be checked in
  the real app.

## Files

- `Programming1/BSCS-7/`, `Programming2/BSCS-7/`: the 960 `.c` submissions
- `manifest.csv`: the answer key, one row per file (`scenario` + `note` columns;
  the notes use Windows-1252 encoding)
- `generate.py`: the generator. `python generate.py` rebuilds everything
  **byte-for-byte identically** (fixed seed). The planted cases are listed in
  `PLANTED`, their expected results in `EXPECT`, and the 3 known red misses in
  `EXPECT_OVERRIDE`.
