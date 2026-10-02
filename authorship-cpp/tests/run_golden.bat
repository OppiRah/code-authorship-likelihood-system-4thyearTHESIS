@echo off
REM ─────────────────────────────────────────────────────────────
REM  CALSS — golden-output regression check
REM
REM    tests\run_golden.bat            build harness, run, compare
REM    tests\run_golden.bat --update   build harness, run, overwrite
REM                                    tests\golden\expected\ (only after
REM                                    a change that is SUPPOSED to alter
REM                                    results, and say so in the commit)
REM
REM  Runs the real analysis pipeline (AI off) on the synthetic datasets
REM  committed next to this project and compares every score byte for
REM  byte against tests\golden\expected\. Each dataset is copied to
REM  %TEMP% first, because loading a folder appends to its
REM  calss_sync_manifest.jsonl.
REM ─────────────────────────────────────────────────────────────
setlocal
cd /d "%~dp0.."

set WORK=%TEMP%\calss_golden
set EXPECTED=tests\golden\expected
set UPDATE=0
if /i "%~1"=="--update" set UPDATE=1

if exist "%WORK%" rmdir /s /q "%WORK%"
mkdir "%WORK%" || exit /b 1

echo Building harness...
REM Same compiler flags as compile.bat. Only the pipeline and what it
REM depends on are linked: no GUI, no main.cpp.
g++ -std=c++17 -O2 -static-libgcc -static-libstdc++ -I include ^
    tests/golden_harness.cpp src/analysis_pipeline.cpp ^
    src/tokenizer.cpp src/features.cpp src/knn_model.cpp src/similarity.cpp ^
    src/gemini.cpp src/secret_store.cpp src/app_paths.cpp src/audit_log.cpp ^
    -lwinhttp -lshell32 -ladvapi32 ^
    -o "%WORK%\golden_harness.exe" || goto :buildfail

set FAILED=0
call :dataset bscs7_programming1 "..\stress_test_bscs7\Programming1"
call :dataset bscs7_programming2 "..\stress_test_bscs7\Programming2"
call :dataset stress_test_all_blocks "..\stress_test"

echo.
if "%UPDATE%"=="1" (
    echo Expected outputs updated in %EXPECTED%.
    exit /b 0
)
if "%FAILED%"=="0" (
    echo GOLDEN CHECK PASSED - all outputs byte-identical.
    exit /b 0
)
echo GOLDEN CHECK FAILED - see differences above.
exit /b 1

:dataset
REM %1 = name, %2 = source folder
robocopy "%~2" "%WORK%\data\%1" /E /NFL /NDL /NJH /NJS /NP >nul
if errorlevel 8 (echo Copy failed for %1 & set FAILED=1 & exit /b 0)
mkdir "%WORK%\out\%1"
"%WORK%\golden_harness.exe" "%WORK%\data\%1" "%WORK%\out\%1"
if errorlevel 1 (echo Harness reported an invalid run for %1 & set FAILED=1)
if "%UPDATE%"=="1" (
    if exist "%EXPECTED%\%1" rmdir /s /q "%EXPECTED%\%1"
    mkdir "%EXPECTED%\%1"
    copy /y "%WORK%\out\%1\hashes.txt" "%EXPECTED%\%1\" >nul
    REM Reference copies for diagnosing a failure; files over 10 MB are
    REM covered by hashes.txt only.
    for %%F in ("%WORK%\out\%1\*.csv") do if %%~zF LSS 10000000 copy /y "%%F" "%EXPECTED%\%1\" >nul
    exit /b 0
)
fc "%EXPECTED%\%1\hashes.txt" "%WORK%\out\%1\hashes.txt" >nul 2>&1 && (echo   %1: identical) || (
    echo   DIFF: %1 - mismatching files:
    fc "%EXPECTED%\%1\hashes.txt" "%WORK%\out\%1\hashes.txt"
    echo   Fresh output: %WORK%\out\%1
    set FAILED=1
)
exit /b 0

:buildfail
echo Harness build failed.
exit /b 1
