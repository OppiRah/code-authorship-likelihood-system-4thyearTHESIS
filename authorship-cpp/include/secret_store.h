#ifndef SECRET_STORE_H
#define SECRET_STORE_H
// ─────────────────────────────────────────────────────────────
// secret_store.h
// Per-user secret storage for CALSS — the Gemini API key and the
// Google Classroom OAuth token live in Windows Credential Manager
// (generic credentials, persisted for this Windows user on this PC,
// encrypted by Windows) instead of plaintext files next to the exe.
// No path dependency: unlike the old api_key.txt / calss_gc_token.json
// lookups, nothing here depends on the process's working directory.
// ─────────────────────────────────────────────────────────────

#include <string>
#include <vector>

#define SECRET_GEMINI_KEY  L"CALSS/GeminiApiKey"
#define SECRET_GC_TOKEN    L"CALSS/GoogleClassroomToken"

// The pre-Credential-Manager plaintext files (one-time migration only).
#define LEGACY_GEMINI_KEY_FILE L"api_key.txt"
#define LEGACY_GC_TOKEN_FILE   L"calss_gc_token.json"

bool secretRead(const wchar_t* target, std::string& out);
// Returns false on failure — never falls back to a file. The Windows
// error code of the last failed write is available from secretLastError().
bool secretWrite(const wchar_t* target, const std::string& value);
unsigned long secretLastError();
bool secretDelete(const wchar_t* target);

struct LegacySecretMigration {
    bool geminiMigrated = false;   // api_key.txt imported + deleted
    bool tokenMigrated  = false;   // calss_gc_token.json imported + deleted
    std::vector<std::wstring> deletedFiles;  // full paths removed
    std::vector<std::wstring> failedFiles;   // found but NOT migrated (kept)
    bool anything() const { return geminiMigrated || tokenMigrated || !failedFiles.empty(); }
};

// One-time import of the legacy plaintext files. Looks in the exe's
// folder and the current working folder; for each file found it
// writes the content to Credential Manager, reads it back to verify
// it matches, and only then deletes the file. A file that can't be
// imported is left in place and reported in failedFiles. Idempotent —
// once the files are gone this does nothing.
LegacySecretMigration migrateLegacySecretFiles();

#endif
