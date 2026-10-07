@echo off
REM -------------------------------------------------------------
REM  CALSS - build the Windows installer. Run compile.bat first.
REM
REM    make_installer.bat
REM
REM  Output (in release\):
REM    CALSS_Setup_<version>.exe     the installer (unsigned)
REM    CALSS_Setup_<version>_SHA256.txt
REM                                  SHA-256 of the installer and of
REM                                  the authorship.exe inside it
REM
REM  Needs Inno Setup 6.4+ (free, jrsoftware.org). Looked for in the
REM  per-user and per-machine default folders, or set ISCC to the full
REM  path of ISCC.exe. Script: installer\calss.iss (version is read from
REM  authorship.exe, i.e. from include\version.h).
REM
REM  Like make_release.bat, it refuses to package without the app's
REM  OAuth client file and never packages personal secrets: the
REM  installer's [Files] list names each file explicitly.
REM -------------------------------------------------------------
setlocal
cd /d "%~dp0"

if not exist authorship.exe (
    echo ERROR: authorship.exe not found. Run compile.bat first.
    exit /b 1
)
if not exist calss_gc_embedded_credentials.json (
    echo ERROR: calss_gc_embedded_credentials.json missing ^(Classroom client file^).
    exit /b 1
)

if not defined ISCC if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not defined ISCC (
    echo ERROR: Inno Setup 6 not found. Install it from https://jrsoftware.org/isdl.php
    echo        or set ISCC=full\path\to\ISCC.exe
    exit /b 1
)

set VER=
for /f "usebackq delims=" %%V in (`powershell -NoProfile -Command "(Get-Item 'authorship.exe').VersionInfo.FileVersion"`) do set VER=%%V
if "%VER%"=="" (
    echo ERROR: authorship.exe has no version resource. Rebuild with compile.bat.
    exit /b 1
)

if not exist release mkdir release
if exist "release\CALSS_Setup_%VER%.exe" del /q "release\CALSS_Setup_%VER%.exe"
"%ISCC%" /Q "installer\calss.iss" || (echo ERROR: installer build failed & exit /b 1)

set COMMIT=nogit
for /f "usebackq delims=" %%C in (`git rev-parse --short HEAD 2^>nul`) do set COMMIT=%%C
set DIRTY=
for /f "usebackq delims=" %%D in (`git status --porcelain -- src include res compile.bat installer 2^>nul`) do set DIRTY=-dirty

powershell -NoProfile -Command ^
  "$i=Get-FileHash -Algorithm SHA256 'release\CALSS_Setup_%VER%.exe'; $e=Get-FileHash -Algorithm SHA256 'authorship.exe';" ^
  "$l=@('CALSS %VER% - SHA-256 checksums','',('Built:  '+(Get-Date -Format 'yyyy-MM-dd HH:mm')),'Commit: %COMMIT%%DIRTY%','',($i.Hash+'  CALSS_Setup_%VER%.exe'),($e.Hash+'  authorship.exe (installed into the CALSS folder)'),'','Check a download in PowerShell:','  Get-FileHash .\CALSS_Setup_%VER%.exe -Algorithm SHA256','The installer is not code-signed; the hash is how you know it is the official build.');" ^
  "Set-Content -Encoding ascii 'release\CALSS_Setup_%VER%_SHA256.txt' $l; Get-Content 'release\CALSS_Setup_%VER%_SHA256.txt'"

echo.
echo Installer: %CD%\release\CALSS_Setup_%VER%.exe
endlocal
