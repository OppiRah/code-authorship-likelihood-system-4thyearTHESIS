#ifndef APP_PATHS_H
#define APP_PATHS_H
// ─────────────────────────────────────────────────────────────
// app_paths.h
// Where CALSS reads and writes its own files, independent of the
// process working directory (which a file dialog or a shortcut's
// "Start in" can change).
//
//   read-only assets shipped with the app  → the exe's folder
//   per-user app state                     → %LOCALAPPDATA%\CALSS
//   documents the user opens / keeps       → Documents\CALSS Reports
//
// Narrow (std::string) variants are in the ANSI code page, matching
// the narrow file APIs (std::ifstream/ofstream, *A Win32 calls) the
// callers use.
// ─────────────────────────────────────────────────────────────

#include <string>

// Folder containing authorship.exe, no trailing backslash.
std::wstring appExeDirW();
std::string  appExeDirA();

// %LOCALAPPDATA%\CALSS, created if missing. Falls back to the exe's
// folder if the known folder can't be resolved.
std::string  appDataDirA();

// Documents\CALSS Reports, created if missing. Falls back to the exe's
// folder if Documents can't be resolved.
std::string  appReportsDirA();

#endif // APP_PATHS_H
