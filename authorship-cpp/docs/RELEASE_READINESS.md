# CALSS Release Readiness — Phase 4 (2026-10-02)

Status of branch `optimization-pass` as a release candidate, written as
the input for Phase 5 (installer). Nothing here implements packaging.

## 1. What a release consists of

`make_release.bat` (run after `compile.bat`) assembles `release\CALSS\`:

| File | Needed for | Located at runtime via |
|---|---|---|
| `authorship.exe` | everything | — |
| `ulccslogo.png` | sidebar seal (falls back to a drawn circle) | exe folder |
| `ulcclogoloadingscreen.png` | splash seal (same fallback) | exe folder |
| `assets\help\calss_help_01…09.png` | Help > Documentation carousel | exe folder |
| `calss_gc_embedded_credentials.json` | Google Classroom sign-in (the app's OAuth client ID; not a personal secret) | exe folder |

All are found relative to the **exe's folder**, never the working
directory, so a shortcut's "Start in" or a file dialog can't break them.

## 2. Release review (static, plus the checks listed)

| Area | Finding |
|---|---|
| Runtime DLLs | **Fixed in Phase 4.** The exe imported MinGW's `libwinpthread-1.dll` and failed to start without MinGW (`0xC0000135`, reproduced with a System32-only `PATH`). Now linked `-static`: imports are Windows system DLLs plus the UCRT `api-ms-win-crt-*` sets. |
| Console window | **Fixed in Phase 4.** The exe was a console-subsystem program: a double-click opened an empty console next to CALSS, and closing it killed the app. Now `-mwindows` (GUI subsystem). |
| OS requirement | 64-bit Windows 10 or 11. UCRT is built in from Windows 10; Windows 7/8.1 would need the UCRT update (KB2999226) and are untested. Per-monitor DPI APIs are loaded dynamically with a fallback. |
| Hard-coded paths | None found (searched `src/` and `include/` for drive letters, user names and repo paths). |
| Read-only locations | None written. The exe folder is only read, so a Program Files install works without admin rights at runtime. |
| Writable locations | Classroom settings → `%LOCALAPPDATA%\CALSS`; similarity reports → `Documents\CALSS Reports`; default Classroom sync folder → `Documents\CALSS_Classroom_Sync`; audit log → `%PROGRAMDATA%\CALSS\audit.log`; CSV → wherever the user saves; `calss_sync_manifest.jsonl` → inside the data folder being analyzed. |
| Secrets | Gemini key and Classroom token in Windows Credential Manager, per Windows user. Nothing personal ships in the release folder (`make_release.bat` deletes stray `api_key.txt` / `calss_gc_token.json`). |
| Paths with non-ASCII characters | Supported end to end since D-4 (every path string is UTF-8, opened via wide APIs). Verified at pipeline level with a `Peña` folder. |
| Admin rights | Not required by the app. |
| Startup | One-time import of legacy `api_key.txt` / `calss_gc_token.json` from the exe folder or working folder, if present; otherwise no first-run writes. |
| Network — Gemini | HTTPS to `generativelanguage.googleapis.com`; explicit timeouts; top 50 flagged pairs per run. |
| Network — Classroom | OAuth installed-app flow, redirect to `http://localhost:8080`, listener on 127.0.0.1 only, 3-minute timeout, `state` checked. |

## 3. Known issues (not fixed in Phase 4)

1. **Manifest re-reading on first load (performance debt).**
   `autoTagManualBlock` (analysis_pipeline.cpp) re-opens and re-reads the
   whole `calss_sync_manifest.jsonl` for every file found in a subfolder,
   then appends one line — O(n²) file I/O. Measured: first load of the
   four-block dataset (2,973 files) ≈ 41 s just to load; a second load of
   the same folder ≈ 1.3 s; one section ≈ 0.06–0.08 s. Deliberately left
   for a later pass; worth doing only if professors routinely load several
   blocks at once. A fix (read the manifest once into a set, append in one
   pass) would not change any score.
2. **Proxy settings.** Gemini and Classroom use WinHTTP's *default* proxy
   setting (`netsh winhttp`), not the proxy configured in Windows
   Settings. On a network that requires a proxy configured only in
   Windows Settings, AI summaries and Classroom sync will fail to connect.
   Not changed (needs a network to test on).
3. **Fixed OAuth port 8080.** If another program is listening on 8080,
   sign-in fails with "Authentication timed out or was cancelled", which
   doesn't say why.
4. **Google OAuth consent screen.** If the Cloud project is in "Testing"
   mode, only accounts on its test-user list can sign in, and their tokens
   expire after 7 days. External configuration, not code.
5. **Audit log on shared PCs.** `%PROGRAMDATA%\CALSS\audit.log` is created
   by whichever Windows user runs CALSS first; on default ACLs other
   standard users may only be able to read it, and their audit entries are
   then silently dropped. Single-user PCs are unaffected. Relevant to how
   the installer creates that folder.
6. **Classroom HTTP timeouts.** Classroom calls use WinHTTP defaults
   (no explicit timeouts, unlike Gemini). A stalled DNS lookup can hold
   the sync sheet.
7. **GUI paint cost.** `drawCard()` dominates per-frame cost on every tab
   (~2 ms per card) and makes the View source scroll glide choppy;
   deferred as fix D (needs visual sign-off). See
   `docs/SOURCE_VIEW_AND_SCALE_WORK_LOG.md`.
8. **No application icon.** The exe uses the generic Windows icon
   (`ulccslogo.ico` exists but no resource script is compiled). Natural
   to add with the installer.
9. **Two "High" counts.** Overview "High similarity" (label-based, all
   pairs) and the Flagged tab's "High" filter (combined ≥ 85 %, flagged)
   count different things. Pre-existing, flagged in CLAUDE.md.

## 4. Thesis methodology — unchanged, decisions for the team

No scoring formula, weight, threshold, tokenization rule, keyword list,
style feature, KNN setting, pair-selection rule, student/category/period
parsing or Gemini prompt was changed by Phases 0–4. Every checkpoint
reproduced the Phase 0 golden outputs byte for byte (see
`tests/README.md`).

- **LCS 500-token cap.** `lcsLength` compares only the first 500
  normalized tokens of each file but `lcsSimilarity` divides by the full
  lengths, so identical files longer than 500 tokens score below 100 %
  (two identical 1,000-token files: LCS similarity 0.50). The synthetic
  datasets max out at 383 tokens, so their results are unaffected. Decided
  2026-09-29 to leave it; it should be stated as a limitation in the paper.
- **Flag rate.** At the 75 % threshold the validation datasets flag 32 %
  (Programming1: 705 of 2,186 pairs) to 64 % (four blocks: 27,991 of
  43,816) of compared pairs. Precision/recall should be measured against
  the `manifest.csv` answer keys before the thesis makes accuracy claims.
  This is an analysis task, not a code change.

## 5. Facts for Phase 5 (installer)

- Install everything from `release\CALSS\` into one folder; the app needs
  no write access there.
- No admin rights needed to run. A per-user install
  (`%LOCALAPPDATA%\Programs\CALSS`) also works.
- Uninstall should decide what to do with: Credential Manager entries
  `CALSS/GeminiApiKey` and `CALSS/GoogleClassroomToken`;
  `%LOCALAPPDATA%\CALSS`; `%PROGRAMDATA%\CALSS\audit.log` (an integrity
  record — probably keep); `Documents\CALSS Reports` (user documents —
  keep).
- See known issue 5 if the installer creates `%PROGRAMDATA%\CALSS`.
- `ulccslogo.ico` is available for the shortcut and an exe icon resource.
