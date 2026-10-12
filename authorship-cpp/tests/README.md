# CALSS regression protection

Two checks guard every change: the **golden-output check** (automatic,
covers the analysis pipeline) and the **GUI smoke checklist** (manual,
covers everything on screen). Run both before committing a change that
touches `src/` or `include/`.

## Golden-output check

```
tests\run_golden.bat            build the harness, run, compare
tests\run_golden.bat --update   regenerate tests\golden\expected\
```

`golden_harness.cpp` calls the real `runAnalysisPipeline()` with AI off
on three synthetic datasets committed at the repository root, and writes
every pair score, every flagged-pair authorship result, every profile
vector and the run summary to CSV (doubles printed with `%.17g`). The
check passes only if the SHA-256 of every CSV matches
`tests/golden/expected/<dataset>/hashes.txt`. The pipeline is
deterministic, so the outputs are compared exactly, with no tolerance.

| Dataset | Source | Files | Pairs | Flagged |
|---|---|---:|---:|---:|
| `bscs7_programming1` | `../stress_test_bscs7/Programming1` | 553 | 2,186 | 705 |
| `bscs7_programming2` | `../stress_test_bscs7/Programming2` | 480 | 1,740 | 662 |
| `stress_test_all_blocks` | `../stress_test` (4 blocks loaded together) | 2,973 | 43,816 | 27,991 |

Reference CSVs sit next to each `hashes.txt` for diagnosing a failure,
except the 46 MB `flagged.csv` of the four-block dataset, which is
covered by its hash only. On a mismatch the batch file prints which
hashes differ and where the fresh output is (`%TEMP%\calss_golden\out`).

**Only use `--update`** when a change is *meant* to alter results (a
classified bug fix), and say so in the commit message together with
what changed and why.

### Timing baseline (2026-10-02, this dev PC, `-O2`, AI off)

Measured through the harness (includes its own CSV/hash writing, which
is not part of a real run):

| Dataset | Pipeline time |
|---|---:|
| `bscs7_programming1` | ~4.3 s |
| `bscs7_programming2` | ~4.6 s |
| `stress_test_all_blocks` | ~70 s |

Per operation (2,973 files, 20,000 sampled pairs): `tokenize` 0.103 ms
per file, `extractFeatures` 0.079 ms per file, `computeSimilarity`
0.043 ms per pair.

### After PF-1 (per-run file caching), same PC

Pipeline only, timed by phase through the progress callback, data
already loaded once (so the manifest exists — see the first-load note):

| Dataset | Before PF-1 | After PF-1 |
|---|---:|---:|
| `bscs7_programming1` | ~1.5 s | ~0.55 s |
| `stress_test_all_blocks` | ~28–29 s | ~7.4 s |

First load of a folder with subfolders is much slower (four blocks:
~41 s just to load) because `autoTagManualBlock` re-reads the whole
`calss_sync_manifest.jsonl` once per file — not addressed by PF-1.

## Workspace-state unit tests (1.3.0)

```
tests\run_workspace_tests.bat
```

Builds `workspace_state_test.cpp` against `src/workspace_state.cpp`
(the "Continue previous session" file) into `%TEMP%` and runs it:
round trip incl. UTF-8/quotes/backslashes, malformed and oversized
input, future format version, invalid fields, fields dropped that don't
belong to the saved page, atomic save/replace, corrupt/empty files, and
the restore decisions (dataset checks, Classroom path test, lookup by
name, pair lookup in either order). 89 checks.

## GUI smoke checklist (manual)

CALSS has no automated GUI tests. Run this by hand before merging a
change that touches the GUI, file handling or integrations. Build with
`compile.bat` first. Items marked **[you]** need your Gemini key, Google
account or a second PC; everything else needs only the repo.

**A. Launch and data loading**

1. Double-click `authorship.exe` in Explorer: splash shows the UL seal,
   the main window opens on the welcome screen, and **no console window**
   appears.
2. Load `../stress_test_bscs7/Programming1` (has a `BSCS-7` subfolder):
   the sidebar file count is non-zero.
3. Load `../stress_test` (four block subfolders): loads; the first load
   is slow (~40 s, known — see Known issues in
   `docs/RELEASE_READINESS.md`), a second load of the same folder is fast.
