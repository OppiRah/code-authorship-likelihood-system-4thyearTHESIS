#pragma once
#include <string>
#include <vector>
#include <map>

// Forward declare HWND to avoid pulling in windows.h here
// (caller must include windows.h before this header)
#ifndef HWND
typedef struct HWND__* HWND;
#endif

// ─────────────────────────────────────────────────────────────
// Google Classroom Integration for CALSS
// OAuth 2.0 Installed App flow via WinHTTP + Winsock listener
// ─────────────────────────────────────────────────────────────

struct GCourse {
    std::string id;
    std::string name;
    std::string section;
    std::string enrollmentCode;
};

struct GAssignment {
    std::string id;
    std::string title;
    std::string description;
    std::string dueDate;
};

struct GCToken {
    std::string accessToken;
    std::string refreshToken;
    long long   expiresAt;  // Unix timestamp
};

// ── Credentials (loaded from JSON file) ─────────────────────
struct GCCredentials {
    std::string clientId;
    std::string clientSecret;
    std::string authUri;
    std::string tokenUri;
};

// ── Result type ───────────────────────────────────────────────
struct GCResult {
    bool        success;
    std::string error;
    int         filesDownloaded;
};

// ── Settings file path ────────────────────────────────────────
// Stored alongside the exe: calss_gc_settings.json
#define GC_SETTINGS_FILE "calss_gc_settings.json"
#define GC_TOKEN_FILE    "calss_gc_token.json"
#define GC_AUTH_PORT     8080

// Local, gitignored file holding the CALSS project's own OAuth
// credentials (see the comment above gc_loadSettings() in
// google_classroom.cpp for why these live outside of source
// control). A placeholder template ships as
// calss_gc_embedded_credentials.example.json.
#define GC_EMBEDDED_CREDS_FILE "calss_gc_embedded_credentials.json"

// ── Public API ────────────────────────────────────────────────

// Load credentials from a Google JSON file (the one you downloaded)
bool gc_loadCredentials(const std::string& jsonPath, GCCredentials& out);

// Save/load credentials to settings file
bool gc_saveSettings(const GCCredentials& creds);
bool gc_loadSettings(GCCredentials& creds);

// Check if we have a valid saved token
bool gc_hasValidToken();

// Full OAuth flow: opens browser, listens on localhost:8080 for callback
bool gc_authenticate(HWND parent, const GCCredentials& creds, bool unused = false);

// Refresh access token using refresh token
bool gc_refreshIfNeeded(const GCCredentials& creds);

// API calls (all return empty vector on failure)
std::vector<GCourse>     gc_getCourses(const GCCredentials& creds);
std::vector<GAssignment> gc_getAssignments(const GCCredentials& creds,
                                            const std::string& courseId);

// Download all .c file submissions for a given assignment.
// assignmentTitle is used to embed a category signal (exam/quiz/activity)
// into the downloaded filenames based on the instructor's own naming —
// see gc_downloadSubmissions() implementation for the priority hierarchy.
// blockName is the course's section (e.g. "BSIT 4") for display grouping
// only — it does not affect file naming or analysis logic.
// Returns result with count of files downloaded.
GCResult gc_downloadSubmissions(HWND parent,
                                  const GCCredentials& creds,
                                  const std::string& courseId,
                                  const std::string& assignmentId,
                                  const std::string& assignmentTitle,
                                  const std::string& blockName,
                                  const std::string& outputFolder);

// Reads calss_sync_manifest.jsonl from the given folder (if present) and
// returns a map of {downloaded filename -> {assignment title, block name}},
// for display purposes only. "Block" is the Classroom course section
// (e.g. "BSIT 4") — used to group students by class section in the UI.
// Returns an empty map if no manifest exists (e.g. the folder only
// contains manually imported files).
struct SyncManifestEntry {
    std::string title; // e.g. "Midterm Exam"
    std::string block; // e.g. "BSIT 4" — may be empty if course had no section set
};
std::map<std::string,SyncManifestEntry> gc_loadSyncManifest(const std::string& folder);

// Revoke / sign out
void gc_signOut();