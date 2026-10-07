// ─────────────────────────────────────────────────────────────
// gemini.cpp
// Gemini API integration — calls API and parses response
// Uses WinHTTP (Windows native) for HTTPS — no external libs needed
// ─────────────────────────────────────────────────────────────

#include "../include/secret_store.h"
#include "../include/gemini.h"
#include "../include/app_paths.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <functional>
#include <mutex>
#include <regex>
#include <set>
#include <cstdlib>

#ifdef _WIN32
  #include <windows.h>
  #include <winhttp.h>
  #pragma comment(lib, "winhttp.lib")
#endif

// ═════════════════════════════════════════════════════════════
// HELPERS
// ═════════════════════════════════════════════════════════════

// Escape a string for safe JSON inclusion
static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

// Convert std::string to wide string for WinHTTP
static std::wstring toWide(const std::string& s) {
    if (s.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(sz - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], sz);
    return w;
}

// Extract the text response from Gemini's JSON response
// Looks for: "text": "..."
static std::string extractTextFromJson(const std::string& json) {
    // Simple parser - find the FIRST "text" field after "parts"
    size_t partsPos = json.find("\"parts\"");
    if (partsPos == std::string::npos) return "";

    size_t textPos = json.find("\"text\"", partsPos);
    if (textPos == std::string::npos) return "";

    size_t colon = json.find(':', textPos);
    if (colon == std::string::npos) return "";

    size_t quoteStart = json.find('"', colon);
    if (quoteStart == std::string::npos) return "";

    std::string result;
    size_t i = quoteStart + 1;
    while (i < json.size()) {
        char c = json[i];
        if (c == '\\' && i + 1 < json.size()) {
            char next = json[i + 1];
            switch (next) {
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'u':
                    // Unicode escape - just skip for simplicity
                    if (i + 5 < json.size()) i += 4;
                    break;
                default: result += next;
            }
            i += 2;
        }
        else if (c == '"') {
            // End of string
            break;
        }
        else {
            result += c;
            ++i;
        }
    }
    return result;
}

// ═════════════════════════════════════════════════════════════
// API KEY LOADING
// ═════════════════════════════════════════════════════════════

std::string loadApiKey() {
    // Windows Credential Manager (see secret_store.h) — no file, no
    // dependency on the working directory.
    std::string key;
    if (!secretRead(SECRET_GEMINI_KEY, key)) {
        // Not stored yet: one-time import of a legacy api_key.txt. The
        // GUI runs this at startup and tells the user; this covers the
        // command-line path, which reports it on stderr.
        LegacySecretMigration m = migrateLegacySecretFiles();
        if (m.geminiMigrated)
            fprintf(stderr, "CALSS: moved api_key.txt into Windows Credential Manager "
                            "and deleted the file.\n");
        if (m.tokenMigrated)
            fprintf(stderr, "CALSS: moved calss_gc_token.json into Windows Credential "
                            "Manager and deleted the file.\n");
        if (!secretRead(SECRET_GEMINI_KEY, key)) return "";
    }

    // Trim whitespace
    while (!key.empty() && (key.back() == '\n' || key.back() == '\r' ||
                             key.back() == ' '  || key.back() == '\t'))
        key.pop_back();
    while (!key.empty() && (key.front() == ' ' || key.front() == '\t'))
        key.erase(key.begin());
    return key;
}

bool gemini_hasKey() { return !loadApiKey().empty(); }

bool gemini_saveKey(const std::string& key) {
    return !key.empty() && secretWrite(SECRET_GEMINI_KEY, key);
}

bool gemini_removeKey() { return secretDelete(SECRET_GEMINI_KEY); }

// ═════════════════════════════════════════════════════════════
// API CALL
// ═════════════════════════════════════════════════════════════
//
// Up to 1.0.0 every request went to one hard-coded model,
// "gemini-2.5-flash-lite", and "Test key" was a GET of that model. Google
// now limits the 2.5 models to accounts that already used them ("For any
// new projects, use our latest models", ai.google.dev/gemini-api/docs/
// models), so a perfectly valid key from a new project failed. Now:
//
//   models.list (GET /v1beta/models, free)  ->  filter + rank (below)
//     ->  cache per key for this session  ->  generateContent on the pick
//     ->  if Google says that model is unavailable (404 etc.): mark it,
//         list again, use the next candidate, retry ONCE; on 503
//         "overloaded" also try the next candidate once; anything else
//         (bad key, quota, network) is reported without a retry.
//
// Both retry cases are replies where nothing was generated, and there is
// at most one, so a failure can't multiply cost.
// The key goes only into the x-goog-api-key header of requests to
// generativelanguage.googleapis.com, never into a URL, log or message.

