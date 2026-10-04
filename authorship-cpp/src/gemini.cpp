// ─────────────────────────────────────────────────────────────
// gemini.cpp
// Gemini API integration — calls API and parses response
// Uses WinHTTP (Windows native) for HTTPS — no external libs needed
// ─────────────────────────────────────────────────────────────

#include "../include/secret_store.h"
#include "../include/gemini.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>

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

#ifdef _WIN32
// "Test key" (AI settings dialog): one GET of the model's own metadata.
// It authenticates exactly like a real request but generates nothing,
// so it costs no generation quota. 200 = the key works.
bool gemini_testKey(const std::string& apiKey, std::string& error, bool* rejected) {
    error.clear();
    if (rejected) *rejected = false;
    if (apiKey.empty()) { error = "No key entered."; return false; }
    HINTERNET hSession = WinHttpOpen(L"AuthorshipScorer/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { error = "Could not open an HTTP session."; return false; }
    // Short timeouts (ms): this runs on the UI thread from the AI
    // settings dialog, and WinHTTP's defaults include an unlimited DNS
    // wait — a bad connection would freeze the dialog.
    WinHttpSetTimeouts(hSession, 10000, 10000, 10000, 15000);
    HINTERNET hConnect = WinHttpConnect(hSession, L"generativelanguage.googleapis.com",
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hRequest = hConnect ? WinHttpOpenRequest(hConnect, L"GET",
        L"/v1beta/models/gemini-2.5-flash-lite", nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
    bool ok = false;
    if (hRequest) {
        std::wstring headers = L"x-goog-api-key: " + toWide(apiKey) + L"\r\n";
        if (WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)-1L,
                               WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, nullptr)) {
            DWORD status = 0, sz = sizeof(status);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                nullptr, &status, &sz, nullptr);
            if (status == 200) ok = true;
            else if (status == 400 || status == 401 || status == 403) {
                if (rejected) *rejected = true;
                error = "Google rejected this key (HTTP " + std::to_string(status) +
                        "). Check that it was copied completely.";
            }
            else
                error = "Unexpected reply from Google (HTTP " + std::to_string(status) + ").";
        } else {
            error = "Could not reach Google. Check the internet connection.";
        }
    } else {
        error = "Could not reach Google. Check the internet connection.";
    }
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

static std::string callGeminiAPI(const std::string& apiKey,
                                   const std::string& prompt,
                                   std::string& error)
{
    error.clear();

    // Build request body
    std::ostringstream body;
    body << "{\"contents\":[{\"parts\":[{\"text\":\""
         << jsonEscape(prompt) << "\"}]}]}";
    std::string bodyStr = body.str();

    // Open WinHTTP session
    HINTERNET hSession = WinHttpOpen(
        L"AuthorshipScorer/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { error = "Failed to open HTTP session"; return ""; }

    // Explicit timeouts (ms): resolve, connect, send, receive. WinHTTP's
    // defaults are an unlimited DNS wait and a 30 s receive, which a
    // full batch of pair narratives can exceed while Gemini generates.
    WinHttpSetTimeouts(hSession, 15000, 15000, 30000, 120000);

    // Connect to host
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"generativelanguage.googleapis.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        error = "Failed to connect to API host";
        return "";
    }

    // Request path — the API key travels in the x-goog-api-key header
    // below, not in the URL (URLs end up in proxy/server logs).
    std::wstring path =
        L"/v1beta/models/gemini-2.5-flash-lite:generateContent";

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"POST", path.c_str(),
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        error = "Failed to create HTTP request";
        return "";
    }

    // Content-Type + API key headers
    std::wstring headers = L"Content-Type: application/json\r\n"
                           L"x-goog-api-key: " + toWide(apiKey) + L"\r\n";

    // Send request
    BOOL ok = WinHttpSendRequest(
        hRequest, headers.c_str(), (DWORD)-1L,
        (LPVOID)bodyStr.data(), (DWORD)bodyStr.size(),
        (DWORD)bodyStr.size(), 0);
    if (!ok) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        error = "Failed to send HTTP request (check internet connection)";
        return "";
    }

    // Wait for response
    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        error = "Failed to receive HTTP response";
        return "";
    }

    // Check status code
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        nullptr, &statusCode, &statusSize, nullptr);

    // Read response body
    std::string response;
    DWORD dwSize = 0;
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;

        std::vector<char> buffer(dwSize + 1);
        DWORD dwDownloaded = 0;
        if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
            response.append(buffer.data(), dwDownloaded);
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (statusCode != 200) {
        char buf[64];
        snprintf(buf, sizeof(buf), "API returned status %lu", statusCode);
        error = std::string(buf) + ": " +
                response.substr(0, std::min((size_t)200, response.size()));
        return "";
    }

    return response;
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
    return result;
#else
    result.success = false;
    result.error = "AI summary only supported on Windows";
    return result;
#endif
}