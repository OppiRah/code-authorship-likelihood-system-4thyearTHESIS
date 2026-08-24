// ─────────────────────────────────────────────────────────────
// Google Classroom Integration — CALSS
// OAuth 2.0 Installed App + WinHTTP + Winsock
// ─────────────────────────────────────────────────────────────

// Winsock2 MUST come before windows.h
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <shlobj.h>

#include "../include/google_classroom.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <filesystem>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")

// ── Simple JSON helpers ───────────────────────────────────────
// Extracts the value of "key":"..." from a JSON string (no library needed)
static std::string jsonGet(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) ++pos;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        ++pos;
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    }
    // numeric
    size_t end = pos;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']')
        ++end;
    return json.substr(pos, end - pos);
}

// Extract all objects from a JSON array field
static std::vector<std::string> jsonArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return result;
    pos = json.find('[', pos);
    if (pos == std::string::npos) return result;
    ++pos;
    int depth = 1;
    size_t objStart = std::string::npos;
    for (size_t i = pos; i < json.size(); ++i) {
        if (json[i] == '{') {
            if (depth == 1) objStart = i;
            ++depth;
        } else if (json[i] == '}') {
            --depth;
            if (depth == 1 && objStart != std::string::npos) {
                result.push_back(json.substr(objStart, i - objStart + 1));
                objStart = std::string::npos;
            }
        } else if (json[i] == ']' && depth == 1) {
            break;
        }
    }
    return result;
}

// URL encode a string
static std::string urlEncode(const std::string& s) {
    std::ostringstream oss;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        } else {
            oss << '%' << std::hex << std::uppercase
                << (int)(c >> 4) << (int)(c & 0x0F);
        }
    }
    return oss.str();
}

// wstring <-> string
static std::wstring toWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    return w;
}
static std::string toNarrow(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    return s;
}

// ── Token storage ─────────────────────────────────────────────
static GCToken g_token;
static bool    g_tokenLoaded = false;