#ifdef _WIN32
namespace {

struct HttpReply {
    bool  answered = false;   // got an HTTP status back
    DWORD status   = 0;
    std::string body;
};

// One HTTPS request to the Gemini API host. Timeouts in ms.
HttpReply geminiRequest(const wchar_t* verb, const std::wstring& path,
                        const std::string& apiKey, const std::string* body,
                        int resolveMs, int connectMs, int sendMs, int receiveMs)
{
    HttpReply r;
    HINTERNET hSession = WinHttpOpen(L"AuthorshipScorer/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return r;
    // WinHTTP's defaults include an unlimited DNS wait; always bound it.
    WinHttpSetTimeouts(hSession, resolveMs, connectMs, sendMs, receiveMs);
    HINTERNET hConnect = WinHttpConnect(hSession, L"generativelanguage.googleapis.com",
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hRequest = hConnect ? WinHttpOpenRequest(hConnect, verb, path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
    if (hRequest) {
        std::wstring headers = L"x-goog-api-key: " + toWide(apiKey) + L"\r\n";
        if (body) headers = L"Content-Type: application/json\r\n" + headers;
        BOOL sent = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)-1L,
            body ? (LPVOID)body->data() : WINHTTP_NO_REQUEST_DATA,
            body ? (DWORD)body->size() : 0, body ? (DWORD)body->size() : 0, 0);
        SecureZeroMemory(&headers[0], headers.size() * sizeof(wchar_t));
        if (sent && WinHttpReceiveResponse(hRequest, nullptr)) {
            DWORD sz = sizeof(r.status);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                nullptr, &r.status, &sz, nullptr);
            r.answered = true;
            DWORD avail = 0;
            do {
                avail = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &avail) || avail == 0) break;
                std::vector<char> buf(avail);
                DWORD got = 0;
                if (WinHttpReadData(hRequest, buf.data(), avail, &got))
                    r.body.append(buf.data(), got);
            } while (avail > 0);
        }
    }
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return r;
}

// ── Error classification ──────────────────────────────────────
enum class Fail { None, Unreachable, KeyRejected, RateLimited, ServiceDown,
                  ModelUnavailable, Other };

// Google's own "message" from an error body (short; never contains the key).
std::string googleMessage(const std::string& body) {
    size_t p = body.find("\"message\"");
    if (p == std::string::npos) return "";
    p = body.find('"', body.find(':', p));
    if (p == std::string::npos) return "";
    std::string msg;
    for (size_t i = p + 1; i < body.size() && body[i] != '"' && msg.size() < 220; ++i) {
        if (body[i] == '\\' && i + 1 < body.size()) { ++i; msg += (body[i] == 'n' ? ' ' : body[i]); }
        else msg += body[i];
    }
    return msg;
}

bool containsCI(std::string hay, std::string needle) {
    std::transform(hay.begin(), hay.end(), hay.begin(), ::tolower);
    std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);
    return hay.find(needle) != std::string::npos;
}

Fail classify(const HttpReply& r) {
    if (!r.answered) return Fail::Unreachable;
    if (r.status == 200) return Fail::None;
    const std::string& b = r.body;
    if (r.status == 429) return Fail::RateLimited;            // RESOURCE_EXHAUSTED
    if (r.status >= 500) return Fail::ServiceDown;            // incl. 503 overloaded
    if (containsCI(b, "API_KEY_INVALID") || containsCI(b, "API key not valid") ||
        containsCI(b, "API_KEY_EXPIRED") || r.status == 401)
        return Fail::KeyRejected;
    if (r.status == 404) return Fail::ModelUnavailable;       // "models/x is not found ..."
    // 400/403 that are about the model, not the key (e.g. "not supported
    // for generateContent", "no longer available to new users").
    if ((r.status == 400 || r.status == 403) && containsCI(b, "model") &&
        (containsCI(b, "not found") || containsCI(b, "not supported") ||
         containsCI(b, "not available") || containsCI(b, "no longer available")))
        return Fail::ModelUnavailable;
    if (r.status == 403) return Fail::KeyRejected;            // API disabled / key restricted
    return Fail::Other;
}

