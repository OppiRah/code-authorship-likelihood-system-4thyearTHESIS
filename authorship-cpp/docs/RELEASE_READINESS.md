# CALSS Release Readiness — Phase 4 (2026-10-02), Phase 5 (2026-10-07)

Status of branch `optimization-pass` as a release candidate. §1–§5 were
written in Phase 4 as the input for the installer; §6 is the portable
test package; §7 is the Phase 5 installer (CALSS 1.0.0); §8 is CALSS
1.1.0 (Classroom storage, Gemini model discovery); §9 is CALSS 1.2.0
(Overview AI summary organized by class); **§10 is CALSS 1.3.0
(Continue previous session; Comparative Authorship Analysis panel
removed)** and is the current state wherever it differs from the
earlier sections.

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
5. **Audit log on shared PCs.** *Fixed in Phase 5 for all-users
   installs (§7.4).* `%PROGRAMDATA%\CALSS\audit.log` is created
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
8. **No application icon.** *Fixed in Phase 5 (§7.1).* The exe used the generic Windows icon
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

**Result (recorded 2026-10-07):** the developer reports that the thesis
mates' cross-PC testing of this package completed successfully and that
the package gate is passed, including ordinary execution on their PCs.
That is a user-reported overall result; the filled-in checklists are not
in the repository, so no individual item above is marked passed here.

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

## 7. Phase 5 — installer, CALSS 1.0.0 (2026-10-07)

Branch `optimization-pass`. Commits: `87b4ee7` (icon, manifest, version
resource), `02ff912` (Import .c files location), `a074b64` (append-only
audit log), `6002355` (installer), then this documentation. No scoring,
tokenizer, feature, KNN, threshold, parsing or Gemini-prompt code was
touched; the golden check was byte-identical after every source commit.

### 7.1 Executable

| Item | Result |
|---|---|
| Version | **1.0.0 "Thesis Release"** — first installer release; a conservative 1.0 with no pre-release tags. Single source: `include/version.h`, read by Help > About, the exe's VERSIONINFO and (through the exe) the installer. Replaces the stale "0.4.0 (Proposal Prototype)"; About now also uses the full product name "Code Authorship Likelihood Scoring System". |
| Resources | `res/calss.rc` → `calss_res.o` (windres, first step of `compile.bat`): icon, manifest, VERSIONINFO. |
| Version info | ProductName "CALSS"; FileDescription "CALSS - Code Authorship Likelihood Scoring System"; FileVersion 1.0.0; ProductVersion "1.0.0 (Thesis Release)"; CompanyName "CALSS thesis team, College of Computer Studies, University of Luzon" (attribution, as in the in-app About text — no copyright or ownership line is claimed); OriginalFilename authorship.exe. |
| Icon | `ulccslogo.ico` was a PNG renamed `.ico` (not usable as a resource). Now a real 9-size ICO (16–256 px) built by `tools/make_icon.py` from the in-app seal `ulccslogo.png`, placed on a rounded tile in the app background colour (GRAY_950) because the white mark disappears on light Explorer backgrounds. Used for the exe, the main window class (taskbar/Alt+Tab), shortcuts, the installer and Add/Remove Programs. |
| Manifest | `res/calss.manifest`: `asInvoker` (never elevates) and `supportedOS` Windows 7–11. **Common Controls v6 deliberately not enabled:** themed v6 push buttons and checkboxes ignore `WM_CTLCOLORBTN`/`WM_CTLCOLORSTATIC`, which the sidebar uses for the maroon "Run analysis" button and the light-on-dark "Include AI summary" label — v6 would turn them stock grey / dark-on-dark. **DPI not declared:** `runGUI()` already sets Per-Monitor-V2 before any window exists and derives `g_dpiScale` from the same call; a manifest entry would be redundant on Windows 10 1703+. Screenshots after the change show the GUI unchanged. |

### 7.2 Build verification (dev PC, Windows 11 26200, g++ 16.1.0)

- `compile.bat` clean build (old exe and `calss_res.o` removed first):
  exit 0, no warnings. Still `-static -mwindows`.
- PE: Subsystem 2 (Windows GUI). Imports: ADVAPI32, COMCTL32, comdlg32,
  GDI32, gdiplus, KERNEL32, MSIMG32, ole32, SHELL32, USER32, WINHTTP,
  WS2_32 + `api-ms-win-crt-*`. No libwinpthread/libstdc++/libgcc.
- Manifest present with `asInvoker`; no Common-Controls dependency;
  version strings read back through Windows' `VersionInfo`.
- `tests\run_golden.bat`: **all three datasets byte-identical** after
  each of `87b4ee7`, `02ff912`, `a074b64`.
- Final `authorship.exe`: 4,501,119 bytes, SHA-256
  `908851FA06C995858DB555D6FCCAAE2CA22EC506609666C06F6081B2BAB095CD`.

### 7.3 Installer technology: Inno Setup 6

Chosen over NSIS, WiX/MSI and MSIX because it is the simplest tool that
covers everything CALSS needs: one declarative script
(`installer/calss.iss`), a built-in uninstaller and Add/Remove Programs
entry, Start Menu/Desktop shortcuts, per-machine *or* per-user install
from the same file, a small Pascal hook for the audit-log step, nothing
installed on the user's PC besides CALSS, free, widely used and well
documented. WiX/MSI and MSIX are heavier (MSIX also needs signing to
install at all); NSIS needs more hand-written script for the same
result.

Built with Inno Setup 6.7 (already installed on the dev PC, per-user,
signed by its publisher Pyrsys B.V.); the script needs 6.4+. Build:
`compile.bat`, then `make_installer.bat` → `release\CALSS_Setup_1.0.0.exe`
+ `release\CALSS_Setup_1.0.0_SHA256.txt`. The script reads version,
description and publisher from the exe. Two builds of the same commit
gave the same installer hash.

### 7.4 Install layout, shortcuts, audit log

- **Mode:** default *all users* → `C:\Program Files\CALSS` (64-bit,
  Windows 10+); Setup asks for admin once. The installer's own dialog
  also offers *only for me* → `%LOCALAPPDATA%\Programs\CALSS`, no admin
  (for lab PCs where professors are not administrators). CALSS itself
  runs as the normal user in both cases (verified: token not elevated).