static bool loadToken() {
    std::ifstream f(GC_TOKEN_FILE);
    if (!f.is_open()) return false;
    std::string json((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());
    g_token.accessToken  = jsonGet(json, "access_token");
    g_token.refreshToken = jsonGet(json, "refresh_token");
    std::string exp = jsonGet(json, "expires_at");
    g_token.expiresAt = exp.empty() ? 0 : std::stoll(exp);
    g_tokenLoaded = !g_token.accessToken.empty();
    return g_tokenLoaded;
}

static void saveToken() {
    std::ofstream f(GC_TOKEN_FILE);
    f << "{\n"
      << "  \"access_token\": \"" << g_token.accessToken << "\",\n"
      << "  \"refresh_token\": \"" << g_token.refreshToken << "\",\n"
      << "  \"expires_at\": " << g_token.expiresAt << "\n"
      << "}\n";
}

bool gc_hasValidToken() {
    if (!g_tokenLoaded) loadToken();
    if (!g_tokenLoaded) return false;
    long long now = (long long)std::time(nullptr);
    return (now < g_token.expiresAt - 60);
}

void gc_signOut() {
    g_token = {};
    g_tokenLoaded = false;
    DeleteFileA(GC_TOKEN_FILE);
}

// ── WinHTTP HTTPS helper ──────────────────────────────────────
struct HttpResponse {
    bool        success;
    int         statusCode;
    std::string body;
    std::string error;
};

static HttpResponse httpPost(const std::string& host,
                               const std::string& path,
                               const std::string& body,
                               const std::string& contentType,
                               const std::string& authHeader = "") {
    HttpResponse res = {};
    HINTERNET hSession = WinHttpOpen(L"CALSS/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { res.error = "WinHttpOpen failed"; return res; }

    HINTERNET hConnect = WinHttpConnect(hSession, toWide(host).c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); res.error = "Connect failed"; return res; }

    HINTERNET hReq = WinHttpOpenRequest(hConnect, L"POST",
        toWide(path).c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hReq) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return res; }

    std::wstring ct = toWide("Content-Type: " + contentType + "\r\n");
    WinHttpAddRequestHeaders(hReq, ct.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
    if (!authHeader.empty()) {
        std::wstring ah = toWide("Authorization: " + authHeader + "\r\n");
        WinHttpAddRequestHeaders(hReq, ah.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
    }

    WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        (LPVOID)body.c_str(), (DWORD)body.size(), (DWORD)body.size(), 0);
    WinHttpReceiveResponse(hReq, nullptr);

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
    res.statusCode = (int)statusCode;

    DWORD bytesAvail = 0;
    while (WinHttpQueryDataAvailable(hReq, &bytesAvail) && bytesAvail > 0) {
        std::string chunk(bytesAvail, '\0');
        DWORD bytesRead = 0;
        WinHttpReadData(hReq, &chunk[0], bytesAvail, &bytesRead);
        res.body.append(chunk.data(), bytesRead);
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    res.success = (statusCode >= 200 && statusCode < 300);
    return res;
}

static HttpResponse httpGet(const std::string& host,
                              const std::string& path,
                              const std::string& authToken,
                              bool binary = false) {
    HttpResponse res = {};
    HINTERNET hSession = WinHttpOpen(L"CALSS/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { res.error = "WinHttpOpen failed"; return res; }

    HINTERNET hConnect = WinHttpConnect(hSession, toWide(host).c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return res; }

    HINTERNET hReq = WinHttpOpenRequest(hConnect, L"GET",
        toWide(path).c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hReq) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return res; }

    std::wstring auth = toWide("Authorization: Bearer " + authToken + "\r\n");
    WinHttpAddRequestHeaders(hReq, auth.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

    WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    WinHttpReceiveResponse(hReq, nullptr);

    DWORD statusCode = 0;
    DWORD sz = sizeof(statusCode);
    WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &sz, WINHTTP_NO_HEADER_INDEX);
    res.statusCode = (int)statusCode;

    DWORD bytesAvail = 0;
    while (WinHttpQueryDataAvailable(hReq, &bytesAvail) && bytesAvail > 0) {
        std::string chunk(bytesAvail, '\0');
        DWORD bytesRead = 0;
        WinHttpReadData(hReq, &chunk[0], bytesAvail, &bytesRead);
        res.body.append(chunk.data(), bytesRead);
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    res.success = (statusCode >= 200 && statusCode < 300);
    return res;
}

// ── Credentials JSON parsing ──────────────────────────────────
bool gc_loadCredentials(const std::string& jsonPath, GCCredentials& out) {
    std::ifstream f(jsonPath);
    if (!f.is_open()) return false;
    std::string json((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());
    // Navigate into "installed": { ... }
    size_t pos = json.find("\"installed\"");
    if (pos == std::string::npos) return false;
    pos = json.find('{', pos + 1);
    if (pos == std::string::npos) return false;
    size_t end = json.find('}', pos);
    if (end == std::string::npos) return false;
    std::string inner = json.substr(pos, end - pos + 1);

    out.clientId     = jsonGet(inner, "client_id");
    out.clientSecret = jsonGet(inner, "client_secret");
    out.authUri      = jsonGet(inner, "auth_uri");
    out.tokenUri     = jsonGet(inner, "token_uri");

    return !out.clientId.empty() && !out.clientSecret.empty();
}

// ─────────────────────────────────────────────────────────────
// Default embedded credentials for the CALSS project.
//
// This app is designed for a single deployment context (the CCS
// thesis project's own Google Cloud project), not as a general
// "bring your own API key" tool. Embedding the client ID/secret
// means the instructor never has to hunt down or upload a JSON
// credentials file — Sync just works on first click.
//
// Note: for OAuth "installed application" clients, the client
// secret is not treated as a confidential secret by Google's own
// documentation — the actual security boundary is the user's
// interactive sign-in and consent screen, not secrecy of this
// string. This is the same model used by other installed apps
// distributing a shared client ID.
//
// Even so, the actual values are kept out of source control (they
// live in GC_EMBEDDED_CREDS_FILE, gitignored) rather than hardcoded
// here, so the public repo doesn't ship them directly and GitHub's
// secret scanning has nothing to flag. A ship-time copy of that
// file must sit next to authorship.exe for zero-setup Sync to work;
// calss_gc_embedded_credentials.example.json documents its shape.
// ─────────────────────────────────────────────────────────────
static bool loadEmbeddedDefaults(GCCredentials& out) {
    std::ifstream f(GC_EMBEDDED_CREDS_FILE);
    if (!f.is_open()) return false;

    std::string json((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());
    out.clientId     = jsonGet(json, "client_id");
    out.clientSecret = jsonGet(json, "client_secret");
    out.authUri      = jsonGet(json, "auth_uri");
    out.tokenUri     = jsonGet(json, "token_uri");
    return !out.clientId.empty() && !out.clientSecret.empty();
}

bool gc_saveSettings(const GCCredentials& creds) {
    std::ofstream f(GC_SETTINGS_FILE);
    if (!f.is_open()) return false;
    f << "{\n"
      << "  \"client_id\": \""     << creds.clientId     << "\",\n"
      << "  \"client_secret\": \"" << creds.clientSecret << "\",\n"
      << "  \"auth_uri\": \""      << creds.authUri      << "\",\n"
      << "  \"token_uri\": \""     << creds.tokenUri     << "\"\n"
      << "}\n";
    return true;
}

bool gc_loadSettings(GCCredentials& creds) {
    std::ifstream f(GC_SETTINGS_FILE);

    if (!f.is_open()) {
        // No local override saved yet — fall back to the embedded
        // CALSS project credentials so Sync works with zero setup.
        if (!loadEmbeddedDefaults(creds)) return false;

        // Persist it locally too, so future runs (and the Settings
        // dialog, if ever opened) see the same values consistently.
        gc_saveSettings(creds);
        return true;
    }

    std::string json((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());
    creds.clientId     = jsonGet(json, "client_id");
    creds.clientSecret = jsonGet(json, "client_secret");
    creds.authUri      = jsonGet(json, "auth_uri");
    creds.tokenUri     = jsonGet(json, "token_uri");

    // If the saved file is somehow missing fields, fill from defaults
    // rather than failing outright.
    if (creds.clientId.empty() || creds.clientSecret.empty() ||
        creds.authUri.empty()  || creds.tokenUri.empty()) {
        GCCredentials defaults;
        if (loadEmbeddedDefaults(defaults)) {
            if (creds.clientId.empty())     creds.clientId     = defaults.clientId;
            if (creds.clientSecret.empty()) creds.clientSecret = defaults.clientSecret;
            if (creds.authUri.empty())      creds.authUri      = defaults.authUri;
            if (creds.tokenUri.empty())     creds.tokenUri     = defaults.tokenUri;
        }
    }

    return !creds.clientId.empty();
}

// ── Token exchange ────────────────────────────────────────────
static bool exchangeCode(const GCCredentials& creds, const std::string& code) {
    std::string body =
        "code="          + urlEncode(code)              +
        "&client_id="    + urlEncode(creds.clientId)    +
        "&client_secret="+ urlEncode(creds.clientSecret)+
        "&redirect_uri=" + urlEncode("http://localhost:8080") +
        "&grant_type=authorization_code";

    // token_uri: https://oauth2.googleapis.com/token
    auto res = httpPost("oauth2.googleapis.com", "/token", body,
                         "application/x-www-form-urlencoded");
    if (!res.success) return false;

    std::string access  = jsonGet(res.body, "access_token");
    std::string refresh = jsonGet(res.body, "refresh_token");
    std::string expin   = jsonGet(res.body, "expires_in");

    if (access.empty()) return false;

    g_token.accessToken  = access;
    if (!refresh.empty()) g_token.refreshToken = refresh;
    long long expSecs = expin.empty() ? 3600 : std::stoll(expin);
    g_token.expiresAt = (long long)std::time(nullptr) + expSecs;
    g_tokenLoaded = true;
    saveToken();
    return true;
}

bool gc_refreshIfNeeded(const GCCredentials& creds) {
    if (!g_tokenLoaded) loadToken();
    if (!g_tokenLoaded) return false;
    if (gc_hasValidToken()) return true;
    if (g_token.refreshToken.empty()) return false;

    std::string body =
        "client_id="     + urlEncode(creds.clientId)    +
        "&client_secret="+ urlEncode(creds.clientSecret)+
        "&refresh_token="+ urlEncode(g_token.refreshToken) +
        "&grant_type=refresh_token";

    auto res = httpPost("oauth2.googleapis.com", "/token", body,
                         "application/x-www-form-urlencoded");
    if (!res.success) return false;

    std::string access = jsonGet(res.body, "access_token");
    std::string expin  = jsonGet(res.body, "expires_in");
    if (access.empty()) return false;

    g_token.accessToken = access;
    long long expSecs = expin.empty() ? 3600 : std::stoll(expin);
    g_token.expiresAt = (long long)std::time(nullptr) + expSecs;
    saveToken();
    return true;
}

// ── OAuth browser flow with localhost listener ────────────────
// Listens on port 8080 for Google's redirect and extracts the code

static std::string listenForCode() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv == INVALID_SOCKET) { WSACleanup(); return ""; }

    // Allow port reuse
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    // Set timeout: 3 minutes
    DWORD timeout = 180000;
    setsockopt(srv, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(GC_AUTH_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(srv); WSACleanup(); return "";
    }
    listen(srv, 1);

    sockaddr_in clientAddr = {};
    int clientAddrLen = sizeof(clientAddr);
    SOCKET client = accept(srv, (sockaddr*)&clientAddr, &clientAddrLen);
    closesocket(srv);

    if (client == INVALID_SOCKET) { WSACleanup(); return ""; }

    // Read the HTTP request
    std::string request;
    char buf[4096];
    int bytesRecv = recv(client, buf, sizeof(buf) - 1, 0);
    if (bytesRecv > 0) {
        buf[bytesRecv] = '\0';
        request = buf;
    }

    // Send a nice response page
    const char* htmlResponse =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html><html><head>"
        "<style>body{font-family:Arial,sans-serif;background:#1E2026;color:#F5F2EB;"
        "display:flex;align-items:center;justify-content:center;height:100vh;margin:0;}"
        ".box{text-align:center;padding:40px;background:#2A2D34;border-radius:12px;"
        "border:1px solid #3A3D45;}"
        "h1{color:#D4AF37;font-size:24px;}"
        "p{color:#9A9AA0;}"
        "</style></head><body>"
        "<div class='box'>"
        "<h1>CALSS \u2014 Authentication Successful</h1>"
        "<p>You have successfully connected Google Classroom.</p>"
        "<p>You may now close this tab and return to the application.</p>"
        "</div></body></html>";

    send(client, htmlResponse, (int)strlen(htmlResponse), 0);
    closesocket(client);
    WSACleanup();

    // Parse code from GET /...?code=XXX
    std::string code;
    size_t pos = request.find("?code=");
    if (pos != std::string::npos) {
        pos += 6;
        size_t end = request.find_first_of("& \r\n", pos);
        code = request.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    } else {
        // Try without ?
        pos = request.find("code=");
        if (pos != std::string::npos) {
            pos += 5;
            size_t end = request.find_first_of("& \r\n", pos);
            code = request.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        }
    }
    return code;
}

bool gc_authenticate(HWND parent, const GCCredentials& creds, bool /*unused*/) {
    std::string scope = urlEncode(
        "https://www.googleapis.com/auth/classroom.courses.readonly "
        "https://www.googleapis.com/auth/classroom.coursework.students.readonly "
        "https://www.googleapis.com/auth/classroom.student-submissions.students.readonly "
        "https://www.googleapis.com/auth/classroom.rosters.readonly "
        "https://www.googleapis.com/auth/drive.readonly"
    );

    std::string authUrl =
        creds.authUri +
        "?client_id="    + urlEncode(creds.clientId) +
        "&redirect_uri=" + urlEncode("http://localhost:8080") +
        "&response_type=code" +
        "&scope="        + scope +
        "&access_type=offline" +
        "&prompt=consent";

    int confirm = MessageBoxW(parent,
        L"CALSS will open your browser to connect Google Classroom.\n\n"
        L"1. Sign in with your Google account\n"
        L"2. Grant the requested permissions\n"
        L"3. You'll see a success page — then return here\n\n"
        L"Click OK to open the browser.",
        L"Connect Google Classroom",
        MB_OKCANCEL | MB_ICONINFORMATION);

    if (confirm != IDOK) return false;

    // Open browser to auth URL
    ShellExecuteA(nullptr, "open", authUrl.c_str(), nullptr, nullptr, SW_SHOW);

    // Listen for the OAuth callback (blocks until received or timeout)
    std::string code = listenForCode();

    if (code.empty()) {
        MessageBoxW(parent,
            L"Authentication timed out or was cancelled.\n"
            L"Please try again.",
            L"Authentication Failed",
            MB_OK | MB_ICONWARNING);
        return false;
    }

    // Exchange code for tokens
    if (!exchangeCode(creds, code)) {
        MessageBoxW(parent,
            L"Failed to exchange authorization code for tokens.\n"
            L"Check your credentials and try again.",
            L"Authentication Failed",
            MB_OK | MB_ICONERROR);
        return false;
    }

    MessageBoxW(parent,
        L"Google Classroom connected successfully!\n\n"
        L"You can now sync student submissions.",
        L"Connected",
        MB_OK | MB_ICONINFORMATION);
    return true;
}

// ── API calls ─────────────────────────────────────────────────
static std::string getAccessToken(const GCCredentials& creds) {
    if (!g_tokenLoaded) loadToken();
    if (!gc_hasValidToken()) gc_refreshIfNeeded(creds);
    return g_token.accessToken;
}

std::vector<GCourse> gc_getCourses(const GCCredentials& creds) {
    std::vector<GCourse> result;
    std::string token = getAccessToken(creds);
    if (token.empty()) return result;

    auto res = httpGet("classroom.googleapis.com",
        "/v1/courses?courseStates=ACTIVE&pageSize=30", token);
    if (!res.success) return result;

    auto objs = jsonArray(res.body, "courses");
    for (const auto& obj : objs) {
        GCourse c;
        c.id             = jsonGet(obj, "id");
        c.name           = jsonGet(obj, "name");
        c.section        = jsonGet(obj, "section");
        c.enrollmentCode = jsonGet(obj, "enrollmentCode");
        if (!c.id.empty()) result.push_back(c);
    }
    return result;
}

std::vector<GAssignment> gc_getAssignments(const GCCredentials& creds,
                                              const std::string& courseId) {
    std::vector<GAssignment> result;
    std::string token = getAccessToken(creds);
    if (token.empty()) return result;

    auto res = httpGet("classroom.googleapis.com",
        "/v1/courses/" + courseId + "/courseWork?pageSize=30", token);
    if (!res.success) return result;

    auto objs = jsonArray(res.body, "courseWork");
    for (const auto& obj : objs) {
        GAssignment a;
        a.id          = jsonGet(obj, "id");
        a.title       = jsonGet(obj, "title");
        a.description = jsonGet(obj, "description");
        if (!a.id.empty()) result.push_back(a);
    }
    return result;
}

// ── Submission download ───────────────────────────────────────

// Get Drive file content
static bool downloadDriveFile(const std::string& token,
                                const std::string& fileId,
                                const std::string& destPath) {
    auto res = httpGet("www.googleapis.com",
        "/drive/v3/files/" + fileId + "?alt=media", token, true);
    if (!res.success || res.body.empty()) return false;

    std::ofstream out(destPath, std::ios::binary);
    if (!out.is_open()) return false;
    out.write(res.body.data(), res.body.size());
    return true;
}

// Extract student name from email (first part before @)
static std::string nameFromEmail(const std::string& email) {
    size_t at = email.find('@');
    std::string name = (at != std::string::npos) ? email.substr(0, at) : email;
    // Capitalize first letter
    if (!name.empty()) name[0] = (char)toupper((unsigned char)name[0]);
    return name;
}

// ── Sync manifest (for human-readable display only) ────────────
// Maps a downloaded filename back to the original Classroom assignment
// title and course section ("block"), since the filename itself is
// encoded for reliable parsing (category tag + "__" + student name +
// fingerprint) rather than for human readability. The GUI reads this to
// show "Midterm Exam" and group students by block ("BSIT 4") instead of
// a raw encoded filename in places like the Submissions Used panel and
// the Students tab.
static void appendManifestEntry(const std::string& outputFolder,
                                  const std::string& filename,
                                  const std::string& assignmentTitle,
                                  const std::string& blockName) {
    std::ofstream f(outputFolder + "\\calss_sync_manifest.jsonl", std::ios::app);
    if (!f.is_open()) return;
    auto sanitize = [](const std::string& s) {
        std::string safe;
        for (char c : s) if (c != '"' && c != '\\') safe += c;
        return safe;
    };
    f << "{\"file\":\"" << filename << "\","
      << "\"title\":\"" << sanitize(assignmentTitle) << "\","
      << "\"block\":\"" << sanitize(blockName) << "\"}\n";
}

std::map<std::string,SyncManifestEntry> gc_loadSyncManifest(const std::string& folder) {
    std::map<std::string,SyncManifestEntry> manifest;
    std::ifstream f(folder + "\\calss_sync_manifest.jsonl");
    if (!f.is_open()) return manifest;

    auto extractField = [](const std::string& line, const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t pos = line.find(search);
        if (pos == std::string::npos) return "";
        pos += search.size();
        size_t end = line.find('"', pos);
        if (end == std::string::npos) return "";
        return line.substr(pos, end - pos);
    };

    std::string line;
    while (std::getline(f, line)) {
        std::string file = extractField(line, "file");
        if (file.empty()) continue;
        SyncManifestEntry entry;
        entry.title = extractField(line, "title");
        entry.block = extractField(line, "block");
        manifest[file] = entry;
    }
    return manifest;
}

GCResult gc_downloadSubmissions(HWND parent,
                                  const GCCredentials& creds,
                                  const std::string& courseId,
                                  const std::string& assignmentId,
                                  const std::string& assignmentTitle,
                                  const std::string& blockName,
                                  const std::string& outputFolder) {
    GCResult result = {false, "", 0};
    std::string token = getAccessToken(creds);
    if (token.empty()) { result.error = "Not authenticated"; return result; }

    std::filesystem::create_directories(outputFolder);

    // ── Priority Signal 1: Classroom assignment title ─────────
    // If the instructor named the assignment with a period keyword
    // (Prelim/Midterm/Semifinal/Final) and/or a type keyword
    // (Exam/Quiz/Activity), that is a deterministic, instructor-authored
    // signal — more reliable than guessing from filename alone.
    //
    // Both period AND type are captured (not just type) because exam
    // files from different periods must never be pairwise-compared
    // against each other — a Midterm Exam and a Final Exam cover
    // completely different problems, so comparing them produces
    // statistically meaningless noise. See the period-aware grouping
    // in main.cpp's pairwise comparison loop.
    std::string titleLower = assignmentTitle;
    std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::tolower);

    // Detect period first. Check "semifinal" before "final" since
    // "semifinal" contains "final" as a substring.
    std::string period;
    if (titleLower.find("semifinal") != std::string::npos ||
        titleLower.find("semi-final") != std::string::npos ||
        titleLower.find("semi final") != std::string::npos) {
        period = "semifinal";
    } else if (titleLower.find("midterm") != std::string::npos ||
               titleLower.find("mid-term") != std::string::npos ||
               titleLower.find("mid term") != std::string::npos) {
        period = "midterm";
    } else if (titleLower.find("final") != std::string::npos) {
        period = "final";
    } else if (titleLower.find("prelim") != std::string::npos) {
        period = "prelim";
    }
    // If no period keyword found, period stays empty — the file will
    // be grouped under a single "general" bucket for comparison purposes.

    // Detect type independently of period.
    std::string type;
    if (titleLower.find("quiz") != std::string::npos) {
        type = "quiz";
    } else if (titleLower.find("activity") != std::string::npos ||
               titleLower.find("seatwork") != std::string::npos ||
               titleLower.find("lab")      != std::string::npos) {
        type = "activity";
        // Try to extract the activity number if present (e.g. "Activity 2")
        size_t actPos = titleLower.find("activity");
        if (actPos != std::string::npos) {
            size_t scanPos = actPos + 8; // length of "activity"
            while (scanPos < titleLower.size() &&
                   !isdigit((unsigned char)titleLower[scanPos]) &&
                   scanPos < actPos + 12) {
                ++scanPos;
            }
            if (scanPos < titleLower.size() && isdigit((unsigned char)titleLower[scanPos])) {
                type += titleLower[scanPos];
            }
        }
    } else if (titleLower.find("exam") != std::string::npos || !period.empty()) {
        // A bare period name with no other type keyword (e.g. just
        // "Finals") is treated as that period's exam by default.
        type = "exam";
    }

    // Combine into a single category tag, then a "__" delimiter before
    // the student name — this makes parsing on the read side (main.cpp)
    // unambiguous regardless of what characters appear in either part.
    std::string categoryTag = period + type; // e.g. "midtermexam", "finalactivity2"

    // Short, stable per-assignment fingerprint (sanitized, alphanumeric
    // only) used purely to keep filenames unique across different
    // assignments in the same category — see destFile construction below.
    std::string assignmentFingerprint;
    for (char c : assignmentId) {
        if (isalnum((unsigned char)c)) assignmentFingerprint += c;
    }
    if (assignmentFingerprint.size() > 6)
        assignmentFingerprint = assignmentFingerprint.substr(
            assignmentFingerprint.size() - 6);

    std::string path = "/v1/courses/" + courseId + "/courseWork/" +
                       assignmentId + "/studentSubmissions?pageSize=100";

    auto subRes = httpGet("classroom.googleapis.com", path, token);
    if (!subRes.success) {
        result.error = "Failed to fetch submissions (HTTP " +
                       std::to_string(subRes.statusCode) + "):\n" + subRes.body;
        return result;
    }

    auto submissions = jsonArray(subRes.body, "studentSubmissions");
    if (submissions.empty()) {
        result.error = "No submissions found for this assignment.";
        return result;
    }

    int downloaded = 0;
    int skipped    = 0;
    bool scopeWarningShown = false; // only warn once per sync call, not per submission

    for (const auto& sub : submissions) {
        try {
            std::string state  = jsonGet(sub, "state");
            std::string userId = jsonGet(sub, "userId");

            if (userId.empty()) { ++skipped; continue; }
            if (state != "TURNED_IN" && state != "RETURNED") { ++skipped; continue; }

            // Stable fallback: derived from the student's permanent Google
            // user ID, not a per-call counter. This guarantees the SAME
            // real person gets the SAME fallback name every time this
            // function runs, across every assignment sync — so even if
            // the real-name lookup below fails, files never get
            // misattributed to the wrong person or collide with a
            // different student who happened to get the same counter
            // value in an earlier/later sync call.
            std::string idSuffix = userId.size() >= 8
                ? userId.substr(userId.size() - 8) : userId;
            std::string studentName = "Student_" + idSuffix;

            auto profileRes = httpGet("classroom.googleapis.com",
                "/v1/userProfiles/" + userId, token);

            if (profileRes.success && !profileRes.body.empty()) {
                std::string given  = jsonGet(profileRes.body, "givenName");
                std::string family = jsonGet(profileRes.body, "familyName");
                if (!given.empty() || !family.empty()) {
                    studentName = given;
                    if (!family.empty()) studentName += family;
                } else {
                    std::string full = jsonGet(profileRes.body, "name");
                    if (!full.empty()) studentName = full;
                }
                // Sanitize for filename
                std::string safe;
                for (char c : studentName) {
                    if (isalnum((unsigned char)c) || c == '_' || c == '-')
                        safe += c;
                    else if (c == ' ')
                        safe += '_';
                }
                if (!safe.empty()) studentName = safe;
            } else if ((profileRes.statusCode == 401 || profileRes.statusCode == 403)
                       && !scopeWarningShown) {
                // The previously-granted token doesn't have permission to
                // read student names — most likely because this app was
                // authorized before the roster-reading permission was
                // added. Sign the user out so their NEXT sync attempt
                // triggers a fresh consent screen that includes it.
                // (Files still download successfully in the meantime,
                // just using the stable ID-based fallback name above.)
                gc_signOut();
                scopeWarningShown = true;
            }

            // Find attachments section
            size_t attPos = sub.find("\"attachments\"");
            if (attPos == std::string::npos) { ++skipped; continue; }

            size_t searchPos = attPos;
            bool foundFile   = false;
            int subFileIdx   = 0;

            while (true) {
                size_t drivePos = sub.find("\"driveFile\"", searchPos);
                if (drivePos == std::string::npos) break;

                size_t objStart = sub.find('{', drivePos);
                if (objStart == std::string::npos) break;

                int depth    = 1;
                size_t objEnd = objStart + 1;
                while (objEnd < sub.size() && depth > 0) {
                    if (sub[objEnd] == '{') ++depth;
                    else if (sub[objEnd] == '}') --depth;
                    ++objEnd;
                }
                if (depth != 0) break; // Malformed JSON safety

                std::string driveObj = sub.substr(objStart, objEnd - objStart);
                std::string fileId   = jsonGet(driveObj, "id");
                std::string fileName = jsonGet(driveObj, "title");
                std::string mime     = jsonGet(driveObj, "mimeType");

                searchPos = objEnd;
                if (fileId.empty()) continue;

                // Accept .c files or C source MIME types
                bool isCFile = false;
                if (fileName.size() >= 2 &&
                    fileName.substr(fileName.size() - 2) == ".c")
                    isCFile = true;
                if (mime == "text/x-csrc" || mime == "text/x-c")
                    isCFile = true;
                // If no extension clue, try downloading and check — skip for now

                if (!isCFile) continue;

                std::string suffix = (subFileIdx > 0)
                    ? ("_" + std::to_string(subFileIdx)) : "";

                // Filename structure: {period}{type}{num}__{studentName}{suffix}_ASGN{fingerprint}.c
                //
                // The "__" delimiter separates the category tag (period +
                // type, e.g. "midtermexam", "finalactivity2") from the
                // student name portion. This is deliberately unambiguous —
                // main.cpp splits on the LAST "__" to recover the category
                // tag and student name independently, regardless of what
                // characters appear in either part.
                //
                // If no period/type was detected from the assignment title
                // (categoryTag empty), we still include the delimiter so
                // the file is recognized as Classroom-sourced with an
                // unknown category, falling into Priority Signal 2
                // (filename-pattern fallback) on the read side.
                //
                // assignmentFingerprint disambiguates different Classroom
                // assignments that would otherwise share the identical
                // category tag (e.g. two separate "Activity 1" assignments
                // posted in different periods) — without it they would
                // download to the same filename and silently overwrite
                // each other during a bulk multi-assignment sync.
                std::string destFile = outputFolder + "\\" +
                    categoryTag + "__" +
                    studentName + suffix +
                    "_ASGN" + assignmentFingerprint + ".c";

                if (downloadDriveFile(token, fileId, destFile)) {
                    ++downloaded;
                    ++subFileIdx;
                    foundFile = true;

                    // Extract just the filename (no path) for the manifest key
                    size_t lastSlash = destFile.find_last_of("\\/");
                    std::string bareName = (lastSlash != std::string::npos)
                        ? destFile.substr(lastSlash + 1) : destFile;
                    appendManifestEntry(outputFolder, bareName, assignmentTitle, blockName);
                }
            }

            if (!foundFile) ++skipped;

        } catch (...) {
            // Skip any submission that causes an exception
            ++skipped;
            continue;
        }
    }

    result.success         = true; // Return true even if some skipped
    result.filesDownloaded = downloaded;
    if (downloaded == 0) {
        result.success = false;
        result.error   = "No .c files were downloaded.\n"
                         "Checked " + std::to_string(submissions.size()) +
                         " submissions, " + std::to_string(skipped) + " had no .c attachments.\n\n"
                         "Make sure students submitted .c files (not Google Docs or other formats).";
    }
    return result;
}