std::string failMessage(Fail f, const HttpReply& r, const std::string& model = "") {
    std::string code = " (HTTP " + std::to_string(r.status) + ")";
    std::string g = googleMessage(r.body);
    switch (f) {
        case Fail::Unreachable:
            return "Gemini could not be reached. Check the internet connection.";
        case Fail::KeyRejected:
            return "Google rejected this API key" + code + ". Check that it was copied "
                   "completely and that its project allows the Gemini API.";
        case Fail::RateLimited:
            return "Gemini quota or rate limit reached" + code + ". Try again later, or "
                   "check the key's limits in Google AI Studio.";
        case Fail::ServiceDown:
            return "Gemini is temporarily unavailable" + code + ". Try again later.";
        case Fail::ModelUnavailable:
            return "Gemini model " + model + " is not available to this API key" + code +
                   (g.empty() ? "." : (": " + g));
        default:
            return "Unexpected reply from Gemini" + code + (g.empty() ? "." : (": " + g));
    }
}

const char* NO_MODEL_MSG =
    "The API key is valid, but no compatible Gemini model is currently "
    "available to it (CALSS needs a stable Gemini Flash-Lite or Flash model).";

// ── MODEL SELECTION ───────────────────────────────────────────
// Google's metadata says which models support generateContent, but not
// which are cheap, so cost is encoded in this small, explicit policy on
// the model ID (the API identifier, never the display name):
//
//   accepted IDs   gemini-<major>[.<minor>]-flash-lite[-NNN]   rank group 0
//                  gemini-<major>[.<minor>]-flash[-NNN]         rank group 1
//                  gemini-flash-lite-latest                     rank group 2
//                  gemini-flash-latest                          rank group 3
//   within a group: newer version first.
//
// The anchored patterns reject everything else on purpose: Pro (cost),
// -preview / -exp / dated previews (can be withdrawn at short notice),
// -tts / -image / -live / -transcribe / embeddings / Gemma / agents
// (wrong kind of model). "-latest" aliases are a last resort because
// Google hot-swaps them. Also required: "generateContent" in
// supportedGenerationMethods and, when given, inputTokenLimit >= 32768
// (a 50-pair prompt is far below that). AI Settings' manual choice only
// offers models that passed this filter, so previews/Pro are never used.
struct Candidate {
    GeminiModelInfo info;
    int group = 9, major = 0, minor = 0, suffix = 0;
};

bool rankModel(const std::string& id, Candidate& c) {
    static const std::regex stable(R"(^gemini-(\d+)(?:\.(\d+))?-(flash-lite|flash)(?:-(\d{3}))?$)");
    static const std::regex latest(R"(^gemini-(flash-lite|flash)-latest$)");
    std::smatch m;
    if (std::regex_match(id, m, stable)) {
        c.group  = (m[3] == "flash-lite") ? 0 : 1;
        c.major  = std::stoi(m[1]);
        c.minor  = m[2].matched ? std::stoi(m[2]) : 0;
        c.suffix = m[4].matched ? std::stoi(m[4]) : 0;
        return true;
    }
    if (std::regex_match(id, m, latest)) {
        c.group = (m[1] == "flash-lite") ? 2 : 3;
        return true;
    }
    return false;
}

std::string jsonStringField(const std::string& obj, const char* key) {
    std::string k = std::string("\"") + key + "\"";
    size_t p = obj.find(k);
    if (p == std::string::npos) return "";
    p = obj.find(':', p + k.size());
    if (p == std::string::npos) return "";
    p = obj.find('"', p);
    if (p == std::string::npos) return "";
    std::string v;
    for (size_t i = p + 1; i < obj.size() && obj[i] != '"'; ++i) {
        if (obj[i] == '\\' && i + 1 < obj.size()) {
            ++i;
            if (obj[i] == 'u') { i += 4; continue; }   // not needed for IDs/names
            v += (obj[i] == 'n') ? ' ' : obj[i];
        } else v += obj[i];
    }
    return v;
}

long jsonNumberField(const std::string& obj, const char* key) {
    std::string k = std::string("\"") + key + "\"";
    size_t p = obj.find(k);
    if (p == std::string::npos) return -1;
    p = obj.find(':', p + k.size());
    if (p == std::string::npos) return -1;
    return std::strtol(obj.c_str() + p + 1, nullptr, 10);
}