- **Installed files (13, plus Inno's `unins000.exe`/`unins000.dat`):**
  ```
  CALSS\
    authorship.exe
    ulccslogo.png
    ulcclogoloadingscreen.png
    calss_gc_embedded_credentials.json   (the app's OAuth client id, §1)
    assets\help\calss_help_01 … 09 .png
  ```
  Nothing else: no source, objects, datasets, docs, keys, tokens or
  logs. The `[Files]` list names each file; the build refuses to run
  without the exe's version resource or the OAuth client file.
- **Shortcuts:** Start Menu "CALSS" (all users: ProgramData Start Menu;
  only for me: the user's). Desktop "CALSS" as an installer task, ticked
  by default (Public Desktop / user Desktop). Both use the exe's icon
  and Start in = install folder; the app does not depend on the working
  directory (resources from the exe folder, writes to known folders).
- **Import .c files fix (`02ff912`):** found during this phase — the
  import copied files to `imported_data` *relative to the working
  directory*. That worked from the portable ZIP but fails from an
  installed shortcut (read-only Program Files). Now
  `Documents\CALSS Imports`. §2's "Read-only locations: none written"
  was therefore not true for imports before this fix.
- **Audit log on shared PCs (known issue 5):**
  - *Cause, confirmed on this PC:* `audit_log.cpp` creates
    `%PROGRAMDATA%\CALSS` and `audit.log` on first use. The folder
    inherits ProgramData's ACL (Users: read + create files); the file
    gets creator full control + Users `(RX)`. `std::ofstream(ios::app)`
    asks for full write access, so any other Windows user's open failed
    and `audit_log()` silently returned.
  - *Fix:* CALSS opens the log with `FILE_APPEND_DATA` only (`a074b64`),
    same CRLF format. The all-users installer pre-creates the folder
    and file and grants `BUILTIN\Users` (by SID `S-1-5-32-545`) **Read +
    Append Data on audit.log only**. No user can overwrite, truncate,
    delete or re-permission the log (deleting needs DELETE on the file or
    delete-child on the folder; Users have neither). The folder keeps
    ProgramData's defaults; Administrators and SYSTEM keep full control.
  - *Evidence:* ACL after install: `BUILTIN\Users:(R,AD)` added. With a
    test file whose DACL gives the caller only `(R,AD)`: the old
    ofstream append fails; the new append-only open writes; truncate
    and delete are denied. A real second Windows account was **not**
    tested (none on this PC; creating one is an account action).
  - *Not covered:* an *only-for-me* install cannot change ProgramData
    ACLs (no admin); on a shared PC use the all-users install. CALSS is
    never run as administrator to work around this.

### 7.5 Data CALSS stores outside its install folder

| Data | Location | Created by | Purpose | Remove on uninstall? | Reason |
|---|---|---|---|---|---|
| Program files | install folder | installer | the program | **Yes** | binaries only |
| Shortcuts, Add/Remove entry | Start Menu, Desktop, registry Uninstall key | installer | launching / uninstalling | **Yes** | installer-owned |
| Gemini API key | Credential Manager `CALSS/GeminiApiKey` (per Windows user) | app, File > AI Settings > Save | AI summaries | No | the user's own secret, per user; the uninstaller runs as one account only. Remove beforehand with AI Settings > *Remove saved key*, or `cmdkey /delete:CALSS/GeminiApiKey` |
| Google Classroom token | Credential Manager `CALSS/GoogleClassroomToken` (per user) | app, after Classroom sign-in | Classroom sync | No | same reason; there is no in-app sign-out button. Remove with `cmdkey /delete:CALSS/GoogleClassroomToken` (and revoke at myaccount.google.com → Security → third-party access) |
| Classroom settings | `%LOCALAPPDATA%\CALSS\calss_gc_settings.json` (per user) | app | cached copy of the app's OAuth client config | No | not personal, regenerated; per user (same account limitation) |
| Audit log | `%PROGRAMDATA%\CALSS\audit.log` | installer (all users) or app | academic-integrity audit trail | **No** | evidence record |
| Similarity reports | `Documents\CALSS Reports` | app | per-student HTML reports | **No** | user documents |
| Imported submissions | `Documents\CALSS Imports` | app, File > Import | copies of the picked `.c` files | **No** | student data the user chose to import |
| Classroom downloads | 1.0.0: `Documents\CALSS_Classroom_Sync` (default) or the loaded data folder. **1.1.0: `%LOCALAPPDATA%\CALSS\Classroom`** | app, sync | downloaded submissions | 1.0.0: No. **1.1.0: Yes** (§8.2) | app-managed working copies of Classroom data |
| Sync manifest | `calss_sync_manifest.jsonl` inside each analyzed folder | app | block/title tags | **No** | lives in the user's data |
| CSV exports | wherever the user saves | app | flagged-pair export | **No** | user documents |
| Fallback audit log | `calss_audit.log` in the working folder | app | only if ProgramData can't be resolved (not observed) | No | — |

### 7.6 Uninstall policy

Uninstall removes the program files, the shortcuts and the Add/Remove
entry, nothing else, and then lists what it kept (reports, imports /
Classroom downloads, the audit log, saved sign-ins). Nothing marked "No"
in the table above is deleted, so a reinstall continues with the same
key, sign-in and audit trail. Credentials are not deleted because (a)
they belong to a Windows user, and an elevated uninstaller may run as a
different (admin) account than the professor's, so it could not do this
reliably anyway, and (b) silently deleting them is not expected
uninstall behaviour. The removal commands are listed above.

### 7.7 Signing, SmartScreen, checksums

The installer and the exe are **not code-signed** (no certificate; none
was bought or configured). Windows SmartScreen or the browser may
therefore warn ("Windows protected your PC", "not commonly downloaded"),
because an unsigned file carries no publisher identity or reputation.
That is expected; do not disable SmartScreen or Defender. Users should
only choose "More info → Run anyway" after checking the SHA-256 against
the value published with the official distribution (the thesis team's
shared Drive folder, next to the installer):

| File | Size (bytes) | SHA-256 |
|---|---:|---|
| `CALSS_Setup_1.0.0.exe` | 3,657,335 | `C301FFCF0AD811D28F465A3DF8F1980FA92D44F669F2FA372E93FC521D6FBBFF` |
| `authorship.exe` (inside) | 4,501,119 | `908851FA06C995858DB555D6FCCAAE2CA22EC506609666C06F6081B2BAB095CD` |
| `CALSS_SampleData_SYNTHETIC.zip` | 563,768 | `534D30B92CC79B04032B8DE555EF0EFC53E59276D771F8FF2DDAEEEC0949B263` |

`make_installer.bat` writes the first two into
`CALSS_Setup_1.0.0_SHA256.txt`. Check with
`Get-FileHash .\CALSS_Setup_1.0.0.exe -Algorithm SHA256`. The
2026-10-05 portable ZIP is superseded by the installer (its exe has no
icon, the 0.4.0 About text and the old import path).

### 7.8 Tests

**Automated (dev PC):** clean build; import table; subsystem; manifest
and version resource; golden check ×3 (byte-identical); append-only ACL
mechanism test; reproducible installer build.

**Same-machine clean-install test** (dev PC, starting with CALSS not
installed; driven by scripts that launch the shortcut and operate the
folder picker, buttons, tabs and Save/Open dialogs):

| # | Check | Only for me (silent) | All users (UAC approved by the developer) |
|---|---|---|---|
| 1–4 | Not installed before; installs; folder; exactly the 13 files + uninstaller | pass | pass |
| 5–6 | Start Menu / Desktop shortcut, exe icon, Start in = app folder | pass | pass (ProgramData Start Menu, Public Desktop) |
| 7–8 | Launch via shortcut; no console (no child process, GUI subsystem); only Windows DLLs loaded | pass | pass; process not elevated |
| 9 | Seal PNG loads from the install folder | pass | pass |
| 10 | Icon embedded and used by exe, shortcuts, installer | pass (extracted and rendered; taskbar not screen-captured) | — |
| 11 | About = "Version 1.0.0 (Thesis Release)" | pass (repo build of the same source) | — |
| 12–13, 19 | Folder picker → `C:\Users\…\CALSS Peña Test\Programming1`: 553 files; analysis (AI off): **705 of 2,186 flagged**, 35 students | pass | pass |
| 14 | Classes list → student detail | pass | — |
| 15–16 | Flagged pairs list, expand #1, View source (file read from the Peña path) | pass | — |
| 17 | Export CSV into the Peña folder: header + 2,186 rows | pass | — |
| 18 | Generate Similarity Report → `Documents\CALSS Reports\…html` | pass | — |
| — | File > Import .c Files → `Documents\CALSS Imports`, success toast | pass | — |
| 20–21 | Restart via Desktop shortcut; saved Gemini key recognised | pass | — |
| 22 | Nothing written into the install folder; no VirtualStore copy | pass | pass |
| 23 | Audit entries appended (folder, analysis, report) | pass | pass (+422 bytes) |
| 24 | Credential Manager entries read and kept | pass | pass |
| 25–27 | Uninstall: files, shortcuts, uninstall key gone; reports, imports, audit log (and its ACL), credentials kept; message shown | pass | pass (message text in the uninstall log) |
| 28–29 | Reinstall → same exe hash, analysis 705/2,186 again | pass | not repeated |

The test data created for this (Peña dataset copy, test report, test
import) was deleted afterwards; the audit entries remain (the log is
append-only by design).

**User-reported:** the portable-package cross-PC gate (§6).

**Not tested here:** another PC; Windows 10; a second Windows account
(shared-PC audit log end to end); an empty Credential Manager / first
run without a Gemini key; Gemini calls; Classroom sign-in; SmartScreen
(a locally built file has no download mark); the wizard pages
interactively (silent modes were used); the Help > Documentation
carousel in the installed copy; upgrading over an older CALSS install
(none exists).

### 7.9 Remaining known limitations

Known issues 1–4, 6, 7 and 9 in §3 are unchanged (manifest re-read
performance, WinHTTP proxy, fixed OAuth port 8080, OAuth app in Testing
/ test users only, Classroom HTTP timeouts, `drawCard` paint cost, two
"High" counts). Also: the build is unsigned; the only-for-me install
does not fix the shared-PC audit log; credentials survive uninstall by
design; there is no Classroom sign-out button; if fetching courses
fails after sign-in, the Choose screen shows an empty tree; the sync
sheet does not try the refresh token before browser sign-in. The Google
Classroom configuration is unchanged (External / Testing, same client,
same scopes). Thesis items T-1/T-2 (§4) remain paper work.

### 7.10 Final cross-PC installer test

Send each tester (via the Drive folder):
`release\CALSS_Setup_1.0.0.exe`, `release\CALSS_Setup_1.0.0_SHA256.txt`,
`release\CALSS_SampleData_SYNTHETIC.zip` and
`release_docs\INSTALLER_TEST_CHECKLIST.txt`. Not the portable ZIP, not
`calss_gc_embedded_credentials.json` on its own (it is inside the
installer), nothing from the repository.

They run the 24 checklist items (hash check, SmartScreen notes,
all-users install, first run, Peña analysis 705/2,186, report/CSV/import,
optional Gemini/Classroom, second-account audit log if possible,
uninstall, reinstall) and send the checklist back. Phase 5 is complete
when at least one non-developer PC passes items 1–18 and 22–24, with no
FAIL left unexplained.

## 8. CALSS 1.1.0 — Classroom storage + Gemini model discovery (2026-10-07)

Branch `optimization-pass`. Commits: `637d94a` (Classroom storage and
uninstall cleanup), `79fcd42` (Gemini model discovery and AI Settings),
`c1c282f` (structured JSON output), `8384456` (version 1.1.0), then this
documentation. The 1.0.0 installer passed cross-PC testing (developer
report); 1.1.0 is a new artifact with a new name and hash, and
`CALSS_Setup_1.0.0.exe` is kept unchanged next to it. Golden check
byte-identical after every source commit; no methodology code touched.

**Version:** 1.1.0, not 1.0.1. `include/version.h`'s rule bumps the
middle number for releases that change behaviour or add features, and
this one changes where Classroom data lives and what uninstall deletes,
and adds the model dropdown.

### 8.1 Google Classroom storage

**Before (≤ 1.0.0), traced in code:**

| Data | Written by | Location | Persistent |
|---|---|---|---|
| Downloaded `.c` submissions | `gc_downloadSubmissions` → `downloadDriveFile` (google_classroom.cpp) | the data folder currently loaded in CALSS (`g_state.dataFolder`); if none, `Documents\CALSS_Classroom_Sync` | yes, survived uninstall |
| Per-block "drop-zone" subfolders | `syncRunThread` (gui_sync_sheet.cpp) | same folder | yes |
| `calss_sync_manifest.jsonl` (file → assignment title, block) | `appendManifestEntry` | same folder | yes |
| OAuth client settings | `gc_saveSettings` | `%LOCALAPPDATA%\CALSS\calss_gc_settings.json` | yes |
| OAuth token | `secretWrite` | Credential Manager `CALSS/GoogleClassroomToken` | yes |
| Temporary files | none (downloads go memory → file) | — | — |

So a sync could write student submissions into any folder the professor
happened to have open, mixing them with that folder's own data.

**Now (1.1.0):** every sync writes to `appClassroomDirU8()` =
`%LOCALAPPDATA%\CALSS\Classroom` (via `SHGetFolderPathW(CSIDL_LOCAL_APPDATA)`,
like the other app_paths helpers). File names, subfolders and the
manifest format are unchanged; after a sync CALSS loads that folder as
before. The sync sheet's "Save to:" shows the real path.

- **Why not the install folder:** Program Files is read-only for normal
  users and must hold only binaries; per-user student data does not
  belong in a shared, admin-owned location. The helper has **no
  fallback to the exe folder** (unlike the reports/imports helpers): if
  `%LOCALAPPDATA%` can't be resolved or created, sync refuses with a
  message and writes nothing.
- **Security:** the folder inherits the user profile's ACL (the user,
  SYSTEM, Administrators; no Users/Everyone entry; checked with
  icacls). CALSS adds no ACLs. Nothing from it is packaged: the
  installer's `[Files]` names each file. Audit entries for a sync record
  block, assignment title and file count, never source contents. The
  OAuth token stays in Credential Manager, never in these files.
- **Not migrated:** existing `Documents\CALSS_Classroom_Sync` folders
  (and synced files inside other folders) are left exactly as they are.

### 8.2 Uninstall (supersedes §7.6 for Classroom data)

`[UninstallDelete] filesandordirs {localappdata}\CALSS\Classroom`, i.e.
the Classroom working copies of the **Windows account running the
uninstaller**, and nothing else under `%LOCALAPPDATA%\CALSS`. The
end-of-uninstall message now says these were deleted.

| Removed on uninstall | Kept |
|---|---|
| program files, shortcuts, Add/Remove entry | `Documents\CALSS Reports`, `Documents\CALSS Imports`, CSV exports |
| `%LOCALAPPDATA%\CALSS\Classroom` (synced submissions + manifest) | `%LOCALAPPDATA%\CALSS\calss_gc_settings.json`, `gemini_model.txt` |
| | `%PROGRAMDATA%\CALSS\audit.log` |
| | Credential Manager `CALSS/GeminiApiKey`, `CALSS/GoogleClassroomToken` |
| | old `Documents\CALSS_Classroom_Sync` folders from ≤ 1.0.0 |

Limitation: on a shared PC with an all-users install, other Windows
accounts' Classroom folders are not removed. An elevated uninstaller can
only reliably resolve the current account's profile, and deleting other
users' profile data was out of the approved scope. If a different admin
account approves the UAC prompt, `{localappdata}` is that admin's
folder.

### 8.3 Gemini model discovery

**Before:** one hard-coded model, `gemini-2.5-flash-lite`, in two
places in gemini.cpp: "Test key" (`GET /v1beta/models/gemini-2.5-flash-lite`)
and every generation (`POST …/gemini-2.5-flash-lite:generateContent`).
A third hard-coded name, "Gemini 2.5 Flash Lite", was the Overview
signature. There was no fallback; errors were "API returned status N" +
200 characters of the body.

**Why it broke:** Google's model page (ai.google.dev/gemini-api/docs/models,
read 2026-10-07) says: *"To ensure reliable performance for everyone, we
are limiting access to the 2.5 models to users who have actively used
them in the past. … For any new projects, use our latest models: 3.5
Flash-Lite or 3.8 Flash."* The deprecations page lists
`gemini-2.5-flash-lite` with no shutdown date but restricted to existing
users. A tester's new key failing with "model unavailable/not
supported" while the old key works is **consistent with that
restriction**. The tester's exact error text was not available here, so
this is the documented cause, not a reproduced one. (The developer's
key still has 2.5 access and lists it.)

