// ─────────────────────────────────────────────────────────────
// app_paths.cpp
// See app_paths.h.
// ─────────────────────────────────────────────────────────────

#include "../include/app_paths.h"

#include <windows.h>
#include <shlobj.h>

static std::wstring dirOf(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"." : path.substr(0, slash);
}

static std::string dirOf(const std::string& path) {
    size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? "." : path.substr(0, slash);
}

// Function-local statics initialized once, thread-safely (C++11), since
// worker threads (analysis, Classroom sync) call these too.
std::wstring appExeDirW() {
    static const std::wstring dir = [] {
        wchar_t buf[MAX_PATH * 2];
        DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH * 2);
        return (n > 0 && n < MAX_PATH * 2) ? dirOf(std::wstring(buf, n)) : std::wstring(L".");
    }();
    return dir;
}

std::string appExeDirA() {
    static const std::string dir = [] {
        char buf[MAX_PATH * 2];
        DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH * 2);
        return (n > 0 && n < MAX_PATH * 2) ? dirOf(std::string(buf, n)) : std::string(".");
    }();
    return dir;
}

// <known folder>\<sub>, created if missing; the exe's folder on failure.
static std::string knownSubdirA(int csidl, const char* sub) {
    char base[MAX_PATH];
    if (SHGetFolderPathA(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, base) != S_OK)
        return appExeDirA();
    std::string dir = std::string(base) + "\\" + sub;
    CreateDirectoryA(dir.c_str(), nullptr); // no-op if it already exists
    DWORD attr = GetFileAttributesA(dir.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
        return appExeDirA();
    return dir;
}

std::string appDataDirA() {
    return knownSubdirA(CSIDL_LOCAL_APPDATA, "CALSS");
}

std::string appReportsDirA() {
    return knownSubdirA(CSIDL_PERSONAL, "CALSS Reports");
}