// Splits the "models" array of a models.list reply into its top-level
// objects (brace matching that skips over strings). False if malformed.
bool splitModelObjects(const std::string& json, std::vector<std::string>& out) {
    size_t p = json.find("\"models\"");
    if (p == std::string::npos) return json.find('{') != std::string::npos; // no models at all
    p = json.find('[', p);
    if (p == std::string::npos) return false;
    int depth = 0; bool inStr = false; size_t start = 0;
    for (size_t i = p + 1; i < json.size(); ++i) {
        char ch = json[i];
        if (inStr) { if (ch == '\\') ++i; else if (ch == '"') inStr = false; continue; }
        if (ch == '"') inStr = true;
        else if (ch == '{') { if (depth++ == 0) start = i; }
        else if (ch == '}') { if (--depth == 0) out.push_back(json.substr(start, i - start + 1)); }
        else if (ch == ']' && depth == 0) return true;
    }
    return false;
}

// models.list, all pages. On failure returns the classified error.
Fail listCompatibleModels(const std::string& apiKey, std::vector<Candidate>& out,
                          HttpReply& lastReply)
{
    out.clear();
    std::string pageToken;
    for (int page = 0; page < 5; ++page) {
        std::wstring path = L"/v1beta/models?pageSize=1000";
        if (!pageToken.empty()) path += L"&pageToken=" + toWide(pageToken);
        lastReply = geminiRequest(L"GET", path, apiKey, nullptr, 10000, 10000, 10000, 15000);
        Fail f = classify(lastReply);
        if (f != Fail::None) return f;
        std::vector<std::string> objs;
        if (!splitModelObjects(lastReply.body, objs)) return Fail::Other;
        for (const auto& o : objs) {
            std::string name = jsonStringField(o, "name");          // "models/<id>"
            if (name.rfind("models/", 0) != 0) continue;
            size_t methods = o.find("\"supportedGenerationMethods\"");
            if (methods == std::string::npos) continue;
            size_t close = o.find(']', methods);
            if (o.substr(methods, close - methods).find("\"generateContent\"") == std::string::npos)
                continue;
            long inLimit = jsonNumberField(o, "inputTokenLimit");
            if (inLimit >= 0 && inLimit < 32768) continue;
            Candidate c;
            c.info.id = name.substr(7);
            if (!rankModel(c.info.id, c)) continue;
            c.info.displayName = jsonStringField(o, "displayName");
            if (c.info.displayName.empty()) c.info.displayName = c.info.id;
            out.push_back(c);
        }
        pageToken = jsonStringField(lastReply.body, "nextPageToken");
        if (pageToken.empty()) break;
    }
    std::sort(out.begin(), out.end(), [](const Candidate& a, const Candidate& b) {
        if (a.group != b.group) return a.group < b.group;
        if (a.major != b.major) return a.major > b.major;
        if (a.minor != b.minor) return a.minor > b.minor;
        if (a.suffix != b.suffix) return a.suffix < b.suffix;   // plain alias before -001
        return a.info.id < b.info.id;
    });
    return Fail::None;
}

// ── Session cache (per key) and preference ────────────────────
std::mutex  g_modelMutex;
size_t      g_cacheKeyHash = 0;
bool        g_cacheValid = false;
std::vector<GeminiModelInfo> g_cacheModels;   // best first
std::set<std::string>        g_failedModels;  // said "unavailable" this session
std::string g_lastModelUsed;
bool        g_prefLoaded = false;
std::string g_modelPref;                      // "" = Automatic

size_t keyHash(const std::string& k) { return std::hash<std::string>{}(k); }

std::string prefPath() { return appDataDirU8() + "\\gemini_model.txt"; }

void storeCache(const std::string& apiKey, const std::vector<Candidate>& cands) {
    std::lock_guard<std::mutex> lk(g_modelMutex);
    size_t h = keyHash(apiKey);
    if (h != g_cacheKeyHash) g_failedModels.clear();
    g_cacheKeyHash = h;
    g_cacheModels.clear();
    for (const auto& c : cands) g_cacheModels.push_back(c.info);
    g_cacheValid = true;
}

std::string loadPrefLocked() {
    if (!g_prefLoaded) {
        std::ifstream f(pathFromUtf8(prefPath()));
        std::string s;
        if (f.is_open()) std::getline(f, s);
        while (!s.empty() && (s.back() == '\r' || s.back() == ' ')) s.pop_back();
        g_modelPref = (s == "auto") ? "" : s;
        g_prefLoaded = true;
    }
    return g_modelPref;
}

// Picks from the cache: the preferred model if it is listed and hasn't
// failed, else the best listed one that hasn't failed. Caller holds lock.
// `skip` = models already tried in this call (see callGeminiAPI).
std::string pickLocked(const std::set<std::string>& skip) {
    std::string pref = loadPrefLocked();
    auto usable = [&](const std::string& id) {
        return !g_failedModels.count(id) && !skip.count(id);
    };
    if (!pref.empty() && usable(pref))
        for (const auto& m : g_cacheModels)
            if (m.id == pref) return pref;
    for (const auto& m : g_cacheModels)
        if (usable(m.id)) return m.id;
    return "";
}

