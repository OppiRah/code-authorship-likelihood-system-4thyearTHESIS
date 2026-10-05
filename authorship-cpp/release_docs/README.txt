CALSS - Code Authorship Likelihood Scoring System
Thesis test build  (see BUILD_INFO.txt for the exact build)
University of Luzon - College of Computer Studies
=====================================================================

WHAT IT IS
----------
CALSS helps a professor review student C-language (.c) submissions. It
compares every pair of submissions (token similarity) and each student's
coding style (14 stylometric features, KNN with K=3), then lists the
pairs worth a closer look. It is a decision aid: a flagged pair is a
reason to look, not proof of plagiarism.

THIS IS A TEST BUILD for thesis-team testing only. It is not the final
release and there is no installer yet.


REQUIREMENTS
------------
- 64-bit Windows 10 or Windows 11.
- Nothing else to install (no MinGW, Visual Studio or runtime packages).
- Internet only for the optional AI summaries and Google Classroom sync.
- No administrator rights needed.


INSTALL / LAUNCH
----------------
1. Extract the whole ZIP to a normal folder, for example
   Documents\CALSS  or  Desktop\CALSS.
   Do NOT run it from inside the ZIP preview window - the logos and
   help pictures next to the program would not be found.
2. Double-click  authorship.exe.
3. Windows may warn "Windows protected your PC" (SmartScreen), because
   the program is not code-signed. Click "More info" -> "Run anyway".
   This is expected for this test build.

Keep the files in this folder together. authorship.exe needs:
  ulccslogo.png, ulcclogoloadingscreen.png, assets\help\*.png,
  calss_gc_embedded_credentials.json


ANALYZING SUBMISSIONS
---------------------
1. Click "Select folder" (or File > Open Data Folder..., Ctrl+O) and pick
   a folder of .c files. Subfolders (e.g. one per block/section) are
   loaded too. File > Import .c Files... (Ctrl+I) copies individual
   files into the loaded folder.
2. Untick "Include AI summary" if you have no Gemini key (see below).
3. Click "Run analysis". Then use the three tabs:
     Overview       - headline numbers, risk bar, AI summary (if on)
     Classes        - every student; open one for their Style DNA,
                      submissions and flagged appearances
     Flagged pairs  - the flagged pairs; filter, search, expand a pair,
                      "View source" to compare the two files side by side
4. Export CSV (Flagged pairs toolbar) saves all analyzed pairs wherever
   you choose. "Generate Similarity Report" on a student page writes an
   HTML report and opens it in your browser.

Student names come from the file names (e.g. "finalexamCarlos.c" ->
"Carlos"; Classroom-synced files use "<type>__<student>.c").

Note: CALSS writes a small index file, calss_sync_manifest.jsonl, into
the folder you load. Load a COPY if the folder must stay untouched.


WHERE CALSS STORES THINGS
-------------------------
Nothing is written into the CALSS program folder.
  Similarity reports      Documents\CALSS Reports\
  Classroom downloads     Documents\CALSS_Classroom_Sync\  (default)
  Classroom settings      %LOCALAPPDATA%\CALSS\
  Audit log               C:\ProgramData\CALSS\audit.log
                          (View > View Audit Log)
  Gemini key, Google      Windows Credential Manager, per Windows user
  sign-in                 (entries "CALSS/GeminiApiKey" and
                          "CALSS/GoogleClassroomToken")
To remove CALSS completely: delete this folder, the folders above, and
those two Credential Manager entries (Control Panel > Credential
Manager > Windows Credentials).


AI SUMMARIES (GEMINI) - OPTIONAL
--------------------------------
CALSS works fully without AI. To turn on AI summaries, get your OWN
Google Gemini API key (Google AI Studio), then File > AI Settings...,
paste it, "Test key", Save. The key is stored only for your Windows user
on this PC. No key is included in this package. AI notes are generated
for at most the 50 highest-scoring flagged pairs per run. Code from those
pairs is sent to Google's Gemini service.


GOOGLE CLASSROOM SYNC - TEST-USER ACCOUNTS ONLY
-----------------------------------------------
File > Sync with Google Classroom... opens your browser to sign in with
your own Google account; CALSS then downloads .c submissions from the
courses you pick. No account or sign-in is included in this package.

IMPORTANT for this test build: the CALSS Google sign-in is in Google's
"Testing" mode. ONLY Google accounts that the team has added as approved
test users can sign in. Any other account is refused by Google (an
"access blocked" / "app has not completed verification" style page).
That is a Google configuration limit, not a CALSS bug. Ask the team to
add your account if you need to test sync. In Testing mode Google also
expires the sign-in after about 7 days; just sign in again.

Sign-in uses http://localhost:8080 on your PC. If another program uses
port 8080, sign-in times out after ~3 minutes with a generic message.


KNOWN LIMITATIONS OF THIS BUILD
-------------------------------
- The FIRST load of a folder with many subfolders can be slow (tens of
  seconds for ~3,000 files); later loads of the same folder are fast.
- Networks that need a proxy set only in Windows Settings may block AI
  summaries and Classroom sync.
- The program has the generic Windows icon (no CALSS icon yet).
- Help > About still says "Version 0.4.0 (Proposal Prototype)".
- Overview "High similarity" and the Flagged pairs "High" filter count
  different things, so their numbers can differ.
- On a PC shared by several Windows users, only the first user's audit
  entries may be recorded.


REPORTING A PROBLEM
-------------------
Send the CALSS developer (team group chat) for each problem:
  - the build from BUILD_INFO.txt
  - Windows version (Settings > System > About)
  - what you did, what you expected, what happened
  - a screenshot, and the exact text of any error message
  - whether it happens again if you repeat the steps
See TESTER_CHECKLIST.txt for the full test list.
