@echo off
REM ─────────────────────────────────────────────────────────────
REM  Code Authorship Likelihood Scoring System
REM  Run Script — Windows
REM ─────────────────────────────────────────────────────────────

if not exist authorship.exe (
    echo.
    echo ERROR: authorship.exe not found.
    echo Please run compile.bat first to build the system.
    echo.
    pause
    exit /b 1
)

if not exist data (
    echo.
    echo ERROR: data folder not found.
    echo Please ensure the 'data' folder exists with .c files inside.
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================================
echo   Running Code Authorship Likelihood Scoring System...
echo ============================================================
echo.

authorship.exe data

echo.
pause