// Model to use for this key: cached pick, or a fresh models.list.
// forceRefresh lists again even if cached (after "model unavailable").
std::string resolveModel(const std::string& apiKey, bool forceRefresh,
                         const std::set<std::string>& skip, std::string& error) {
    {
        std::lock_guard<std::mutex> lk(g_modelMutex);
        if (!forceRefresh && g_cacheValid && g_cacheKeyHash == keyHash(apiKey)) {
            std::string m = pickLocked(skip);
            if (m.empty()) error = NO_MODEL_MSG;
            return m;
        }
    }
    std::vector<Candidate> cands;
    HttpReply reply;
    Fail f = listCompatibleModels(apiKey, cands, reply);    // network, no lock held
    if (f != Fail::None) { error = failMessage(f, reply); return ""; }
    storeCache(apiKey, cands);
    std::lock_guard<std::mutex> lk(g_modelMutex);
    std::string m = pickLocked(skip);
    if (m.empty()) error = NO_MODEL_MSG;
    return m;
}

} // namespace

GeminiKeyCheck gemini_checkKey(const std::string& apiKey) {
    GeminiKeyCheck out;
    if (apiKey.empty()) { out.error = "No key entered."; return out; }
    std::vector<Candidate> cands;
    HttpReply reply;
    Fail f = listCompatibleModels(apiKey, cands, reply);
    out.reachable = reply.answered;
    if (f != Fail::None) {
        out.rejected = (f == Fail::KeyRejected);
        out.error = failMessage(f, reply);
        return out;
    }
    out.keyValid = true;
    for (const auto& c : cands) out.models.push_back(c.info);
    storeCache(apiKey, cands);
    {
        std::lock_guard<std::mutex> lk(g_modelMutex);
        g_failedModels.clear();          // fresh check = fresh start
        // Automatic's choice, ignoring any manual preference.
        for (const auto& m : g_cacheModels) { out.autoModel = m.id; break; }
    }
    if (out.models.empty()) out.error = NO_MODEL_MSG;
    return out;
}

std::string gemini_getModelPreference() {
    std::lock_guard<std::mutex> lk(g_modelMutex);
    return loadPrefLocked();
}

bool gemini_setModelPreference(const std::string& modelId) {
    std::ofstream f(pathFromUtf8(prefPath()), std::ios::trunc);
    if (!f.is_open()) return false;
    f << (modelId.empty() ? "auto" : modelId) << "\n";
    f.close();
    std::lock_guard<std::mutex> lk(g_modelMutex);
    g_modelPref = modelId;
    g_prefLoaded = true;
    g_failedModels.clear();
    return true;
}

std::string gemini_lastModelUsed() {
    std::lock_guard<std::mutex> lk(g_modelMutex);
    return g_lastModelUsed;
}

// generateContent on the selected model, with the one-time fallback
// described at the top of this section. Returns the raw JSON reply.
static std::string callGeminiAPI(const std::string& apiKey,
                                   const std::string& prompt,
                                   std::string& error)
{
    error.clear();
    std::ostringstream body;
    body << "{\"contents\":[{\"parts\":[{\"text\":\""
         << jsonEscape(prompt) << "\"}]}]}";
    std::string bodyStr = body.str();

    bool refresh = false;
    std::set<std::string> tried;
    std::string firstError;
    for (int attempt = 0; attempt < 2; ++attempt) {
        std::string model = resolveModel(apiKey, refresh, tried, error);
        if (model.empty()) {
            if (!firstError.empty())
                error = firstError + " No other compatible Gemini model is available to this key.";
            return "";
        }
        tried.insert(model);
        // Explicit timeouts (ms): resolve, connect, send, receive. A full
        // batch of pair narratives can take well over WinHTTP's 30 s default.
        HttpReply r = geminiRequest(L"POST",
            L"/v1beta/models/" + toWide(model) + L":generateContent",
            apiKey, &bodyStr, 15000, 15000, 30000, 120000);
        Fail f = classify(r);
        if (f == Fail::None) {
            error.clear();
            std::lock_guard<std::mutex> lk(g_modelMutex);
            g_lastModelUsed = model;
            return r.body;
        }
        error = failMessage(f, r, model);
        if (attempt == 1) return "";
        if (f == Fail::ModelUnavailable) {
            // Gone for this key: never pick it again this session, list
            // again, and try the next candidate once.
            std::lock_guard<std::mutex> lk(g_modelMutex);
            g_failedModels.insert(model);
            refresh = true;
        } else if (!(f == Fail::ServiceDown && r.status == 503)) {
            return "";   // key, quota, network, other: retrying won't help
        }
        // 503 "model overloaded" (seen on the capacity-limited 2.5 models):
        // nothing was generated, so try the next candidate once, without
        // ruling this model out for later runs.
        firstError = error;
    }
    return "";
}
#endif

