#pragma once
#include <string>

// ─────────────────────────────────────────────────────────────
// Audit Log for CALSS
// Records every significant action with timestamp for
// academic integrity defensibility. Also provides SHA-256
// hashing for report tamper-evidence.
// ─────────────────────────────────────────────────────────────

// Log is stored at %PROGRAMDATA%\CALSS\audit.log (not next to the exe).
// This makes it harder for a student to tamper with or delete —
// modifying ProgramData requires elevated permissions.
#define AUDIT_LOG_FILE "calss_audit.log"  // fallback name only; see audit_log.cpp

// Append a timestamped entry to the audit log.
// category examples: "ANALYSIS RUN", "REPORT GENERATED",
// "REPORT OPENED", "GOOGLE CLASSROOM SYNC", "FOLDER SELECTED"
void audit_log(const std::string& category, const std::string& details);

// Compute SHA-256 hash of a file's contents.
// Returns lowercase hex string, or empty string on failure.
std::string audit_sha256File(const std::string& filePath);

// Compute SHA-256 hash of a string (e.g. HTML content before writing).
std::string audit_sha256String(const std::string& content);

// Returns the full resolved path to the audit log file
// (%PROGRAMDATA%\CALSS\audit.log). Use this to open/view the log
// from the UI — do not assume it's next to the executable.
std::string audit_getLogPath();