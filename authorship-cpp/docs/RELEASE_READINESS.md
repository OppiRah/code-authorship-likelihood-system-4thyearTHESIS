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

## 6. Portable thesis-test package (2026-10-05)

First portable package for the two thesis mates (Google Drive → their
own PCs). Not an installer. Branch `optimization-pass`; source state
`a170da6` (no `src/`/`include/` change since); packaging commits
`e535c51`, `c019eda`.

**What changed (packaging only, no source change):**
`make_release.bat` now also copies `release_docs\README.txt` and
`TESTER_CHECKLIST.txt`, generates `BUILD_INFO.txt` (release name,
branch, commit, exe SHA-256), checks the folder holds exactly the 16
expected files, and writes two ZIPs: the app package and a separate
`CALSS_SampleData_SYNTHETIC.zip` (`stress_test_bscs7/Programming1` +
`Programming2`, with expected counts). `--nozip` skips the ZIPs. The
script is ASCII now (cmd split the old UTF-8 box-drawing REM lines into
junk commands). Release docs are CRLF + UTF-8 BOM via `.gitattributes`.

**Evidence (dev PC, Windows 11 26200):**

| Check | Result |
|---|---|
| Build | `compile.bat`'s exact g++ command, clean (old exe moved aside first): exit 0, no warnings, ~90 s. g++ 16.1.0 (MinGW-Builds, posix-seh). |
| Golden regression | All three datasets **byte-identical** (`bscs7_programming1`, `bscs7_programming2`, `stress_test_all_blocks`), harness built from the same source as the exe. |
| EXE | 4,125,363 bytes; SHA-256 `0A27C4EB013B73791AEEFD848E881388091B178396A1E834040B426EE5D9BB63`; PE32+; Subsystem 2 (Windows GUI). |
| Imports | ADVAPI32, COMCTL32, comdlg32, GDI32, gdiplus, KERNEL32, MSIMG32, ole32, SHELL32, USER32, WINHTTP, WS2_32 + `api-ms-win-crt-*` (UCRT). No `libwinpthread`, `libstdc++`, `libgcc`. |
| Package contents | 16 files: `authorship.exe`, `BUILD_INFO.txt`, `README.txt`, `TESTER_CHECKLIST.txt`, `calss_gc_embedded_credentials.json`, `ulccslogo.png`, `ulcclogoloadingscreen.png`, `assets\help\calss_help_01…09.png`. |
| Secrets scan | No Gemini key (`AIza…`), access token (`ya29.…`) or refresh token in the folder or exe; no developer user name / repo path in any file. The only `access_token` hits are the exe's empty JSON template. The OAuth **client** file ships by design (§1). |
| Packaged launch | ZIP extracted to `%TEMP%\CALSS pkgtest Peña 1\Downloaded\CALSS` (space + non-ASCII), started with `PATH=C:\Windows\System32;C:\Windows` and working directory set to an unrelated empty folder: main window opens; no child console process; every loaded module except the exe is under `C:\Windows`; sidebar seal PNG and Help carousel slide 1 picture render (screen captures); closes cleanly on WM_CLOSE; nothing written to the package folder or the working directory. |
| Not run here | GUI analysis from the packaged exe (pipeline covered by the golden check; GUI items in `tests/README.md` were run manually by the developer before packaging), a fresh Windows user / fresh Credential Manager (this launch used the developer's account, which already has a Gemini key), Gemini, Classroom sign-in. |

**Still open, requires the thesis mates' PCs (not passed until they
report):** start on a PC without MinGW, SmartScreen/Defender behaviour
for an unsigned exe downloaded from Drive, first-run with an empty
Credential Manager, GUI analysis + CSV + report end to end, Peña path in
the GUI, Classroom sign-in with an approved test-user account. Checklist:
`release_docs/TESTER_CHECKLIST.txt` (ships in the package).

**Noted, not changed (not packaging blockers):**
- Help > About still reads "Version 0.4.0 (Proposal Prototype)"
  (`gui.cpp`); README tells testers it's known.
- Help images 03/04 and 06/09 are byte-identical pairs (same screenshot
  used twice); the two logo PNGs are identical too.
- The ZIPs come from Windows PowerShell 5.1 `Compress-Archive`, which
  stores `\` path separators: fine for Windows Explorer / 7-Zip, may
  look flat in non-Windows unzip tools.
- Known issues 1–9 in §3 are unchanged; the OAuth app stays External /
  Testing (only listed test users can sign in; ~7-day token expiry).