// ═════════════════════════════════════════════════════════════
// TIERED AI OUTPUTS (restructure)
//
// Uses loadApiKey(), jsonEscape(), toWide(), callGeminiAPI() and
// extractTextFromJson() above.
// ═════════════════════════════════════════════════════════════

// ── Evidence formatting ─────────────────────────────────────────

// One student's deviating features, formatted for the prompt. Only
// mismatches are listed in detail (the analytically interesting part);
// match count is given as context so the model can judge how isolated
// a deviation is against the rest of that student's profile. Every value
// here comes directly from StyleNoteDisplay fields already populated by
// parseStyleNote() in analysis_pipeline.cpp -- nothing is computed or inferred here.
static std::string formatStudentEvidence(const AuthorshipDisplay& d) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3);

    out << d.studentName << ": " << d.scorePct << "% authorship match ("
        << d.label << "), reliability " << d.reliabilityLabel
        << " (" << d.reliabilityNote << "), profile built from "
        << d.profileSize << " prior submission(s), "
        << d.matchedCount << "/" << d.totalFeatures << " features matched.\n";

    auto listDeviations = [&](const std::vector<StyleNoteDisplay>& features) {
        for (const auto& f : features) {
            if (f.isMatch) continue;
            out << "  [" << f.category << "] " << f.feature
                << " -- profile:" << f.profileValue
                << " | submission:" << f.submissionValue << "\n";
        }
    };
    listDeviations(d.lexicalFeatures);
    listDeviations(d.layoutFeatures);
    listDeviations(d.syntacticFeatures);

    return out.str();
}

// ── Tier 1: per-pair evidence narration ─────────────────────────

// Batched into a single call returning a JSON array of plain strings, one
// per pair, in the same order as `flagged`. See the rate-limit note at the
// bottom of this file for why batching (rather than one call per pair) is
// the default here.
static std::string buildPairNarrativePrompt(
    const std::vector<PairAnalysisDisplay>& flagged)
{
    std::ostringstream prompt;

    prompt << "You are an academic integrity analyst. Below is per-pair "
              "evidence data from a code authorship scoring system. For "
              "EACH pair, write a short evidence narrative (2-3 sentences) "
              "explaining which features deviated and why that combination "
              "does or does not matter.\n\n";

    prompt << "RULES:\n";
    prompt << "- Base every sentence ONLY on the numeric/categorical data "
              "given for that pair. Do not invent context, causes, or "
              "details not present in the data.\n";
    prompt << "- Never use the words 'cheated' or 'plagiarized'. Use "
              "language like 'flagged', 'worth reviewing', or 'deviates "
              "from profile' instead.\n";
    prompt << "- Weight Layout-category deviations (comment density, "
              "spacing, formatting) as weaker signal on their own; weight "
              "a combination of Lexical and Syntactic deviations as "
              "stronger signal.\n";
    prompt << "- If a student's reliability is Low or Moderate, explicitly "
              "say the score carries reduced statistical weight because of "
              "the small profile size -- do not present it with the same "
              "confidence as a High-reliability score.\n";
    prompt << "- Plain professional language. No headers, no markdown, no "
              "bullet points -- prose only, per pair.\n\n";

    prompt << "RESPONSE FORMAT (critical):\n";
    prompt << "Return ONLY a raw JSON array of exactly " << flagged.size()
           << " strings, one narrative per pair, in the exact order the "
              "pairs are given below. No markdown code fences, no keys, "
              "no extra text before or after the array. Example shape: "
              "[\"narrative for pair 1\", \"narrative for pair 2\"]\n\n";

    prompt << "PAIR DATA:\n";
    for (size_t i = 0; i < flagged.size(); ++i) {
        const auto& p = flagged[i];
        prompt << "--- PAIR " << (i + 1) << " ---\n";
        prompt << "Combined similarity: " << p.combinedScore << "% ("
               << p.similarityLabel << ")\n";
        prompt << formatStudentEvidence(p.studentA);
        prompt << formatStudentEvidence(p.studentB);
        prompt << "\n";
    }

    prompt << "Return the JSON array now:";
    return prompt.str();
}

