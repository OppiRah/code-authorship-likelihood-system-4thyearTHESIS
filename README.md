# CALSS — Code Authorship Likelihood Scoring System

A Windows desktop tool that helps programming instructors review student
C programming submissions for **code similarity** and **coding-style
(authorship-related) anomalies**.

Undergraduate thesis project, College of Computer Studies, University of
Luzon. **Current and final release: CALSS 1.3.1** (development frozen).

> **CALSS is a decision-support tool. It does not establish plagiarism,
> cheating or academic misconduct.** A flagged pair or a low authorship
> score only means that a submission may deserve a closer look. Every
> result must be reviewed by the instructor with the source code and
> other academic evidence before any conclusion is drawn.

## Download

**[CALSS v1.3.1 — Final Thesis Release](https://github.com/OppiRah/code-authorship-likelihood-system-4thyearTHESIS/releases/tag/v1.3.1)**

| Asset | What it is |
|---|---|
| `CALSS_Setup_1.3.1.exe` | Windows installer (64-bit Windows 10/11). **Not code-signed.** |
| `CALSS_1.3.1_Lab_Deployment.zip` | Full laboratory package: the same installer, installation guide (PDF), README, release notes, checksums and the sample data. |
| `CALSS_SampleData_SYNTHETIC.zip` | Demonstration datasets (see [Sample data](#sample-data)). |
| `CALSS_1.3.1_SHA256SUMS.txt` | SHA-256 of the three files above. |

Check the installer before running it:

```powershell
Get-FileHash .\CALSS_Setup_1.3.1.exe -Algorithm SHA256
# expected 947C8E2CA0A357117D756A44538798C1A7DCE12C4760879FAEBC0B001BA0BABA
```

Because the installer is unsigned, Windows SmartScreen may warn about it.
That warning is about reputation, not a malware detection. Verify the hash,
scan the file with Microsoft Defender, and follow your institution's IT
policy. Do not disable SmartScreen, Defender or UAC.

## What it does

- Loads `.c` submissions from a folder (one level of class/block
  subfolders), from imported files, or by optional Google Classroom sync.
- **Token similarity** between submissions: Jaccard and Longest Common
  Subsequence on the tokenized code.
- **Stylometric profile** per student: 14 coding-style features (naming,
  comments, brace placement, spacing, loop preference, function length…)
  built from activity submissions.
- **Authorship likelihood**: a K-nearest-neighbour comparison (K = 3) of
  an exam submission against the student's own profile.
- Views: Overview (grouped by class), Classes (style "DNA" per student),
  Flagged pairs (filter, search, side-by-side read-only source view).
- CSV export, per-student HTML similarity report, append-only audit log.
- **Continue previous session**: re-opens the most recent folder and
  re-runs the analysis (AI off).
- Optional **Gemini** explanations of the flagged results.

## Technology

C++17, Win32 API and GDI+ — no UI framework and no third-party runtime.
Built with MinGW-w64 g++ as one static executable. Secrets (Gemini key,
Classroom sign-in) are stored in Windows Credential Manager. Installer:
Inno Setup 6.

## Repository layout

| Path | Contents |
|---|---|
| `authorship-cpp/src`, `include` | Application source and headers |
| `authorship-cpp/res`, `assets` | Icon, manifest, version info, help images |
| `authorship-cpp/installer` | Inno Setup script and installer information page |
| `authorship-cpp/tests` | Golden-output regression check, workspace unit tests, manual smoke checklist |
| `authorship-cpp/docs` | Architecture, release readiness record, Google OAuth production-readiness research |
| `authorship-cpp/release_docs` | User/lab documentation sources used by the packaging scripts |
| `authorship-cpp/website` | **Draft** information site (see [Status](#status)) |
| `stress_test`, `stress_test_bscs7` | Demonstration / validation datasets with answer keys |

## Building

Requirements: MinGW-w64 (`g++`, `windres`) on `PATH`.

```bat
cd authorship-cpp
compile.bat
```

This produces `authorship.exe` (GUI subsystem, statically linked). The
installer and the lab package are built with `make_installer.bat` (needs
Inno Setup 6) and `make_lab_package.bat` (needs Python with `reportlab`).
Classroom sync needs a Google OAuth **desktop** client file named
`calss_gc_embedded_credentials.json` next to the exe; it is not in this
repository (see `calss_gc_embedded_credentials.example.json`).

## Tests

```bat
cd authorship-cpp
tests\run_golden.bat            :: full pipeline (AI off) on the datasets, byte-compared with tests\golden\expected
tests\run_workspace_tests.bat   :: unit tests for the saved-session parser
```

Manual GUI smoke checklist: `authorship-cpp/tests/README.md`.

The golden check proves that results are **reproducible and unchanged**
between versions. It does **not** measure how accurately CALSS detects
copying. Accuracy against the datasets' answer keys is a separate,
thesis-level evaluation.

## Sample data

`stress_test/` (145 students, 4 blocks) and `stress_test_bscs7/` (30
students, Programming 1 and 2) were generated for testing
(`stress_test_bscs7/generate.py`). They have planted copy cases and a
per-file answer key in `manifest.csv`. They also include a small number of
submissions from **five thesis-team test accounts**, synced from a test
Google Classroom (`AMBEGUIAJansen_Rence_L`, `EthanSandoval`,
`PARASDivino_Dale_F`, `Rah`, `RanueeeOmega`). These are not students, and
they are not in the answer keys. **None of the data is real student work.**

To try it: install CALSS, unzip `CALSS_SampleData_SYNTHETIC.zip`, choose
**Select folder**, pick **one** subject folder (for example
`Programming2`), untick **Include AI summary** and click **Run analysis**.

## Optional integrations

**Google Classroom sync.** This feature reads course lists, coursework,
submissions and the roster, and downloads `.c` attachments through
`drive.readonly`. Google classifies that scope as *restricted*. The
OAuth app is in Google's **Testing** mode. Only Google accounts that the
team has added as test users can sign in, and the sign-in expires after
about 7 days. **The app has not been verified by Google**, and the
production verification has not been started. See
`authorship-cpp/docs/GOOGLE_OAUTH_PRODUCTION_READINESS.md`. Folder import
works without Google.

**Gemini.** This feature is off unless an instructor saves their **own**
API key (File › AI Settings) and ticks *Include AI summary*. When it is
on, student names, class labels, scores and style-feature values for up
to 50 flagged pairs are sent to Google. With a free-tier key, Google may
use that content to improve its products. The core analysis works
fully offline without Gemini.

## Known limitations

- Results depend on how many activity submissions a student has; short or
  few submissions give weak style profiles.
- Students who solve the same small problem can naturally look similar.
- Google Classroom: Testing mode only, fixed `localhost:8080` callback, no
  PKCE, no sign-out button, no pagination beyond 30 courses / 30
  assignments / 100 submissions.
- The installer is not code-signed. Windows 10 and minimum hardware have
  not been formally tested.
- More in `authorship-cpp/docs/RELEASE_READINESS.md` and the release
  notes.

## Status

- **Desktop application: complete.** CALSS 1.3.1 is the final thesis
  release, deployed with the laboratory administrator's approval on
  approved lab instructor PCs.
- **Google OAuth production verification: not done.** Classroom sync
  stays in Testing mode.
- **`authorship-cpp/website/` is a draft.** Its privacy policy and terms
  have **not** been approved by the project owner or the university, and
  the site is **not deployed**.
