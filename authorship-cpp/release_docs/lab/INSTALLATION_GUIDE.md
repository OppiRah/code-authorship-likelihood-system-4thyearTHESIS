# CALSS {{VERSION}} - Laboratory Installation Guide

For authorized laboratory administrators. Package built {{DATE}}.

CALSS (Code Authorship Likelihood Scoring System) is an undergraduate thesis project of the College of Computer Studies, University of Luzon. This guide covers installing it on approved instructor/admin computers.

## 1. Purpose and deployment scope

CALSS is an academic decision-support application. It analyzes student C programming submissions (.c files) for code similarity and coding-style (authorship-related) anomalies, and lists submissions that may warrant further review by the instructor.

CALSS does **not** establish plagiarism, cheating or misconduct. A flagged pair or a score is a reason to review the work. Results must be interpreted by an instructor together with other relevant academic evidence.

Deployment scope, as approved by the laboratory head administrator:

- Install on: the instructor/admin computer at the front of each approved programming laboratory.
- Do not install on: student laboratory computers, every workstation, personal student devices, or any computer not covered by the approval.

## 2. Prerequisites

- 64-bit Windows 10 or Windows 11. Setup refuses to run on older or 32-bit Windows.
- About 10 MB of free disk space for the program, plus space for the submissions analyzed and reports created.
- An administrator account for the recommended all-users installation (asked once by Windows). CALSS itself runs as a normal user and never asks for administrator rights.
- No other software: no runtime, framework or driver is installed. CALSS installs no service and no background task.
- Internet access only for the optional Google Classroom and Gemini features.
- Minimum processor and memory (RAM) requirements have not been formally measured. CALSS was developed and tested on ordinary Windows 11 laptops.

## 3. Package contents

| File | Purpose |
|---|---|
| {{INSTALLER}} | The installer ({{SIZE}} bytes, not code-signed) |
| SHA256SUMS.txt | SHA-256 checksums of the files in this package |
| INSTALLATION_GUIDE.pdf | This guide |
| README.txt | Short overview |
| RELEASE_NOTES.txt | What is in version {{VERSION}} and known limitations |
| {{SAMPLE}} | Synthetic test submissions (not real students) for the acceptance check |

Obtain the package only from the official distribution: the Google Drive link provided by the CALSS thesis team. Do not use copies passed on by others.

## 4. Verify the installer before running it

Confirm all of the following before running Setup:

1. The file name is exactly **{{INSTALLER}}** and its size is **{{SIZE}} bytes** (right-click > Properties).
2. The SHA-256 checksum matches (section 5).
3. Microsoft Defender finds no threat (section 6).

If any check fails, do not run the file. Delete it, download the package again from the official link, and contact the thesis team.

## 5. SHA-256 verification

The expected SHA-256 of {{INSTALLER}} is:

`{{SHA256}}`

The same value is in SHA256SUMS.txt. To check it, open PowerShell in the download folder and run:

`Get-FileHash .\{{INSTALLER}} -Algorithm SHA256`

Compare every character of the result with the value above. Because the installer is not code-signed, this checksum is how you confirm it is the exact file the thesis team released and that it was not changed or damaged in transit. Also confirm the checksum against the value the team publishes with the official link, not only against the copy inside the downloaded package.

## 6. Microsoft Defender scan

1. Make sure Microsoft Defender is on and up to date: Windows Security > Virus & threat protection > Protection updates > Check for updates.
2. In File Explorer, right-click {{INSTALLER}} > **Scan with Microsoft Defender** (on Windows 11 it may be under "Show more options").
3. Expected result: no current threats.

If Defender reports a threat, **stop**. Do not run the installer, do not allow or restore the file, and do not add an exclusion. Report the detection name to university IT and to the thesis team so it can be investigated.

A clean scan lowers risk but cannot guarantee that a file is free of malware. The checksum and the official source matter as well.

## 7. Windows security prompts: SmartScreen and UAC

Windows can show three different things. They are not the same:

- **Microsoft Defender SmartScreen reputation warning** ("Windows protected your PC", or a browser warning that the file is not commonly downloaded). This means the file has no established publisher reputation. CALSS is not code-signed and is downloaded rarely, so this warning is likely. It is **not** a malware detection.
- **User Account Control (UAC)** ("Do you want to allow this app to make changes to your device?"). This appears because an all-users installation writes to Program Files. It names an unknown publisher because the file is not signed.
- **An actual Microsoft Defender detection** (a threat is quarantined or blocked). Treat it as in section 6: stop and investigate.

If a SmartScreen warning appears, whether to continue is the laboratory administrator's decision under university IT policy, made only after sections 4-6 pass. Instructors and students should not dismiss these warnings themselves. Do not disable SmartScreen, Defender or UAC, and do not change security settings to hide the warning. The CALSS installer adds no Defender exclusion and changes no Windows security setting; its only permission change is letting all users of the computer add entries to its own audit log file (section 12).

**About code signing.** A code-signing certificate from a publicly trusted certificate authority would let Windows show a verified publisher name and would prove that the file was not modified after signing. It does not give immediate SmartScreen reputation; that builds up over time and downloads. A self-signed certificate is not equivalent: Windows does not trust it, so it would not establish publisher identity for other computers. The thesis team has not bought a certificate. For this limited institutional deployment, the checks in sections 4-6 together with the laboratory administrator's approval take the place of a publisher signature.

## 8. Installation steps

1. Sign in to the instructor/admin computer with an account allowed to install software under the approved procedure.
2. Run {{INSTALLER}}.
3. **Select Setup Install Mode**: choose **Install for all users** (recommended). Approve the UAC prompt. ("Install for me only" needs no administrator rights but installs for the current Windows user only, and it cannot prepare the shared audit log described in section 12.)
4. **Welcome**: click Next.
5. **Before You Install CALSS**: read the information page (scope, limitation, requirements, data storage, shared accounts), then click Next.
6. **Select Destination Location**: keep the default, `C:\Program Files\CALSS` (for "only for me": `%LOCALAPPDATA%\Programs\CALSS`). Click Next.
7. **Select Additional Tasks**: "Create a desktop shortcut" is ticked by default. Click Next.
8. **Ready to Install**: click Install.
9. **Completing**: leave "Launch CALSS" ticked to start it, then Finish.

Setup installs 13 files (the program, two logo images, the Google Classroom client file and nine Help pictures) plus its uninstaller, a Start Menu shortcut, an optional desktop shortcut and an Add/Remove Programs entry named "CALSS {{VERSION}}". It writes nothing else, except in an all-users installation: it creates `C:\ProgramData\CALSS\audit.log` and lets every Windows user of the computer read the file and add to it (section 12).

## 9. First launch

Start CALSS from the Start Menu or the desktop shortcut. The window opens on "Load student submissions to begin" with Select folder, Import .c files and Sync Classroom. No account, key or internet connection is needed for this. Help > About shows "Version {{VERSION}} (Thesis Release)".

If the same Windows account used CALSS before, a "Continue previous session" card may also appear (see section 10, step 6).

## 10. Basic functionality check

Use the synthetic sample data, not real student submissions:

1. Extract {{SAMPLE}} to a normal folder (for example Documents). Do not open it from inside the ZIP window.
2. In CALSS, untick **Include AI summary** (bottom left), so that nothing is sent to an online service.
3. Click **Select folder** and choose `CALSS_SampleData_SYNTHETIC\Programming1`. Expected: 553 files.
4. Click **Run analysis**. Expected Overview headline: **705 of 2,186 pairs flagged for review**, 35 students. The "Flagged pairs by class" section starts collapsed.
5. Open the **Classes** and **Flagged pairs** tabs. Both should list results; expanding a flagged pair shows its comparison and "View source".
6. Close CALSS, start it again, and choose **Continue** on the "Continue previous session" card. The analysis re-runs (AI off) and returns to the page you left.

