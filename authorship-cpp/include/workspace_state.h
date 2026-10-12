#ifndef WORKSPACE_STATE_H
#define WORKSPACE_STATE_H
// ─────────────────────────────────────────────────────────────
// workspace_state.h
// "Continue previous session" (1.3.0): the ONE most recent workspace,
// saved as %LOCALAPPDATA%\CALSS\workspace.json.
//
// Only navigation and a reference to the dataset folder are stored —
// never scores, results, source code, keys or tokens. Continuing a
// session re-runs the normal analysis on the folder (AI off) and then
// re-applies this navigation by NAME (student, block, pair file
// names), never by list index, so a changed dataset can't land on the
// wrong student or pair.
//
// No GUI or Win32 types here beyond the file I/O in the .cpp, so the
// parser and the decision helpers are unit-tested on their own
// (tests/workspace_state_test.cpp).
// ─────────────────────────────────────────────────────────────

#include <string>
#include <vector>

// Bump when the meaning of a field changes. A file with a HIGHER version
// (written by a newer CALSS) is not used — see parseWorkspace.
static const int WORKSPACE_FORMAT_VERSION = 1;
static const char* const WORKSPACE_FORMAT_TAG = "calss-workspace";

// Bounds for reading: anything larger is not a file CALSS wrote.
static const size_t WORKSPACE_MAX_FILE_BYTES = 64 * 1024;
static const size_t WORKSPACE_MAX_PATH_CHARS = 4096;  // UTF-8 bytes
static const size_t WORKSPACE_MAX_NAME_CHARS = 512;

// page values
static const char* const WS_PAGE_OVERVIEW = "overview";
static const char* const WS_PAGE_CLASSES  = "classes";
static const char* const WS_PAGE_FLAGGED  = "flagged";

struct WorkspaceState {
    int         version = WORKSPACE_FORMAT_VERSION;
    std::string appVersion;   // CALSS version that wrote it (informational)
    std::string savedAt;      // local time, display only ("2026-10-12 14:05")
    std::string dataset;      // UTF-8 full path of the analyzed folder
    int         fileCount = 0; // .c files when it was analyzed (display/compare only)
    std::string page;         // WS_PAGE_*
    std::string block;        // Classes block chip name, "" = All
    std::string student;      // StudentProfileSummary::name, "" = roster
    std::string pairFileA;    // flagged pair: PairAnalysisDisplay::filenameA/B
    std::string pairFileB;    //   ("" = none); matched in either order
    std::string pairLabel;    // "NameA vs NameB", display only

    bool operator==(const WorkspaceState& o) const;
    bool operator!=(const WorkspaceState& o) const { return !(*this == o); }
};

// Same navigation + dataset (ignores savedAt/appVersion), i.e. "would
// saving this change anything that matters?"
bool sameWorkspaceContent(const WorkspaceState& a, const WorkspaceState& b);

std::string serializeWorkspace(const WorkspaceState& ws);

enum class WorkspaceParse {
    Ok,
    Malformed,      // not JSON CALSS wrote, wrong tag, bad types, too big
    FutureVersion,  // written by a newer CALSS; ignored, left on disk
    Invalid         // well-formed but unusable (no dataset, bad page)
};
// Never throws. On anything but Ok, `out` is left default.
WorkspaceParse parseWorkspace(const std::string& text, WorkspaceState& out);

// %LOCALAPPDATA%\CALSS\workspace.json (UTF-8).
std::string workspaceFilePath();

// Reads + parses the file. Missing file → returns false, result Malformed
// is NOT reported (missing is the normal first-run case): `status` is
// set only when a file was found.
bool loadWorkspaceFile(const std::string& path, WorkspaceState& out,
                       WorkspaceParse* status = nullptr);

// Writes path.tmp, then replaces path in one step (MoveFileExW
// REPLACE_EXISTING | WRITE_THROUGH), so a crash mid-save leaves the
// previous file intact. Returns false on any failure (old file kept).
bool saveWorkspaceFile(const std::string& path, const WorkspaceState& ws);

// ── Restoration decisions (pure) ─────────────────────────────

enum class DatasetCheck {
    Ok,
    Missing,           // path doesn't exist / can't be accessed
    MissingClassroom,  // ...and it was CALSS's own Classroom download folder
    NotAFolder,
    NoCFiles
};
DatasetCheck classifyDataset(bool exists, bool isDirectory, int cFileCount,
                             bool isClassroomDownload);

// Case-insensitive "path is inside dir" for Windows paths ('\' or '/').
bool pathIsInside(const std::string& path, const std::string& dir);

// Index of `name` in `names` (exact match — the pipeline's own student /
// block identity), or -1. Empty name → -1.
int findByName(const std::vector<std::string>& names, const std::string& name);

// Index of the pair whose two files are {a, b} in either order, or -1.
int findPair(const std::vector<std::pair<std::string, std::string>>& pairs,
             const std::string& a, const std::string& b);

// "Overview" / "Classes → Block → Student" / "Flagged Pairs → A vs B".
std::string describeLocation(const WorkspaceState& ws);

// Last path component of the dataset ("Programming1").
std::string datasetDisplayName(const std::string& path);

#endif // WORKSPACE_STATE_H