// ── Tier 2: cross-pair pattern detection ────────────────────────

static std::string buildCrossPairPrompt(
    const std::vector<PairAnalysisDisplay>& flagged)
{
    std::ostringstream prompt;

    prompt << "You are an academic integrity analyst reviewing multiple "
              "flagged code-similarity pairs from one assignment. Below, "
              "for each pair, is which features deviated for each student "
              "(not full text -- just which features and their category). "
              "Identify whether multiple pairs share the same deviating "
              "feature cluster (a possible common external source) versus "
              "pairs that look like independent, unrelated incidents.\n\n";

    prompt << "RULES:\n";
    prompt << "- Base this ONLY on the deviation data given below. Do not "
              "invent a shared cause -- if nothing overlaps, say so plainly "
              "as the single finding.\n";
    prompt << "- Never use the words 'cheated' or 'plagiarized'.\n";
    prompt << "- Whenever you mention a specific pair, cite it as 'Pair N' "
              "using the number from its '--- PAIR N' header below, e.g. "
              "'Pair 3 (CastroBryan vs DizonCzarina)'. For several pairs "
              "write 'Pairs 1, 3 and 12'. Never refer to a pair by student "
              "names alone -- the 'Pair N' number is what the instructor "
              "clicks to open that pair.\n";
    prompt << "- Cite at most 5 pairs per finding. When a pattern spans more "
              "pairs than that, give the total and name the lowest-numbered "
              "(highest-scoring) examples, e.g. 'shared by 112 pairs, "
              "including Pairs 1, 8 and 10'. Never list every pair.\n";
    prompt << "- Each finding must be self-contained (2-3 sentences max) "
              "and readable on its own, since findings are shown as "
              "separate items, not one continuous paragraph.\n";
    prompt << "- Plain professional language. No markdown headers, no "
              "bullet characters inside a finding -- the array itself "
              "provides the separation.\n\n";

    prompt << "RESPONSE FORMAT (critical):\n";
    prompt << "Return ONLY a raw JSON array of 2 to 4 strings, one per "
              "distinct finding, ordered with the most notable pattern "
              "first. If there is truly nothing worth separating into "
              "multiple findings, return a single-element array. No "
              "markdown code fences, no keys, no extra text before or "
              "after the array. Example shape: [\"finding one\", "
              "\"finding two\"]\n\n";

    prompt << "FLAGGED PAIRS (" << flagged.size() << " total):\n";
    for (size_t i = 0; i < flagged.size(); ++i) {
        const auto& p = flagged[i];
        prompt << "--- PAIR " << (i + 1) << ": " << p.studentA.studentName
               << " vs " << p.studentB.studentName << " ("
               << p.combinedScore << "%) ---\n";

        auto listFeatureNames = [&](const std::string& who,
                                     const AuthorshipDisplay& d) {
            std::vector<std::string> devs;
            auto collect = [&](const std::vector<StyleNoteDisplay>& fs) {
                for (const auto& f : fs)
                    if (!f.isMatch) devs.push_back("[" + f.category + "] " + f.feature);
            };
            collect(d.lexicalFeatures);
            collect(d.layoutFeatures);
            collect(d.syntacticFeatures);

            prompt << who << " (" << d.studentName << ") deviating features: ";
            if (devs.empty()) {
                prompt << "none";
            } else {
                for (size_t j = 0; j < devs.size(); ++j) {
                    if (j) prompt << ", ";
                    prompt << devs[j];
                }
            }
            prompt << "\n";
        };
        listFeatureNames("Student A", p.studentA);
        listFeatureNames("Student B", p.studentB);
        prompt << "\n";
    }

    prompt << "Return the JSON array of findings now:";
    return prompt.str();
}

// ── JSON array-of-strings parsing ───────────────────────────────

// Parses a single JSON string literal starting at json[pos] (which must
// be '"'). Advances pos to just past the closing quote. Mirrors the
// escape handling in extractTextFromJson() above.
static std::string parseJsonStringLiteral(const std::string& json, size_t& pos) {
    std::string result;
    size_t i = pos + 1; // skip opening quote
    while (i < json.size()) {
        char c = json[i];
        if (c == '\\' && i + 1 < json.size()) {
            char next = json[i + 1];
            switch (next) {
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'u':
                    if (i + 5 < json.size()) i += 4;
                    break;
                default: result += next;
            }
            i += 2;
        } else if (c == '"') {
            ++i; // consume closing quote
            break;
        } else {
            result += c;
            ++i;
        }
    }
    pos = i;
    return result;
}