4. **Non-ASCII path:** copy `../stress_test_bscs7/Programming1` to a
   folder named `Peña` and load it: same file count and, after analysis,
   the same headline numbers as the original (2,186 pairs, 705 flagged).
5. Import files: pick two `.c` files; they are copied to
   `Documents\CALSS Imports` and a toast shows.

**B. Analysis, AI off**

6. Untick "Include AI summary", run: progress moves through the phases;
   start a second run and Cancel works.
7. Overview: headline, stat row and risk bar render; the AI box says the
   summary was not requested (or, with no key set up, that AI is off).
8. Classes: roster rows, sort dropdown, block sub-tabs; open a student:
   Style DNA strip, submissions, flagged appearances, deep link to a pair,
   Back.
9. Flagged Pairs: list, severity filter, search, expand a pair: it ends
   with the AI EVIDENCE NOTE (no Comparative Authorship Analysis box or
   Copy findings button since 1.3.0, and no empty space below the note).
10. View source from a flagged pair: single file opens; compare mode has
    gold match and red style highlights; A/B tabs, Prev/Next, N/P/H keys,
    Tab, mouse wheel and Up/Down/PgUp/PgDn/Home/End scrolling; close.
11. View > View Student Profiles: the toast points to the Classes tab.

**C. Outputs**

12. Export CSV (Flagged Pairs toolbar): Save As anywhere writes the file;
    student names match the app (e.g. `Carlos`, not `finalsexamCarlos`).
13. Right after the export, Generate Similarity Report on a student: the
    report opens in the browser and is saved in `Documents\CALSS Reports`
    (proves the export didn't move the app's working folder).
14. View > View Audit Log opens Notepad (`C:\ProgramData\CALSS\audit.log`).

**D. AI on [you]**

15. File > AI Settings: Test key succeeds with your key; with the key
    removed, "Include AI summary" is disabled.
16. Run with AI on: Overview shows cross-pair findings; expanded pairs show
    AI notes. With more than 50 flagged pairs (Programming1 has 705):
    pairs past #50 say "No AI note for this pair…", and the Overview
    attribution line says "based on the 50 highest-scoring of N".

**E. Google Classroom [you]**

17. File > Google Classroom: the browser opens Google's consent page;
    after sign-in the page says "Authentication Successful" and the sheet
    moves to course selection; sync downloads into the chosen folder.
18. Start sign-in and close the browser tab: within ~3 minutes CALSS
    reports that sign-in timed out (it no longer waits forever).

**F. Restart and other machines**

19. Close and reopen CALSS: the AI key and Classroom sign-in are
    remembered (Credential Manager); no first-run notices reappear.
20. Help > Documentation carousel: all slides, Next/Prev/Skip.
20a. **Continue previous session (1.3.0).** Run an analysis, open
    Classes > a block chip > a student, close CALSS, reopen: the welcome
    screen shows "CONTINUE PREVIOUS SESSION" with the dataset, "Classes →
    block → student" and the time. Continue: the analysis re-runs with
    the AI summary off (even if the box is ticked) and lands on that
    student; Back to Classes shows that block selected. Repeat with an
    expanded flagged pair (it comes back expanded and scrolled to) and
    with Overview. "Start new" hides the card and changes nothing.
20b. Rename the dataset folder and Continue: "Previous session
    unavailable" with "Select folder…"; nothing crashes. Remove one
    student's files and Continue: "Session partly restored" names the
    student and the new file count. Open
    `%LOCALAPPDATA%\CALSS\workspace.json` in Notepad and break it:
    CALSS starts normally without the card.
21. Resize the window and drag the sidebar edge: no stale paint.
22. **[you]** Run `make_release.bat`, copy `release\CALSS\` to another
    Windows 10/11 PC without MinGW (or another Windows user account), and
    repeat items 1, 2, 6 and 13 there.

**Done by Claude (2026-10-02, Phase 4):** item 1's "starts without MinGW
and without a console" part was checked by launching the exe with `PATH`
limited to System32 from an unrelated folder (it starts; before `-static`
it failed with `0xC0000135`) and by checking its window list (no console
host). The Peña case (item 4) was checked at the pipeline level, not in
the GUI. Every other item above has **not** been run in the GUI.
