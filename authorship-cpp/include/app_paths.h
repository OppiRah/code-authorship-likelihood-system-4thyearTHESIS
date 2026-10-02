#ifndef APP_PATHS_H
#define APP_PATHS_H
// ─────────────────────────────────────────────────────────────
// app_paths.h
// File locations and path encoding for CALSS.
//
// LOCATIONS — independent of the process working directory (which a
// file dialog or a shortcut's "Start in" can change):
//   read-only assets shipped with the app  → the exe's folder
//   per-user app state                     → %LOCALAPPDATA%\CALSS
//   documents the user opens / keeps       → Documents\CALSS Reports
//
// ENCODING RULE — every std::string that holds a path is UTF-8.
// Windows' narrow (*A) APIs and a plain std::ifstream(std::string)
// interpret bytes in the ANSI code page instead, so a folder or file
// named e.g. "Peña" would not be found. Open files with
// pathFromUtf8() (std::ifstream f(pathFromUtf8(p))) and call the
// wide (*W) Win32 APIs with widenUtf8(p).
// ─────────────────────────────────────────────────────────────

#include <filesystem>
#include <string>

// UTF-8 <-> UTF-16 conversion.
std::wstring widenUtf8(const std::string& utf8);
std::string  utf8FromWide(const std::wstring& wide);

// A UTF-8 path as a std::filesystem::path (for fstream constructors).
std::filesystem::path pathFromUtf8(const std::string& utf8);

// Folder containing authorship.exe, no trailing backslash.
std::wstring appExeDirW();
std::string  appExeDirU8();

// %LOCALAPPDATA%\CALSS (UTF-8), created if missing. Falls back to the
// exe's folder if the known folder can't be resolved.
std::string  appDataDirU8();

// Documents\CALSS Reports (UTF-8), created if missing. Falls back to
// the exe's folder if Documents can't be resolved.
std::string  appReportsDirU8();

#endif // APP_PATHS_H
