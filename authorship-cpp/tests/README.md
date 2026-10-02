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

## GUI smoke checklist

Build with `compile.bat`, launch `authorship.exe`, and tick each item.

1. Splash screen shows the UL seal; main window opens on the welcome screen.
2. Load a folder (`../stress_test_bscs7/Programming1`): file count appears in the sidebar.
3. Import files: picking two `.c` files copies them in and shows a toast.
4. Run analysis with AI **off**: progress bar advances through phases; Cancel works on a second run.
5. Overview: headline, stat row and risk bar render; no AI box errors.
6. Flagged Pairs: list renders; severity filter dropdown and search box filter pairs; expand a pair; Copy findings; View source (single file and compare, gold highlights, scrolling).
7. Export CSV: Save As writes a file; student names in the CSV match the names shown in the app.
8. Classes: roster rows, sort dropdown, block sub-tabs; open a student; Style DNA strip; deep link to a flagged pair; Back.
9. Student detail: Generate Similarity Report opens an HTML file in the browser.
10. Unclassified files (if any): Assign dialog opens, validates, assigns.
11. File > AI Settings: Test key works with a valid key; with no key, "Include AI summary" is disabled.
12. Run analysis with AI **on** (needs a key): Overview batch patterns and per-pair narratives appear.
13. File > Google Classroom: sign-in opens the browser; choose, sync, done states work.
14. View > View Audit Log opens Notepad.
15. Help > Documentation carousel: all slides, Next/Prev/Skip.
16. Resize window and drag the sidebar edge: no stale paint.
