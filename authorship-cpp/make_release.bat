@echo off
REM -------------------------------------------------------------
REM  CALSS - build a clean, portable release folder + ZIP to hand to
REM  someone else (thesis mates, a professor). Run compile.bat first.
REM
REM    make_release.bat            release\CALSS\ + release\<name>.zip
REM    make_release.bat --nozip    release\CALSS\ only
REM
REM  The folder contains ONLY what the app needs to run, plus the
REM  user-facing docs:
REM    authorship.exe
REM    ulccslogo.png                 (sidebar seal)
REM    ulcclogoloadingscreen.png     (loading-screen seal)
REM    assets\help\calss_help_*.png  (Help > Documentation guide)
REM    calss_gc_embedded_credentials.json
REM        - the APP's Google OAuth client ID, needed for Classroom sync.
REM          Not a personal secret (Google treats desktop-app client
REM          credentials as public); it is NOT your sign-in token.
REM    README.txt, TESTER_CHECKLIST.txt  (from release_docs\)
REM    BUILD_INFO.txt                (generated: date, commit, exe SHA-256)
REM
REM  Deliberately NOT copied:
REM    api_key.txt, calss_gc_token.json  - personal secrets; these now
REM        live in Windows Credential Manager (per Windows user), so
REM        nothing of yours travels with the app. Each person enters
REM        their own Gemini key under File > AI Settings, and signs in
REM        to Classroom with their own Google account.
REM    calss_gc_settings.json            - regenerated automatically
REM    report.html, similarity_report_*.html, calss_audit.log,
REM    profile.log, *.log, data\          - generated output / student data
REM    assets\help\_dryrun_test.png       - test image
REM
REM  Also writes release\CALSS_SampleData_SYNTHETIC.zip from the
REM  committed synthetic datasets (..\stress_test_bscs7\Programming1/2)
REM  so testers have data to analyze. It is a separate ZIP on purpose:
REM  the app package itself carries no submissions.
REM -------------------------------------------------------------
setlocal
cd /d "%~dp0"

if not exist authorship.exe (
    echo ERROR: authorship.exe not found. Run compile.bat first.
    exit /b 1
)

set OUT=release\CALSS
if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%\assets\help"

copy /y authorship.exe "%OUT%\" >nul
copy /y ulccslogo.png "%OUT%\" >nul
copy /y ulcclogoloadingscreen.png "%OUT%\" >nul
copy /y assets\help\calss_help_*.png "%OUT%\assets\help\" >nul
copy /y calss_gc_embedded_credentials.json "%OUT%\" >nul
copy /y release_docs\README.txt "%OUT%\" >nul
copy /y release_docs\TESTER_CHECKLIST.txt "%OUT%\" >nul

REM Safety net: refuse to leave a personal secret in the release.
if exist "%OUT%\api_key.txt" del /q "%OUT%\api_key.txt"
if exist "%OUT%\calss_gc_token.json" del /q "%OUT%\calss_gc_token.json"

REM BUILD_INFO.txt + release name (date and commit) via PowerShell,
REM which every Windows 10/11 PC has.
set STAMP=
for /f "usebackq delims=" %%S in (`powershell -NoProfile -Command "Get-Date -Format yyyy-MM-dd"`) do set STAMP=%%S
set COMMIT=nogit
for /f "usebackq delims=" %%C in (`git rev-parse --short HEAD 2^>nul`) do set COMMIT=%%C
set DIRTY=
for /f "usebackq delims=" %%D in (`git status --porcelain -- src include compile.bat 2^>nul`) do set DIRTY=-dirty
set RELNAME=CALSS_thesis-test_%STAMP%_%COMMIT%%DIRTY%

powershell -NoProfile -Command ^
  "$h=(Get-FileHash -Algorithm SHA256 '%OUT%\authorship.exe').Hash;" ^
  "$l=@('CALSS thesis test build','','Release:  %RELNAME%','Built:    '+(Get-Item '%OUT%\authorship.exe').LastWriteTime.ToString('yyyy-MM-dd HH:mm'),'Branch:   '+(git rev-parse --abbrev-ref HEAD 2>$null),'Commit:   '+(git rev-parse HEAD 2>$null)+'%DIRTY%','EXE SHA-256: '+$h,'','Portable package for cross-PC testing. Not an installer.');" ^
  "Set-Content -Encoding ascii '%OUT%\BUILD_INFO.txt' $l"

REM Contents check: exactly the expected files, nothing else.
powershell -NoProfile -Command ^
  "$want=@('authorship.exe','ulccslogo.png','ulcclogoloadingscreen.png','calss_gc_embedded_credentials.json','README.txt','TESTER_CHECKLIST.txt','BUILD_INFO.txt') + ((Get-ChildItem 'assets\help\calss_help_*.png').Name | %%{ 'assets\help\'+$_ });" ^
  "$have=Get-ChildItem -Recurse -File '%OUT%' | %%{ $_.FullName.Substring((Resolve-Path '%OUT%').Path.Length+1) };" ^
  "$d=Compare-Object $want $have; if ($d) { $d | Out-String | Write-Host; exit 1 } else { Write-Host ('Contents check OK: '+$have.Count+' files') }"
if errorlevel 1 (
    echo ERROR: release folder contents differ from the expected list.
    exit /b 1
)

echo.
echo Release folder ready: %CD%\%OUT%
dir /b /s "%OUT%"

if /i "%~1"=="--nozip" goto :done

if exist "release\%RELNAME%.zip" del /q "release\%RELNAME%.zip"
powershell -NoProfile -Command "Compress-Archive -Path '%OUT%' -DestinationPath 'release\%RELNAME%.zip'" || (echo ERROR: zip failed & exit /b 1)
echo App ZIP:    %CD%\release\%RELNAME%.zip

REM Synthetic sample data (separate ZIP). Copied to a staging folder
REM first so only the committed dataset goes in.
set SAMPLE=release\_sample_stage\CALSS_SampleData_SYNTHETIC
if exist "release\_sample_stage" rmdir /s /q "release\_sample_stage"
mkdir "%SAMPLE%"
robocopy "..\stress_test_bscs7\Programming1" "%SAMPLE%\Programming1" /E /NFL /NDL /NJH /NJS /NP >nul
if errorlevel 8 (echo ERROR: sample copy failed & exit /b 1)
robocopy "..\stress_test_bscs7\Programming2" "%SAMPLE%\Programming2" /E /NFL /NDL /NJH /NJS /NP >nul
if errorlevel 8 (echo ERROR: sample copy failed & exit /b 1)
(
echo CALSS sample data - SYNTHETIC
echo.
echo These .c files were generated for testing CALSS. They are not real
echo student submissions. Load Programming1 or Programming2 with
echo "Select folder" in CALSS. Expected with AI off:
echo   Programming1: 553 files, 2,186 pairs, 705 flagged
echo   Programming2: 480 files, 1,740 pairs, 662 flagged
echo CALSS adds/updates calss_sync_manifest.jsonl in a loaded folder.
) > "%SAMPLE%\README_SAMPLE.txt"
if exist "release\CALSS_SampleData_SYNTHETIC.zip" del /q "release\CALSS_SampleData_SYNTHETIC.zip"
powershell -NoProfile -Command "Compress-Archive -Path '%SAMPLE%' -DestinationPath 'release\CALSS_SampleData_SYNTHETIC.zip'" || (echo ERROR: sample zip failed & exit /b 1)
rmdir /s /q "release\_sample_stage"
echo Sample ZIP: %CD%\release\CALSS_SampleData_SYNTHETIC.zip

:done
endlocal