When finished, delete the extracted sample folder if it is no longer needed.

## 11. Google Classroom and Gemini (optional)

Both are optional. The core analysis works without them, fully offline.

**Google Classroom sync** (File > Sync with Google Classroom...) opens the default browser so the instructor can sign in with their own Google account. CALSS then downloads .c submissions from the courses they choose. Considerations:

- Needs internet access, and the local sign-in callback uses `http://localhost:8080` on the computer. If another program uses port 8080, sign-in times out after about 3 minutes.
- The CALSS Google sign-in is in Google's "Testing" mode: only Google accounts the thesis team has added as test users can sign in, and Google ends the sign-in after about 7 days. Ask the thesis team to add an instructor's account if sync is needed.
- The sign-in is saved in Windows Credential Manager for that Windows user. CALSS has no sign-out button. To remove it: `cmdkey /delete:CALSS/GoogleClassroomToken`, and revoke CALSS's access in the Google Account (Security > third-party access).
- Downloads go to `%LOCALAPPDATA%\CALSS\Classroom` of that Windows user.

**Gemini AI explanations** need internet access and the instructor's own Google Gemini API key (from Google AI Studio). No key is included. Enter it in File > AI Settings..., which tests it before saving. When "Include AI summary" is ticked, source code from up to 50 of the highest-scoring flagged pairs per run is sent to Google's Gemini service. Before enabling it, check that sending student work to an external AI service is allowed by university policy, and review Google's terms for the key being used. Without a key, "Include AI summary" is unavailable and everything else works. "Continue previous session" never uses Gemini.

## 12. Data storage and privacy

| Data | Location | Shared by |
|---|---|---|
| Program | `C:\Program Files\CALSS` (or `%LOCALAPPDATA%\Programs\CALSS`) | all users (read only) |
| Classroom downloads | `%LOCALAPPDATA%\CALSS\Classroom` | one Windows user |
| Settings, Gemini model choice, last session (`workspace.json`: folder path, page, class and student names; no scores, code or keys) | `%LOCALAPPDATA%\CALSS` | one Windows user |
| Gemini API key, Google sign-in | Windows Credential Manager, `CALSS/GeminiApiKey`, `CALSS/GoogleClassroomToken` | one Windows user |
| Similarity reports (HTML) | `Documents\CALSS Reports` | one Windows user |
| Imported .c files | `Documents\CALSS Imports` | one Windows user |
| CSV exports | wherever the user saves them | - |
| Index of classes/blocks | `calss_sync_manifest.jsonl` inside a folder CALSS analyzes | whoever can open that folder |
| Audit log | `C:\ProgramData\CALSS\audit.log` | **all users of the computer** |

The audit log records, with date and time, folder selections and their paths, analysis runs and their counts, AI setting changes, manual file assignments and generated reports (including student names). It does not record the Windows user name. Every Windows user of the computer can read it and add entries; none can edit or delete it. CALSS writes nothing into its program folder.

Submissions, reports, exports and the audit log can contain student names and academic work. Follow university data-handling policy. Do not share submissions, reports, exports, API keys or sign-ins. Delete working copies that are no longer needed. Only analyze submissions the instructor is authorized to handle.

## 13. Shared Windows accounts

Everything marked "one Windows user" above belongs to a Windows account, not to a person. If several instructors share one Windows account on the lab computer, each of them can open the others' Classroom downloads, reports and imports, use the saved Gemini key and Google sign-in, and see the "Continue previous session" card for the last dataset analyzed. Windows Credential Manager does not separate people who share one account.

Where feasible, give each instructor a separate, authorized Windows account. If an account must be shared: do not save personal API keys or Google sign-ins in it, remove saved credentials after use (`cmdkey /delete:CALSS/GeminiApiKey`, `cmdkey /delete:CALSS/GoogleClassroomToken`), and delete downloaded or imported submissions and reports after each session.

## 14. Upgrade behavior