**Now:**
- **Discovery:** `GET https://generativelanguage.googleapis.com/v1beta/models?pageSize=1000`
  (official `models.list`, ai.google.dev/api/models; all pages; free,
  generates nothing). Also used as the key check: 200 = valid key.
- **Compatible:** `supportedGenerationMethods` contains
  `generateContent`, `inputTokenLimit` ≥ 32,768, and the API identifier
  (`name` without `models/`, never the display name) matches one of the
  anchored patterns below.
- **Ranking (automatic):**
  1. `gemini-<N>[.<M>]-flash-lite[-NNN]`, newest version first
  2. `gemini-<N>[.<M>]-flash[-NNN]`, newest first
  3. `gemini-flash-lite-latest`, then 4. `gemini-flash-latest` (Google
     hot-swaps these, so last resort)

  Everything else is never auto-selected: Pro (cost), `-preview` /
  `-exp` / dated previews (withdrawn at short notice), TTS, image, live,
  transcribe, embeddings, Gemma, agents. Google's metadata has no cost
  field, so "low cost" is this explicit family preference, not a
  computed one. For the developer's key it picks `gemini-3.5-flash-lite`
  from 10 compatible models (of 45 that support generateContent).
- **Cache:** the list and pick are cached per key (by hash) for the
  session, under a mutex, since the dialog worker and the analysis
  thread both use them. A run makes no list call if the dialog already
  checked the key; otherwise one list call before the first generation.
