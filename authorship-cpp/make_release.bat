@echo off
REM ─────────────────────────────────────────────────────────────
REM  CALSS — build a clean release folder to hand to someone else
REM  (e.g. a professor). Run compile.bat first.
REM
REM  The folder contains ONLY what the app needs to run:
REM    authorship.exe
REM    ulccslogo.png                 (sidebar seal)
REM    ulcclogoloadingscreen.png     (loading-screen seal)
REM    assets\help\calss_help_*.png  (Help > Documentation guide)
REM    calss_gc_embedded_credentials.json
REM        - the APP's Google OAuth client ID, needed for Classroom sync.
REM          Not a personal secret (Google treats desktop-app client
REM          credentials as public); it is NOT your sign-in token.
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
REM ─────────────────────────────────────────────────────────────
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

REM Safety net: refuse to leave a personal secret in the release.
if exist "%OUT%\api_key.txt" del /q "%OUT%\api_key.txt"
if exist "%OUT%\calss_gc_token.json" del /q "%OUT%\calss_gc_token.json"

echo.
echo Release folder ready: %CD%\%OUT%
dir /b /s "%OUT%"
endlocal