Running a newer CALSS installer on a computer that has an older CALSS installed upgrades it in place: one CALSS entry in Add/Remove Programs, same folder and shortcuts, and settings, saved keys, the last session and the audit log are kept. Install the upgrade in the same mode (all users or only for me) as the existing installation. An upgrade from 1.3.0 to {{VERSION}} ("only for me") was tested by the thesis team on the development computer.

## 15. Uninstallation and data retention

Uninstall from Settings > Apps > Installed apps > CALSS {{VERSION}} > Uninstall (or Control Panel > Programs and Features). Uninstall removes:

- the program files, the shortcuts and the Add/Remove Programs entry;
- the Google Classroom downloads in `%LOCALAPPDATA%\CALSS\Classroom` **of the Windows account that runs the uninstaller only**. Other Windows accounts' Classroom downloads stay on the computer. In an all-users installation, the uninstaller may run as the administrator account that approved it, so the instructors' own Classroom folders may be left in place.

Kept on purpose (a message lists them after uninstalling): `Documents\CALSS Reports`, `Documents\CALSS Imports`, `C:\ProgramData\CALSS\audit.log`, the rest of `%LOCALAPPDATA%\CALSS` (settings and the last session), and the Credential Manager entries.

To remove all CALSS data when a computer is decommissioned or the deployment ends: for each Windows account that used CALSS, delete `%LOCALAPPDATA%\CALSS`, `Documents\CALSS Reports` and `Documents\CALSS Imports`, and run the two `cmdkey /delete` commands from section 13. Then delete `C:\ProgramData\CALSS` (administrator) after keeping any audit record that university policy requires.

## 16. Troubleshooting

| Problem | What to do |
|---|---|
| Setup says this Windows version is not supported | CALSS needs 64-bit Windows 10 or 11. |
| SmartScreen "Windows protected your PC" | Expected for an unsigned file; see section 7. Continue only by administrator decision under IT policy after sections 4-6. |
| Defender reports a threat | Stop; see section 6. Do not bypass it. |
| Checksum does not match | Do not run. Download again from the official link; contact the thesis team. |
| CALSS logos or Help pictures missing | Reinstall; the program must stay together with its installed files. |
| "Include AI summary" cannot be ticked | No Gemini key is saved for this Windows user (File > AI Settings...). This is normal without a key. |
| Classroom sign-in fails or "access blocked" | The Google account is not an approved test user, port 8080 is in use, or the network blocks Google. Core analysis is unaffected. |
| Networks with a proxy set only in Windows Settings | AI and Classroom may fail to connect; core analysis is unaffected. |
| First load of a very large folder is slow | Expected on the first load (tens of seconds for about 3,000 files); later loads are faster. |
| Another user's audit entries missing on a shared PC | Use the all-users installation, which prepares the shared audit log. |

## 17. Administrator acceptance checklist

| # | Check | Result |
|---|---|---|
| 1 | Package obtained from the official thesis-team Google Drive link | |
| 2 | File name {{INSTALLER}}, size {{SIZE}} bytes | |
| 3 | SHA-256 matches `{{SHA256}}` | |
| 4 | Microsoft Defender up to date; scan of the installer: no threats | |
| 5 | Any SmartScreen warning handled per university IT policy (note what appeared) | |
| 6 | Installed for all users; UAC approved by an authorized administrator | |
| 7 | "Before You Install CALSS" page shown and read | |
| 8 | Start Menu (and desktop) shortcuts start CALSS; About shows {{VERSION}} | |
| 9 | Sample Programming1 with AI off: 553 files, 705 of 2,186 flagged | |
| 10 | Overview, Classes and Flagged pairs tabs open | |
| 11 | Continue previous session returns to the last page | |
| 12 | Instructors informed of the limitation (section 1), data locations (section 12) and shared-account guidance (section 13) | |
| 13 | Sample data and test reports deleted after the check | |
| 14 | Installed only on the approved instructor/admin computer | |

Computer / laboratory: ________________________________

Checked by: ________________________________

Date: ________________