- **Fallback:** if generateContent returns model-unavailable (404, or
  400/403 whose message is about the model), that model is excluded for
  the session, the list is refreshed and the next candidate is tried
  **once**. A 503 ("overloaded", seen on the capacity-limited 2.5 models)
  also tries the next candidate once, without excluding it. No other
  error is retried. Both retry cases are replies where nothing was
  generated, so cost can't multiply; `GEMINI_MAX_PAIRS` (50) and the two
  calls per run are unchanged.
- **Errors (distinct messages):** invalid key (400 `API_KEY_INVALID` /
  401 / 403) · "The API key is valid, but no compatible Gemini model is
  currently available to it" · "Gemini could not be reached" (no
  answer / timeout) · "Gemini quota or rate limit reached (HTTP 429)" ·
  "Gemini is temporarily unavailable (HTTP 5xx)" · model not available
  (with Google's message) · unexpected reply. Timeouts: list 10/10/10/15 s,
  generate 15/15/30/120 s. Core analysis is unaffected by any of them.
- **Manual choice (added):** AI Settings has a "Gemini model" dropdown:
  *Automatic (recommended)* (default) plus the compatible models for
  this key (display name + ID). Stored in
  `%LOCALAPPDATA%\CALSS\gemini_model.txt` (`auto` or an ID; not a
  secret). A manual model that later becomes unavailable falls back to
  Automatic's next candidate. It only offers models that pass the
  filter, so Pro/preview can't be chosen.
- **Threading:** AI Settings' Test, Save and the on-open check run on a
  worker thread and post `WM_AI_CHECK_DONE`. The result never carries
  the key: Save re-reads the box and compares a hash. Stale results
  (dialog closed / newer check) are dropped by a job id. 1.0.0 ran
  Test/Save on the UI thread.
- **Security:** the key goes only into the `x-goog-api-key` header to
  `generativelanguage.googleapis.com`; header and copies are zeroed after
  use; never in URLs, logs, error text or the preference file. CALSS
  never looks for keys anywhere; the user supplies one.
- **Structured output (`c1c282f`):** found while testing — with
  `gemini-3.5-flash-lite`, 1 of 4 Tier 1 runs returned an array the parser
  read as 1 element instead of 50, so every per-pair note failed for that
  run. Both calls now send `generationConfig {responseMimeType:
  "application/json", responseSchema: ARRAY of STRING}`
  (ai.google.dev/api/generate-content). **Prompt wording unchanged.**
- The Overview signature shows the model actually used.

Official sources: ai.google.dev/api/models (models.list / Model
fields), ai.google.dev/gemini-api/docs/models (model list, 2.5 access
restriction, version patterns), ai.google.dev/gemini-api/docs/deprecations,
ai.google.dev/api/generate-content (GenerationConfig).

### 8.4 Tests

**Automated / dev PC:**

| Check | Result |
|---|---|
| Build (each checkpoint, then clean at 1.1.0) | exit 0, no warnings (`-Wall -Wextra` on gemini.cpp: only the pre-existing `#pragma comment` note); GUI subsystem; no MinGW DLLs |
| Golden check after `637d94a`, `79fcd42`, `c1c282f`, 1.1.0 | all three datasets byte-identical |
| Error classification (synthetic replies) | 200, no answer, 400 API_KEY_INVALID, 403 API disabled, 404 model not found, 400 "no longer available", 429, 503, generic 400 → all classified as intended |
| Model filter on 16 real IDs | accepts flash-lite/flash/-latest; rejects pro, preview, tts, image, gemma, transcribe, omni, nano-banana, pro-latest |
| Invalid key (live) | "Google rejected this API key (HTTP 400)…", rejected flag set |
| Developer's key (live) | valid; 10 compatible models; Automatic = `gemini-3.5-flash-lite` |
| Fallback (live) | non-existent model injected first → real 404 → excluded, list refreshed, retry on `gemini-3.5-flash-lite` succeeded; exactly one retry |
| Tier 1 + Tier 2 with structured output (live) | `gemini-3.5-flash-lite`: 50/50 notes, 3 findings; `gemini-2.5-flash-lite` (manual): 503 once (Google capacity), then 50/50 and 3 |
| GUI, repo build | AI run on Programming1: summary signed `gemini-3.5-flash-lite`, Pair links work, per-pair AI note shown; AI Settings: saved key confirmed in background, dropdown filled, Automatic shown; manual choice saved/reloaded/reset (audit "AI MODEL SETTING"); invalid key via Save rejected, not saved, saved key intact |
| Installer 1.1.0, only-for-me, silent | 13 files + uninstaller; version 1.1.0 in exe and Add/Remove; About "Version 1.1.0 (Thesis Release)"; installed AI Settings shows the discovered model |
| Classroom location (simulated sync data) | synthetic dataset placed in `%LOCALAPPDATA%\CALSS\Classroom`; installed CALSS loads and analyzes it: 705 of 2,186, 35 students; folder ACL = profile default |
| Uninstall | deleted 578 entries: program, 2 shortcuts, 559 under `…\CALSS\Classroom`, **nothing else** (checked line by line in the uninstall log). Kept: `gemini_model.txt`, Documents\CALSS Reports (report generated just before), old `Documents\CALSS_Classroom_Sync`, audit log, both credentials |
| Reinstall | works; AI Settings again shows Automatic = gemini-3.5-flash-lite |
| Secrets | no `AIza`, `ya29.` or refresh-token pattern in the exe; installer file list explicit |

Test data created for this (dataset copies, a report, the model
preference file) was deleted afterwards; audit entries remain by design.

**Not tested here (needs people/accounts):** a real Classroom sync into
the new folder (needs an approved test-user sign-in); the tester's new
Gemini key (CALSS must pick a non-2.5 model for it); "no key" on a
fresh account (code path unchanged since 1.0.0); real network outage
and real 429 quota (classification only tested on synthetic replies);
all-users install/uninstall of 1.1.0 (the 1.0.0 all-users path passed;
the only installer change is `[UninstallDelete]`); another PC.

### 8.5 Artifacts

| File | Size (bytes) | SHA-256 |
|---|---:|---|
| `release\CALSS_Setup_1.1.0.exe` | 3,708,306 | `55D78824B5CAAC0C5721C106C9172012FB412677053006FF18BB82173686D1F2` |
| `authorship.exe` 1.1.0 (inside) | — | `A3764F21F0135094B3FC71C14F8745F638B85076CEF079064E0D3ABF9558254C` |
| `release\CALSS_SampleData_SYNTHETIC.zip` | 563,768 | `534D30B92CC79B04032B8DE555EF0EFC53E59276D771F8FF2DDAEEEC0949B263` (unchanged) |

Send testers: `CALSS_Setup_1.1.0.exe`, `CALSS_Setup_1.1.0_SHA256.txt`,
`CALSS_SampleData_SYNTHETIC.zip`, `release_docs\INSTALLER_TEST_CHECKLIST.txt`
(updated for 1.1.0: model check in item 19, Classroom path in item 20,
Classroom cleanup in item 22).

## 9. CALSS 1.2.0 — Overview AI summary organized by class (2026-10-08)

Branch `optimization-pass`. Commits: `0924c30` (grouping + Gemini
per-class summaries), `98ff059` (Overview UI), `3bbd899` (version
1.2.0), then this documentation. Golden check byte-identical after each
source commit; no methodology code touched. 1.0.0 and 1.1.0 installers
are kept unchanged next to the new one.

### 9.1 Why

An interviewed professor: *"…It would also be helpful if the analyses
could be organized by subject or class, especially when working with
different subjects."* The team clarified this means the AI analysis on
the Overview. (The "continue where I left off" history part of the same
feedback is **not** in this release; it came in 1.3.0, §10.)

### 9.2 Before / after

**1.1.0 Overview AI area:** one "AI INSTRUCTOR SUMMARY" block: 2–4
numbered cross-pair findings from one Gemini request (Tier 2) over the
top 50 flagged pairs, with clickable "Pair N" links, and the model
signature. No notion of classes; nothing in this area when AI was off,
missing or failed except a message.

**1.2.0 Overview AI area (stats, headline and language notice above it
are unchanged):**
1. **AI INSTRUCTOR SUMMARY** — same block, now 1–3 short *overall*
   findings (same rendering, links and signature; same off / no-key /
   failed messages).
2. **NOTABLE CASES** — only when there are 2+ classes: the top 5 flagged
   pairs in CALSS's own order (flaggedPairs, i.e. combined score, the
   same order as the Flagged pairs tab), each with its class. No new
   metric, threshold or AI ranking. Hidden with one class because it
   would repeat that class's list.
