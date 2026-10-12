@echo off
REM -------------------------------------------------------------
REM  CALSS - unit tests for the "Continue previous session" state file
REM  (src\workspace_state.cpp): parsing, validation, atomic save, and
REM  the restore decisions. Builds into %TEMP%, touches nothing else.
REM
REM    tests\run_workspace_tests.bat
REM -------------------------------------------------------------
setlocal
cd /d "%~dp0.."

set WORK=%TEMP%\calss_workspace_tests
if exist "%WORK%" rmdir /s /q "%WORK%"
mkdir "%WORK%" || exit /b 1

g++ -std=c++17 -O2 -Wall -static -static-libgcc -static-libstdc++ -I include ^
    tests/workspace_state_test.cpp src/workspace_state.cpp src/app_paths.cpp ^
    -lshell32 -lole32 ^
    -o "%WORK%\workspace_state_test.exe" || (echo Test build failed. & exit /b 1)

"%WORK%\workspace_state_test.exe" "%WORK%"
if errorlevel 1 (echo WORKSPACE TESTS FAILED & exit /b 1)
echo WORKSPACE TESTS PASSED
exit /b 0
