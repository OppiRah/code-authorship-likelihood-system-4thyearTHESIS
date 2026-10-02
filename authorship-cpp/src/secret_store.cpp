// ─────────────────────────────────────────────────────────────
// secret_store.cpp — see secret_store.h
// ─────────────────────────────────────────────────────────────

#include "../include/secret_store.h"

#ifdef _WIN32
#include <windows.h>
#include <wincred.h>
#include <fstream>
#include <iterator>
#include <algorithm>

bool secretRead(const wchar_t* target, std::string& out) {
    out.clear();
    PCREDENTIALW cred = nullptr;
    if (!CredReadW(target, CRED_TYPE_GENERIC, 0, &cred) || !cred) return false;
    out.assign((const char*)cred->CredentialBlob, cred->CredentialBlobSize);
    SecureZeroMemory(cred->CredentialBlob, cred->CredentialBlobSize);
    CredFree(cred);
    return !out.empty();
}

// CRED_TYPE_GENERIC blob limit. Current Windows SDKs define
// CRED_MAX_CREDENTIAL_BLOB_SIZE as (5*512) = 2560 bytes; MinGW's
// wincred.h still has the old 512, which would wrongly reject any
// Classroom token over 512 bytes. Verified on Windows 11: CredWriteW
// accepts 2560 bytes and rejects 2561 (ERROR_BAD_LENGTH, 1783).
static const size_t SECRET_MAX_BLOB = 5 * 512;

static DWORD g_lastSecretError = 0;
unsigned long secretLastError() { return g_lastSecretError; }

bool secretWrite(const wchar_t* target, const std::string& value) {
    g_lastSecretError = 0;
    if (value.empty()) { g_lastSecretError = ERROR_INVALID_PARAMETER; return false; }
    if (value.size() > SECRET_MAX_BLOB) { g_lastSecretError = ERROR_BAD_LENGTH; return false; }
    CREDENTIALW cred = {};
    cred.Type               = CRED_TYPE_GENERIC;
    cred.TargetName         = const_cast<LPWSTR>(target);
    cred.CredentialBlobSize = (DWORD)value.size();
    cred.CredentialBlob     = (LPBYTE)const_cast<char*>(value.data());
    // This Windows user on this PC only — not roamed to other machines.
    cred.Persist            = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName           = const_cast<LPWSTR>(L"CALSS");
    if (CredWriteW(&cred, 0)) return true;
    g_lastSecretError = GetLastError();
    return false;
}

bool secretDelete(const wchar_t* target) {
    if (CredDeleteW(target, CRED_TYPE_GENERIC, 0)) return true;
    return GetLastError() == ERROR_NOT_FOUND;  // already gone counts as success
}

// ── Legacy file migration ─────────────────────────────────────

static std::wstring fullPathOf(const std::wstring& p) {
    wchar_t buf[MAX_PATH * 2];
    DWORD n = GetFullPathNameW(p.c_str(), MAX_PATH * 2, buf, nullptr);
    return (n > 0 && n < MAX_PATH * 2) ? std::wstring(buf, n) : p;
}

// Distinct existing copies of `name` in the exe's folder and the
// current working folder (the old code read from the working folder,
// which was normally — but not always — the exe's folder).
static std::vector<std::wstring> legacyCandidates(const wchar_t* name) {
    std::vector<std::wstring> dirs;
    wchar_t exe[MAX_PATH * 2];
    DWORD n = GetModuleFileNameW(nullptr, exe, MAX_PATH * 2);
    if (n > 0 && n < MAX_PATH * 2) {
        std::wstring e(exe, n);
        size_t slash = e.find_last_of(L"\\/");
        if (slash != std::wstring::npos) dirs.push_back(e.substr(0, slash));
    }
    wchar_t cwd[MAX_PATH * 2];
    DWORD c = GetCurrentDirectoryW(MAX_PATH * 2, cwd);
    if (c > 0 && c < MAX_PATH * 2) dirs.push_back(std::wstring(cwd, c));

    std::vector<std::wstring> found;
    for (const auto& d : dirs) {
        std::wstring p = fullPathOf(d + L"\\" + name);
        DWORD attr = GetFileAttributesW(p.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) continue;
        bool dup = false;
        for (const auto& f : found)
            if (CompareStringOrdinal(f.c_str(), -1, p.c_str(), -1, TRUE) == CSTR_EQUAL) dup = true;
        if (!dup) found.push_back(p);
    }
    return found;
}

static bool readWholeFile(const std::wstring& path, std::string& out) {
    std::ifstream f(path.c_str(), std::ios::binary);
    if (!f.is_open()) return false;
    out.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return true;
}

static std::string trimmed(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t'))
        s.pop_back();
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    return s.substr(i);
}

// Write → read back → compare → delete. Returns true only if the file
// was imported AND removed.
static bool importAndDelete(const wchar_t* target, const std::string& value,
                            const std::wstring& path, LegacySecretMigration& m) {
    std::string check;
    if (!secretWrite(target, value) || !secretRead(target, check) || check != value) {
        m.failedFiles.push_back(path);
        return false;
    }
    if (!DeleteFileW(path.c_str())) {
        m.failedFiles.push_back(path);
        return false;
    }
    m.deletedFiles.push_back(path);
    return true;
}

LegacySecretMigration migrateLegacySecretFiles() {
    LegacySecretMigration m;

    // Gemini key: first line of api_key.txt, trimmed. The shipped
    // placeholder text is not a key — leave such a file alone.
    for (const auto& path : legacyCandidates(LEGACY_GEMINI_KEY_FILE)) {
        std::string raw;
        if (!readWholeFile(path, raw)) { m.failedFiles.push_back(path); continue; }
        std::string key = trimmed(raw.substr(0, raw.find('\n')));
        if (key.empty() || key == "PASTE_YOUR_GEMINI_API_KEY_HERE") continue;
        if (importAndDelete(SECRET_GEMINI_KEY, key, path, m)) m.geminiMigrated = true;
    }

    // Classroom token: the whole JSON blob, stored byte-for-byte so the
    // existing loader parses it exactly as before.
    for (const auto& path : legacyCandidates(LEGACY_GC_TOKEN_FILE)) {
        std::string json;
        if (!readWholeFile(path, json)) { m.failedFiles.push_back(path); continue; }
        if (json.find("\"access_token\"") == std::string::npos &&
            json.find("\"refresh_token\"") == std::string::npos) {
            m.failedFiles.push_back(path);  // not a token file we understand — keep it
            continue;
        }
        if (importAndDelete(SECRET_GC_TOKEN, json, path, m)) m.tokenMigrated = true;
    }
    return m;
}

#else  // non-Windows builds: no secret store

bool secretRead(const wchar_t*, std::string& out) { out.clear(); return false; }
bool secretWrite(const wchar_t*, const std::string&) { return false; }
unsigned long secretLastError() { return 0; }
bool secretDelete(const wchar_t*) { return true; }
LegacySecretMigration migrateLegacySecretFiles() { return {}; }

#endif