3. **FLAGGED PAIRS BY CLASS** — one collapsible section per class:
   ▾/▸, class name, *N flagged pairs · highest X%* (CALSS); when open:
   the Gemini summary for that class (gold rule, "Pair N" links) or its
   state, then the class's top 5 pairs, and "Showing the top 5 of N in
   this class. Open Flagged pairs ›".

Every pair row (score in severity colour, its #N in Flagged pairs,
*A ↔ B*, class context, *View ›*) uses the existing
`g_aiSummaryLinkRects` → `jumpToFlaggedPair` path, the same as the
"Pair N" links: it opens Flagged pairs with that pair expanded and
scrolled to. Rows and headers are keyboard focusables (`AISummaryLink`,
new `OverviewGroupToggle`) and get the roster-style hover highlight.
Default: all sections open with ≤ 3 classes, otherwise only the first;
reset after each run. Toggling only repaints — no analysis, no network.
Notable cases + class sections are CALSS data and are drawn whether AI
is off, missing, failed or partial.

### 9.3 Academic grouping used

CALSS already has one grouping: the **block** stored per file in the
data folder's `calss_sync_manifest.jsonl` — Classroom course name +
section after a sync, or the subfolder name for manually organized
folders. The Classes tab shows exactly these as its block chips, and
students without one as "General". 1.2.0 uses the same field and names
(UI wording "class", like the tab):
- `PairAnalysisDisplay::blockA/blockB` = each file's block (pipeline,
  `readManifestBlocks`, same file/field as `gc_loadSyncManifest`).
- `AnalysisResults::flaggedGroups` (`buildFlaggedGroups`): both files in
  the same block → that block (or "General"); files in two different
  blocks → one **"Across classes"** group (CALSS compares across
  blocks, so these pairs exist and are labeled honestly instead of being
  forced into one side). Order: block names A–Z, General, Across
  classes; pairs in flaggedPairs order. Every flagged pair is in exactly
  one group.
- There is no separate "subject" field in CALSS; a block name such as
  "Programming 1 (C Language) - BSIT Block 1" already carries the
  subject when it comes from Classroom.

### 9.4 CALSS vs Gemini

