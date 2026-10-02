// ─────────────────────────────────────────────────────────────
// app_paths.cpp
// See app_paths.h.
// ─────────────────────────────────────────────────────────────

#include "../include/app_paths.h"

#include <windows.h>
#include <shlobj.h>

std::wstring widenUtf8(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), &w[0], n);
    return w;
}

std::string utf8FromWide(const std::wstring& wide) {
    if (wide.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(),
                                nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(),
                        &s[0], n, nullptr, nullptr);
    return s;
}

std::filesystem::path pathFromUtf8(const std::string& utf8) {
    return std::filesystem::path(widenUtf8(utf8));
}

static std::wstring dirOf(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"." : path.substr(0, slash);
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

std::string appExeDirU8() {
    static const std::string dir = utf8FromWide(appExeDirW());
    return dir;
}

// <known folder>\<sub> as UTF-8, created if missing; the exe's folder
// on failure.
static std::string knownSubdirU8(int csidl, const wchar_t* sub) {
    wchar_t base[MAX_PATH];
    if (SHGetFolderPathW(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, base) != S_OK)
        return appExeDirU8();
    std::wstring dir = std::wstring(base) + L"\\" + sub;
    CreateDirectoryW(dir.c_str(), nullptr); // no-op if it already exists
    DWORD attr = GetFileAttributesW(dir.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
        return appExeDirU8();
    return utf8FromWide(dir);
}

std::string appDataDirU8() {
    return knownSubdirU8(CSIDL_LOCAL_APPDATA, L"CALSS");
}

std::string appReportsDirU8() {
    return knownSubdirU8(CSIDL_PERSONAL, L"CALSS Reports");
}
