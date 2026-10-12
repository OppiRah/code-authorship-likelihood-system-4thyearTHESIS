CALSS {{VERSION}} - Code Authorship Likelihood Scoring System
Laboratory deployment package
College of Computer Studies, University of Luzon (undergraduate thesis project)
=====================================================================

WHAT CALSS IS
-------------
An academic decision-support application for instructors. It analyzes
student C programming submissions (.c files) for code similarity and
coding-style (authorship-related) anomalies and lists the submissions
that may warrant further review.

CALSS does NOT establish plagiarism, cheating or misconduct. Flagged
pairs and scores must be interpreted by an instructor together with
other relevant academic evidence.

INTENDED USERS AND SCOPE
------------------------
Authorized instructors and university personnel. For this deployment,
CALSS is installed only on the approved instructor/admin computer of
each approved programming laboratory - not on student computers,
personal devices or unapproved university computers.

REQUIREMENTS
------------
- 64-bit Windows 10 or Windows 11.
- About 10 MB of disk space, plus space for submissions and reports.
- Administrator approval once for the recommended all-users install.
- Nothing else to install. Minimum CPU/RAM have not been formally
  measured.
- Internet only for the optional features below.

IN THIS PACKAGE
---------------
  {{INSTALLER}}    the installer (NOT code-signed)
  SHA256SUMS.txt               checksums of the files in this package
  INSTALLATION_GUIDE.pdf       step-by-step guide for administrators
  RELEASE_NOTES.txt            what is in {{VERSION}}, known limitations
  {{SAMPLE}}  synthetic test data (not real students)

INSTALLING - OVERVIEW
---------------------
1. Get the package only from the thesis team's official Google Drive
   link.
2. Check the installer's SHA-256 against SHA256SUMS.txt:
     Get-FileHash .\{{INSTALLER}} -Algorithm SHA256
   Expected: {{SHA256}}
3. Scan it with an up-to-date Microsoft Defender. If a threat is
   reported, STOP and report it - do not bypass it.
4. Run it, choose "Install for all users", follow the wizard, and read
   the "Before You Install CALSS" page.
5. Do the acceptance check in INSTALLATION_GUIDE.pdf, section 17.

Because CALSS is not code-signed, Windows may show a SmartScreen
"Windows protected your PC" warning. That is a reputation warning, not
a malware detection. Whether to continue is the laboratory
administrator's decision under university IT policy, after the checks
above - see INSTALLATION_GUIDE.pdf, section 7. Do not disable
SmartScreen, Defender or UAC.

OPTIONAL INTEGRATIONS
---------------------
- Google Classroom sync: internet + a Google account approved by the
  thesis team (the sign-in is in Google's "Testing" mode).
- Gemini AI explanations: internet + the instructor's own Gemini API
  key. Code from up to 50 flagged pairs per run is sent to Google.
The core analysis works offline without either.

DATA
----
Classroom downloads, settings, the last session, saved keys/sign-ins,
reports and imports are kept per Windows user; the audit log
(C:\ProgramData\CALSS\audit.log) is shared by all users of the
computer. Anyone using the same Windows account can see that account's
CALSS data and saved sign-ins. Details, and what uninstall removes:
INSTALLATION_GUIDE.pdf, sections 12-15.

SUPPORT
-------
Contact the CALSS thesis team through the channel through which you
received this package.