| From CALSS (deterministic) | From Gemini (optional text) |
|---|---|
| class names, grouping, order, membership; flagged counts; highest score; every pair's score, #N, names; Notable cases selection; all links | overall findings (1–3 strings); one summary per class (matched by id) |

### 9.5 Gemini request, schema and prompt

- **Requests:** unchanged — **2 generateContent per run** (Tier 1 per-pair
  notes for the top 50, Tier 2 overview), plus the 1.1.0 models.list on
  first use and at most one model-fallback retry per call. 1.1.0 had the
  same worst case. No per-class or per-pair requests were added.
- **Tier 2 input:** at most 10 classes (`GEMINI_MAX_GROUPS`) and at most
  `GEMINI_MAX_PAIRS` (50) pairs in total: each class's top
  max(3, 50 / classes) pairs, in CALSS order, with the same per-pair
  deviating-feature lists as before. Classes beyond 10 show "AI
  summaries cover the first 10 classes".
- **Schema before (Tier 2):** `ARRAY of STRING` (2–4 findings).
  **After:** `OBJECT {overall: ARRAY of STRING (1–3), groups: ARRAY
  (min = max = classes sent) of OBJECT {id: STRING, summary: STRING}}`,
  both required. Ids `G1…Gn` are assigned by CALSS; summaries are
  mapped back by id (unknown/duplicate/empty entries ignored). The
  min/max was added because the model once returned 4 of 5 classes.
  Tier 1's schema is unchanged.
