// ─────────────────────────────────────────────────────────────
// gemini.cpp
// Gemini API integration — calls API and parses response
// Uses WinHTTP (Windows native) for HTTPS — no external libs needed
// ─────────────────────────────────────────────────────────────

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
    std::ifstream f("api_key.txt");
    if (!f.is_open()) return "";

    std::string key;
    std::getline(f, key);

    // Trim whitespace
    while (!key.empty() && (key.back() == '\n' || key.back() == '\r' ||
                             key.back() == ' '  || key.back() == '\t'))
        key.pop_back();
    while (!key.empty() && (key.front() == ' ' || key.front() == '\t'))
        key.erase(key.begin());

    // Check if it's still the placeholder
    if (key == "PASTE_YOUR_GEMINI_API_KEY_HERE") return "";

    return key;
}

// ═════════════════════════════════════════════════════════════
// PROMPT BUILDING
// ═════════════════════════════════════════════════════════════

std::string buildPrompt(const std::vector<PairResult>& flagged,
                        AuthorshipScorer& scorer)
{
    std::ostringstream prompt;

    prompt << "You are an academic integrity analyst reviewing code authorship "
              "verification results for a programming instructor. Below is detailed "
              "analysis data from a code authorship scoring system that compares "
              "student exam submissions against their established style profiles.\n\n";

    prompt << "YOUR TASK:\n";
    prompt << "Write a structured instructor briefing with these sections:\n\n";
    prompt << "1. PRIORITY CASES - Which pairs need immediate attention and why. "
              "Rank them by concern level.\n";
    prompt << "2. PATTERN ANALYSIS - Are multiple flagged pairs connected? Do any "
              "students appear across multiple flags? Are there clusters suggesting "
              "a shared external source?\n";
    prompt << "3. STYLE INCONSISTENCY DETAILS - For the most concerning cases, "
              "explain specifically which style dimensions (naming, comments, "
              "formatting, structure) deviate from the student's profile and what "
              "that might indicate.\n";
    prompt << "4. RECOMMENDED ACTIONS - Concrete next steps the instructor should "
              "take (e.g., compare specific submissions side-by-side, interview "
              "specific students, check supervised exam performance).\n\n";

    prompt << "RULES:\n";
    prompt << "- DO NOT name any student as the 'copier' or 'cheater'\n";
    prompt << "- DO NOT make definitive accusations\n";
    prompt << "- DO use student names when describing findings\n";
    prompt << "- DO be specific about which features mismatch and what values differ\n";
    prompt << "- DO identify the student in each pair whose submission shows LOWER "
              "style consistency (they are the one worth investigating)\n";
    prompt << "- DO end by reminding that all determinations rest with the instructor\n";
    prompt << "- Use plain professional language\n";
    prompt << "- Keep it under 400 words total\n\n";

    prompt << "ANALYSIS DATA:\n";
    prompt << "Total flagged pairs: " << flagged.size() << "\n\n";

    for (size_t i = 0; i < flagged.size(); ++i) {
        const auto& pair = flagged[i];
        prompt << "--- PAIR " << (i + 1) << " ---\n";
        prompt << "Files: " << pair.studentA << " vs " << pair.studentB << "\n";
        prompt << "Combined similarity: " << pair.combinedScore << "% ("
               << pair.label << ")\n";

        // Get authorship results with full detail
        AuthorshipResult rA = scorer.score(pair.studentA, pair.pathA);
        AuthorshipResult rB = scorer.score(pair.studentB, pair.pathB);

        if (rA.success) {
            int matches = 0, total = (int)rA.styleMatch.size();
            for (const auto& note : rA.styleMatch)
                if (note.find("[MATCH]") != std::string::npos &&
                    note.find("[MISMATCH]") == std::string::npos) ++matches;
            prompt << pair.studentA << ": " << rA.scorePct << "% authorship ("
                   << rA.label << "), profile from " << rA.profileSize
                   << " submissions, " << matches << "/"
                   << total << " features matched\n";
            // Send individual feature details
            for (const auto& note : rA.styleMatch) {
                prompt << "  " << note << "\n";
            }
        }

        if (rB.success) {
            int matches = 0, total = (int)rB.styleMatch.size();
            for (const auto& note : rB.styleMatch)
                if (note.find("[MATCH]") != std::string::npos &&
                    note.find("[MISMATCH]") == std::string::npos) ++matches;
            prompt << pair.studentB << ": " << rB.scorePct << "% authorship ("
                   << rB.label << "), profile from " << rB.profileSize
                   << " submissions, " << matches << "/"
                   << total << " features matched\n";
            for (const auto& note : rB.styleMatch) {
                prompt << "  " << note << "\n";
            }
        }
        prompt << "\n";
    }

    prompt << "Generate the instructor briefing now:";

    return prompt.str();
}

// ═════════════════════════════════════════════════════════════
// API CALL
// ═════════════════════════════════════════════════════════════

#ifdef _WIN32
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

    // Build request path with API key
    std::wstring path =
        L"/v1beta/models/gemini-2.5-flash-lite:generateContent?key=" +
        toWide(apiKey);

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

    // Set Content-Type header
    LPCWSTR headers = L"Content-Type: application/json\r\n";

    // Send request
    BOOL ok = WinHttpSendRequest(
        hRequest, headers, -1,
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
// MAIN ENTRY
// ═════════════════════════════════════════════════════════════

AISummary generateAISummary(const std::vector<PairResult>& flagged,
                             AuthorshipScorer& scorer)
{
    AISummary result;
    result.success = false;
    result.skipped = false;

    // Load API key
    std::string apiKey = loadApiKey();
    if (apiKey.empty()) {
        result.skipped = true;
        result.error = "No API key configured. AI summary skipped.";
        return result;
    }

    if (flagged.empty()) {
        result.skipped = true;
        result.error = "No flagged pairs to summarize.";
        return result;
    }

    // Build prompt
    std::string prompt = buildPrompt(flagged, scorer);

#ifdef _WIN32
    // Call API
    std::string error;
    std::string response = callGeminiAPI(apiKey, prompt, error);

    if (response.empty()) {
        result.success = false;
        result.error = error;
        return result;
    }

    // Parse response
    std::string text = extractTextFromJson(response);
    if (text.empty()) {
        result.success = false;
        result.error = "Could not parse API response";
        return result;
    }

    result.text = text;
    result.success = true;
    return result;
#else
    result.success = false;
    result.error = "AI summary only supported on Windows";
    return result;
#endif
}

// ═════════════════════════════════════════════════════════════
// TIERED AI OUTPUTS (restructure)
//
// Reuses loadApiKey(), jsonEscape(), toWide(), and callGeminiAPI() above
// unchanged. Everything below is additive; nothing above this point was
// modified beyond the two #include lines and the header signature list.
// ═════════════════════════════════════════════════════════════

// ── Evidence formatting ─────────────────────────────────────────

// One student's deviating features, formatted for the prompt. Only
// mismatches are listed in detail (the analytically interesting part);
// match count is given as context so the model can judge how isolated
// a deviation is against the rest of that student's profile. Every value
// here comes directly from StyleNoteDisplay fields already populated by
// parseStyleNote() in main.cpp -- nothing is computed or inferred here.
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
    prompt << "- Reference students and pairs by name/number when pointing "
              "out an overlap.\n";
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