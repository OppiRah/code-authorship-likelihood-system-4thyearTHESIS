@echo off
REM ─────────────────────────────────────────────────────────────
REM  Code Authorship Likelihood Scoring System
REM  Run Script — Windows
REM
REM  Launches the GUI. (The old "authorship.exe data" CLI mode was
REM  removed; load submissions from inside the app instead.)
REM ─────────────────────────────────────────────────────────────

cd /d "%~dp0"

if not exist authorship.exe (
    echo.
    echo ERROR: authorship.exe not found.
    echo Please run compile.bat first to build the system.
    echo.
    pause
    exit /b 1
)

start "" authorship.exe