- **Prompt changes (`buildOverviewPrompt`, replaces `buildCrossPairPrompt`):**
  kept: base only on the data; never "cheated"/"plagiarized"; cite pairs
  as "Pair N" from the headers; cite a limited number; self-contained,
  plain text. Added: role framing ("CALSS has already computed every
  score, flag and grouping… only explain them"); the class structure
  (`=== G1: <name> -- N flagged pairs (showing the top K) ===`); WRITE
  section for `overall` and `groups`; scores are not probabilities of
  misconduct, never state or imply guilt, also no
  "cheating"/"plagiarism"; do not rank students; no new statistics;
  summaries 1–2 sentences and don't repeat the class name/count; refer
  to classes by name, never by id (the model wrote "the single class
  G1" once). Tier 1's prompt is unchanged.

### 9.6 Failure behavior

| Situation | Overview |
|---|---|
| AI not requested / no key | existing message in the summary block; class sections without AI lines |
| Gemini failed (network, quota, 5xx, bad key, no model) | existing "AI summary unavailable: …" message; each class: "AI summary unavailable for this class." |
| overall present, a class missing | that class: "No AI summary was returned for this class." |
| classes present, overall missing | "No overall summary was returned. The class summaries are below." |
| > 10 classes | classes 11+: "No AI summary: AI summaries cover the first 10 classes." |
| 0 flagged pairs | no Notable / class sections (nothing to show), no Gemini request |
| 1 flagged pair | Tier 2 skipped as before (fewer than 2 pairs); the class section still lists it |

### 9.7 Tests

| Check | Result |
|---|---|
| Build (each checkpoint + clean 1.2.0) | exit 0; no new warnings (`-Wall -Wextra` caught a `"\x01across"` hex-escape bug, fixed before commit); GUI subsystem; no MinGW DLLs |
| Golden check after `0924c30`, `98ff059`, 1.2.0 | all three datasets byte-identical |
| Pair-to-group correctness (copies of Programming1 and the four-block set, independent Python check against the manifest **and** each file's physical subfolder) | 705/705 and 27,991/27,991 pairs each in exactly one group; blocks correct; group rule correct; counts correct; score order kept |
| Groups found | Programming1: 1 class (BSCS-7, 705). Four blocks: BSCS-3A 1,587 · 3B 1,874 · 3C 1,313 · 3D 2,152 · Across classes 21,065 |
| Live Tier 2 (developer key, gemini-3.5-flash-lite) | 12 k-char prompt, exactly 50 pairs; all 5 class summaries returned in 2 of 2 runs after min/maxItems; cited pairs (#7, #85 for BSCS-3A) verified to belong to that class |
| GUI, repo build, AI off | hierarchy renders; counts match the dump; rows #26/#34/#43/#81/#95 under BSCS-3B match the dump exactly; expand/collapse works; clicking #34 opens Flagged pair #34 FloresRowena ↔ MarquezJustine expanded |
| GUI, AI on | overall findings + per-class summaries with working "Pair N" links; single-class run: no Notable cases, one open section |
| AI failure / partial (dev-only build with a stub Gemini module, not shipped) | deterministic sections intact; per-class "unavailable" / "not returned" notes correct; "Pair 4" link in a class summary opens pair #4 |
| Installed 1.2.0 (only-for-me, silent) | 13 files; version 1.2.0; AI run on Programming1 in the installed copy (no id leak after the prompt rule); zero-pair run shows no empty sections |
| Uninstall / reinstall | program + shortcuts + simulated `%LOCALAPPDATA%\CALSS\Classroom` removed; imports, audit log, both credentials kept; reinstall works, same results |
| Secrets | none in the exe |

Test data created for this was deleted afterwards.

**Not tested here:** another PC; a real Classroom-synced dataset with
long course names (only subfolder-named blocks were available); more
than 10 classes; keyboard-only navigation of the new rows (registered as
focusables, not exercised); the all-users install of 1.2.0 (installer
unchanged since 1.1.0).

### 9.8 Limitations

- The "Across classes" group can dominate when blocks are compared with
  each other (21,065 of 27,991 in the synthetic four-block set); it is
  shown as its own section rather than hidden.
- A class's section lists its top 5; the full list is the Flagged pairs
  tab, which has no class filter yet.
- Gemini text quality varies by run and model; summaries are
  explanatory only and can still be generic.
- Students' class on the Classes tab is their *first* file's block,
  while the Overview groups by each pair's *files*; they agree whenever
  a student's files are all in one block (true for every dataset here).

### 9.9 Artifacts

| File | Size (bytes) | SHA-256 |
|---|---:|---|
| `release\CALSS_Setup_1.2.0.exe` | 3,717,529 | `C160D29B2C844F668CCD6838466C8EDE22958BC3575CDA01056D09061C3453EF` |
| `authorship.exe` 1.2.0 | 4,772,435 | `1C231D8CF32253556E1EE65FA7618C3C29D53239A7F46BBAFC08609986363E11` |


## 10. CALSS 1.3.0 — Continue previous session; Comparative panel removed (2026-10-12)

Branch `optimization-pass`. Commits: `48a0f29` (workspace state file +
unit tests), `b5f2e18` (Continue previous session UI and restore),
`9adfcf1` (Comparative Authorship Analysis panel removed), `5650c5c`
(unreachable Copy findings code removed), `e0b0c2a` (version 1.3.0),
`d6a6074` (installer uninstall message), then this documentation.
No methodology code touched (tokenizer, features, KNN, similarity,
weights, thresholds, pipeline all unchanged); golden check
byte-identical. The 1.0.0–1.2.0 installers are kept unchanged.

### 10.1 Why, and what "history" means here

An interviewed professor: *"I would suggest adding a history feature,
if it's not already included. It would be nice to be able to go back to
previous work and continue where I left off."* The team scoped this to
**the most recent session only**: close CALSS, reopen it, choose to
continue, and land back on the same dataset and class / student / pair.
Not implemented on purpose: a list of past analyses, multiple saved
sessions, a database, or stored results. Reasons: it is what the
professor described ("continue where I left off"), it fits the two
weeks left, and storing old results would risk showing scores that no
longer match the files.

Separately, the team asked to remove the maroon **Comparative
Authorship Analysis** panel (with its Copy findings button) at the end
of each expanded flagged pair — redundant with the head-to-head
comparison above it.

### 10.2 What is saved

`%LOCALAPPDATA%\CALSS\workspace.json` (per Windows user, the profile's
default private ACL; same folder as `calss_gc_settings.json` and
`gemini_model.txt`). A flat, versioned JSON object:

| Field | Meaning |
|---|---|
| `format`, `version` | `"calss-workspace"`, 1. A higher version (newer CALSS) is ignored and left on disk. |
| `app_version`, `saved_at` | CALSS version and local time that wrote it (display only) |
| `dataset` | UTF-8 full path of the analyzed folder |
| `file_count` | its `.c` file count then (only used to say "the folder now has N files") |
| `page` | `overview` / `classes` / `flagged` |
| `block`, `student` | Classes: the selected block chip name ("" = All) and student name ("" = roster) |
| `pair_file_a`, `pair_file_b`, `pair_label` | Flagged Pairs: the last pair the professor opened (file names, matched in either order) and "NameA vs NameB" for the card |

Never saved: scores or any analysis results, source code, Gemini key,
Google tokens, AI text. Identifiers are names (the pipeline's own
student identity, block names, pair file names), never list indices.
Scroll position is not saved (the flagged pair is scrolled to; other
pages open at the top).

**When it is written:** `workspaceSaveIfChanged()` runs after every
content paint and on exit, rebuilds the navigation above, and writes
only when it differs from the last write — and only while the screen
shows a finished, valid analysis of the loaded folder (never during a
run, after a cancel/error, or after a new folder was selected but not
yet analyzed — the previous valid session is kept then). Writes are
atomic (`.tmp` + `MoveFileExW(REPLACE_EXISTING | WRITE_THROUGH)`), so a
crash mid-save leaves the old file. A failed write is not retried every
paint.

**Reading:** strict bounded parser (64 KB, strings/integers only,
escapes and surrogate pairs decoded, control characters rejected, path
≤ 4096 bytes, names ≤ 512). Malformed, invalid, future-version or
missing → no card, CALSS starts normally; the file is left until the
next finished analysis replaces it.

### 10.3 Startup and restore lifecycle

1. Startup (`workspaceInitAtStartup`, before the first paint): a usable
   file → the empty-state screen shows **CONTINUE PREVIOUS SESSION**
   above the three source cards: *Dataset* (folder name · N files),
   *Last location* (e.g. `Classes → BSCS-7 → AquinoAaron`), *Last used*,
   with **Continue** and **Start new**. Nothing happens automatically.
   Mouse, hover and keyboard (Tab/Enter) use the existing empty-state
   card mechanism.
2. **Start new** hides the card for this launch; the file is unchanged
   and is replaced by the next finished analysis.
3. **Continue** checks the folder: exists, is a folder, has `.c` files
   (same counter as Select folder). If not → a modal, the card goes
   away, CALSS stays usable:
   - missing / renamed / moved / inaccessible: *"The folder can't be
     found…"* + shortened path, buttons **Select folder…** / Close;
   - no `.c` files / not a folder: same modal, matching reason;
   - inside `%LOCALAPPDATA%\CALSS\Classroom` and missing (e.g. after
     uninstall): *"The Google Classroom submissions for "…" are no longer
     on this computer…"*, buttons **Sync Classroom** / Close. Sync opens
     only if clicked.
4. Otherwise the folder is loaded exactly like Select folder and the
   **normal analysis runs** (`startAnalysisRun`, the body of
   `onRunAnalysis`) on the worker thread with the usual progress screen
   and Cancel. Deterministic results are recalculated from the current
   files; nothing from the old session is reused.
5. On `WM_ANALYSIS_COMPLETE` (`workspaceOnAnalysisComplete`): if the run
   is valid and for that folder, the saved page is opened and each saved
   item is looked up by name in the new results — block chip
   (`blockTabIndexForName`, same list as the chip bar), student
   (`profiles[].name`), flagged pair (`jumpToFlaggedPair`: expanded and
   scrolled to). All found → toast *"Previous session restored. The
   analysis was re-run on the current files with the AI summary off."*
   (+ *"The folder now has N files (it had M)."* if changed). Something
   missing → **Session partly restored** modal listing it (e.g. *the
   student "AquinoAaron"*, *the flagged pair … (it is no longer
   flagged)*); the rest is restored. Cancelled / failed run → toast,
   nothing restored.
6. Closing CALSS during the restore run behaves like closing during any
   run; the saved file is untouched (it is only written for finished
   runs).

### 10.4 Gemini, network and Classroom during restore

- The restore run always uses `useAI = false`, even with a key saved
  and "Include AI summary" ticked: **no Gemini request, no quota**,
  works offline and with no key or no valid model. The Overview and
  pair notes show the existing "AI summary not requested" states. To
  get AI text the professor ticks the box and presses Run analysis (the
  normal path). Old AI text is never shown — it was never saved.
- No Google sign-in, Classroom refresh or sync is started; an existing
  Classroom download folder is just read like any folder. The 1.1.0
  storage policy (`%LOCALAPPDATA%\CALSS\Classroom`) is unchanged.
- Audit log: the restore's `ANALYSIS STARTED` entry adds
  `Continuing previous session: Yes` (and shows `AI summary requested:
  No`). The workspace file itself is never logged.

### 10.5 Privacy and uninstall

`workspace.json` can reveal a folder path, class and student names. It
is per Windows user, in the user's own `%LOCALAPPDATA%`, not packaged,
not logged. Uninstall keeps it under the existing settings-retention
policy (like `calss_gc_settings.json`); the uninstall message now lists
`%LOCALAPPDATA%\CALSS` as kept ("settings and the last session: folder
path, class and student names, no scores or code") so a professor on a
shared PC knows to delete it. `[UninstallDelete]` is unchanged —
still only the Classroom subfolder. After an uninstall removed the
Classroom downloads, a reinstalled CALSS offers the old session and
Continue shows the missing-Classroom modal (tested, 10.8).

### 10.6 Comparative Authorship Analysis panel removed

Removed from each expanded flagged pair: the maroon box, its
"COMPARATIVE AUTHORSHIP ANALYSIS" heading, the interpretation text, the
in-panel disclaimer and the **Copy findings** button
(`drawComparativeBox`, `measureComparativeBoxHeight`; the box's height
and gap left `pairExpandedHeight`/`drawFlaggedPairBlock`, so the band
now ends 24 px below the AI Evidence Note — no empty area; scroll
extents and hit rects follow). Copy findings had no other entry point,
so its code went too (`copyFindingsForPair`, `buildFindingsText`,
`copyTextToClipboardW`, its rects/feedback globals/timer and
`FocusKind::CopyFindingsBtn`) — no invisible click or Tab target
remains. **Kept:** the AI EVIDENCE NOTE (unchanged, incl. its no-key /
not-requested / failed states), the comparison table, View source, all
scores, the page footer disclaimer, CSV export, the per-student report.
`PairAnalysisDisplay::interpretation` is still computed by the pipeline
(not displayed anywhere now). Gemini prompts/schemas untouched.

### 10.7 Code

| File | Change |
|---|---|
| `include/workspace_state.h`, `src/workspace_state.cpp` (new) | file format, parser, atomic save, pure decisions (`classifyDataset`, `pathIsInside`, `findByName`, `findPair`, `describeLocation`) |
| `src/gui_workspace.cpp` (new) | startup card, Continue / Start new, restore after analysis, save-on-change |
| `src/gui.cpp` | `onRunAnalysis` → `startAnalysisRun(useAI, restoring)`; hooks in `WM_ANALYSIS_COMPLETE`, content `WM_PAINT`, `WM_DESTROY`, `runGUI`, `togglePairExpanded`, `jumpToFlaggedPair`; menu ids 5006/5007; Copy findings code removed |
| `src/gui_students.cpp` | `computeBlockTabNames()` shared by the chip bar and `blockTabIndexForName` / `blockTabNameForIndex` |
| `src/gui_welcome.cpp` | card above the source cards; hover index counts the card's buttons |
| `src/gui_flagged.cpp` | Comparative box removed |
| `include/gui_common.h` | prototypes / ids; Copy findings declarations removed |
| `compile.bat` | + `src/workspace_state.cpp`, `src/gui_workspace.cpp` (24 units) |
| `tests/workspace_state_test.cpp`, `tests/run_workspace_tests.bat` (new) | 89 unit checks |
| `include/version.h` | 1.3.0 |
| `installer/calss.iss` | uninstall message line only |

### 10.8 Tests

GUI tests were driven by a script (launch the exe, post clicks/keys to
the content window, capture the window with `PrintWindow`) and checked
on the screenshots, on synthetic data copied to a temp folder (the
committed datasets were not touched).

| Check | Result |
|---|---|
| Build (`compile.bat`, each checkpoint, `-Wall -Wextra` on changed files) | exit 0, no new warnings; imports only Windows DLLs + UCRT |
| Golden check after the session work and on the final tree | all three datasets byte-identical (705 / 662 / 27,991 flagged) |
| `tests\run_workspace_tests.bat` | 89 checks, 0 failed |
| Fresh launch, no file | normal welcome screen, no card |
| Analyze → Overview → close/reopen → Continue | Overview, re-run (705 of 2,186), toast; 0 TCP connections from the process during the restore (key saved, AI box ticked) |
| Classes → BSCS-7 → AquinoAaron → close/reopen | card "Classes → BSCS-7 → AquinoAaron"; Continue lands on the student; Back shows BSCS-7 selected |
| Flagged Pairs, expand pair #3 → close/reopen → Continue | Flagged Pairs, pair #3 expanded and scrolled to |
| Start new | card hidden, nothing loaded, file byte-identical |
| Dataset folder renamed | "Previous session unavailable" + Select folder…; app usable |
| 16 files of one student removed | re-run (537 files, 34 students, 650 flagged); "Session partly restored" names the student and 537 vs 553; block restored |
| Corrupt file (binary garbage), future version 2 | no card, no crash, file left in place |
| Restore with AI on/no network | restore never requests AI (audit `AI summary requested: No`); no connections observed. Not tested with the network adapter actually disabled. |
| Classroom-owned folder (synthetic copy inside `…\CALSS\Classroom`) | restores (662 of 1,740), no connections; deleted → missing-Classroom modal, nothing recreated or opened |
| Normal Run analysis after a restore | saves the new state (Overview) |
| Flagged Pairs after removal | panel and button gone, AI note kept, no gap; clicking where the button was does nothing; next pair expands; View source opens; Tab cycling works |
| Help > About | "Version 1.3.0 (Thesis Release)" |
| Installed 1.3.0 (only-for-me, silent) | 15 files (13 + uninstaller), no workspace/credentials/logs; exe hash matches; restores student and flagged pair |
| Uninstall | program, shortcut, uninstall entry, Classroom folder removed; `workspace.json`, settings, audit log, both Credential Manager entries kept; message lists the kept folder |
| Reinstall | card back; Classroom-session → missing-Classroom modal; normal restore works |

The developer's real Classroom downloads were backed up before the
uninstall tests and restored afterwards (74/74 files hash-identical);
test files and the test `workspace.json` were deleted.

**Incidents during testing.** (1) A test script failed to untick "Include
AI summary" and started one normal AI run on the developer's key
(synthetic data); it was closed during "Generating AI summary" — normal
app behavior for a ticked box, not a restore path. (2) In 1 of 14
keyboard (Tab) stress runs the test instance exited after its restore
had completed; no crash was recorded by Windows and 13 repeat runs with
varied timing did not reproduce it. Watch for it in cross-PC testing.

**Not tested here:** another PC; the all-users install of 1.3.0
(installer otherwise unchanged since 1.1.0); a real Classroom sync
followed by restore; restore with the network adapter disabled;
selecting a folder through the folder dialog in the scripted runs (the
restore path loads folders itself; the dialog code is unchanged).

### 10.9 Limitations

- One session only, by design; starting a new analysis replaces it.
- Restore re-runs the whole analysis (seconds for a class; ~70 s for
  the 2,973-file four-block set) — correct but not instant.
- AI text is not restored; it needs a normal Run analysis with the box
  ticked (two Gemini requests, as before).
- Scroll position, filters, search text, sort order and which other
  pairs were expanded are not restored.
- A dataset on a disconnected drive shows the "can't be found" modal;
  reconnecting the drive and reopening CALSS offers it again.
- `PairAnalysisDisplay::interpretation` is still computed but no longer
  shown anywhere.
- Pre-existing, not changed: `g_expandedPairs` is not cleared between
  runs in one CALSS session (indices from the previous run stay
  expanded).

### 10.10 Artifacts

| File | Size (bytes) | SHA-256 |
|---|---:|---|
| `release\CALSS_Setup_1.3.0.exe` | 3,734,986 | `F6059A8AE9CFF609CB729750A2670B69A9BEBCD9849EE0D2AE78BB363F5B617B` |
| `authorship.exe` 1.3.0 | 4,831,172 | `7A9D5B11E3A9159476B2D35BC7E551B54CEC923BC4AF5A69ED80D74CE70FD90A` |

Send testers: `CALSS_Setup_1.3.0.exe`, `CALSS_Setup_1.3.0_SHA256.txt`,
`CALSS_SampleData_SYNTHETIC.zip`, `release_docs\INSTALLER_TEST_CHECKLIST.txt`
(updated for 1.3.0: items 15 and 18a–18e, 23, 24).