// Extracts a top-level JSON array of strings from `raw`, tolerating
// markdown code fences and leading/trailing text the model may add despite
// being told not to. Returns false (leaving `out` unchanged) if no '['..']'
// array of quoted strings can be found.
static bool extractJsonStringArray(const std::string& raw,
                                    std::vector<std::string>& out)
{
    size_t start = raw.find('[');
    if (start == std::string::npos) return false;

    std::vector<std::string> result;
    size_t i = start + 1;
    while (i < raw.size()) {
        // Skip whitespace and commas
        while (i < raw.size() &&
               (raw[i] == ' ' || raw[i] == '\n' || raw[i] == '\r' ||
                raw[i] == '\t' || raw[i] == ','))
            ++i;
        if (i >= raw.size()) return false;
        if (raw[i] == ']') { ++i; break; }
        if (raw[i] != '"') return false; // malformed -- bail rather than guess
        result.push_back(parseJsonStringLiteral(raw, i));
    }

    if (result.empty()) return false;
    out = std::move(result);
    return true;
}

// ── Public entry points ─────────────────────────────────────────

std::vector<AIPairNarrative> generatePairNarratives(
    const std::vector<PairAnalysisDisplay>& flagged)
{
    std::vector<AIPairNarrative> results(flagged.size());
    for (size_t i = 0; i < flagged.size(); ++i) {
        results[i].pairId = flagged[i].studentA.studentName + " vs " +
                             flagged[i].studentB.studentName;
        results[i].success = false;
        results[i].skipped = false;
    }

    auto setAll = [&](bool skipped, bool success, const std::string& err) {
        for (auto& r : results) {
            r.skipped = skipped;
            r.success = success;
            r.error = err;
        }
    };

    std::string apiKey = loadApiKey();
    if (apiKey.empty()) {
        setAll(true, false, "No API key configured. AI narratives skipped.");
        return results;
    }
    if (flagged.empty()) {
        return results; // nothing to narrate; empty vector, nothing to set
    }

    std::string prompt = buildPairNarrativePrompt(flagged);

#ifdef _WIN32
    std::string error;
    std::string response = callGeminiAPI(apiKey, prompt, error);
    if (response.empty()) {
        setAll(false, false, error);
        return results;
    }

    std::string text = extractTextFromJson(response);
    if (text.empty()) {
        setAll(false, false, "Could not parse API response");
        return results;
    }

    std::vector<std::string> narratives;
    if (!extractJsonStringArray(text, narratives) ||
        narratives.size() != flagged.size()) {
        setAll(false, false,
               "AI response did not match expected pair count "
               "(got " + std::to_string(narratives.size()) + ", expected " +
               std::to_string(flagged.size()) + ")");
        return results;
    }

    for (size_t i = 0; i < results.size(); ++i) {
        results[i].narrative = narratives[i];
        results[i].success = true;
    }
    return results;
#else
    setAll(false, false, "AI summary only supported on Windows");
    return results;
#endif
}

AIBatchPatterns generateCrossPairPatterns(
    const std::vector<PairAnalysisDisplay>& flagged)
{
    AIBatchPatterns result;
    result.success = false;
    result.skipped = false;

    if (flagged.size() < 2) {
        result.skipped = true;
        result.error = "Fewer than 2 flagged pairs -- no cross-pair pattern to detect.";
        return result;
    }

    std::string apiKey = loadApiKey();
    if (apiKey.empty()) {
        result.skipped = true;
        result.error = "No API key configured. Pattern analysis skipped.";
        return result;
    }

    std::string prompt = buildCrossPairPrompt(flagged);

#ifdef _WIN32
    std::string error;
    std::string response = callGeminiAPI(apiKey, prompt, error);
    if (response.empty()) {
        result.success = false;
        result.error = error;
        return result;
    }

    std::string text = extractTextFromJson(response);
    if (text.empty()) {
        result.success = false;
        result.error = "Could not parse API response";
        return result;
    }

    std::vector<std::string> findings;
    if (!extractJsonStringArray(text, findings)) {
        result.success = false;
        result.error = "AI response was not a valid findings array";
        return result;
    }

    result.findings = std::move(findings);
    result.success = true;
    result.model = gemini_lastModelUsed();
    return result;
#else
    result.success = false;
    result.error = "AI summary only supported on Windows";
    return result;
#endif
}