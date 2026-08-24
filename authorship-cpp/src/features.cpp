// ─────────────────────────────────────────────────────────────
// features.cpp
// 14 coding style feature extraction from .c source files
// ─────────────────────────────────────────────────────────────

#include "../include/features.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <numeric>

// ── Constants defined here ────────────────────────────────────
const double FEATURE_MAX[FEATURE_COUNT] = {
    15.0,   // avg_var_name_length
    1.0,    // single_letter_ratio
    1.0,    // camel_case_ratio
    1.0,    // underscore_ratio
    1.0,    // comment_density
    1.0,    // inline_comment_ratio
    1.0,    // bracket_same_line
    1.0,    // for_loop_ratio
    1.0,    // spaces_around_operators
    80.0,   // avg_line_length
    1.0,    // blank_line_ratio
    100.0,  // avg_function_length
    1.0,    // return_at_end_ratio
    1.0,    // all_caps_var_ratio
};

const char* FEATURE_LABELS[FEATURE_COUNT] = {
    "Avg Variable Name Length",
    "Single-Letter Variable Ratio",
    "camelCase Naming Ratio",
    "snake_case Naming Ratio",
    "Comment Density",
    "Inline Comment Ratio",
    "Bracket Same Line",
    "For-Loop Preference",
    "Spaced Operators",
    "Avg Line Length",
    "Blank Line Ratio",
    "Avg Function Length",
    "Return At End Ratio",
    "ALL_CAPS Variable Ratio",
};

// ── C keywords to exclude from identifier analysis ────────────
static const std::unordered_set<std::string> C_KW = {
    "auto","break","case","char","const","continue","default","do",
    "double","else","enum","extern","float","for","goto","if","inline",
    "int","long","register","restrict","return","short","signed",
    "sizeof","static","struct","switch","typedef","union","unsigned",
    "void","volatile","while","printf","scanf","main","NULL","EOF",
    "malloc","free","strlen","strcpy","strcmp","fprintf","fopen","fclose"
};

// ═════════════════════════════════════════════════════════════
// INTERNAL HELPERS
// ═════════════════════════════════════════════════════════════

static bool readFile(const std::string& path, std::string& out) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

static double safeDivide(double num, double den) {
    return (den == 0.0) ? 0.0 : num / den;
}

static double safeMean(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    double sum = 0.0;
    for (double x : v) sum += x;
    return sum / v.size();
}

static int countStr(const std::string& text, const std::string& pat) {
    int count = 0;
    size_t pos = 0;
    while ((pos = text.find(pat, pos)) != std::string::npos) {
        ++count;
        pos += pat.size();
    }
    return count;
}

// camelCase: starts lowercase, has at least one uppercase later
static bool isCamelCase(const std::string& s) {
    if (s.size() < 2) return false;
    if (!islower((unsigned char)s[0])) return false;
    for (size_t i = 1; i < s.size(); ++i)
        if (isupper((unsigned char)s[i])) return true;
    return false;
}

// snake_case: has underscore with letters on both sides
static bool isSnakeCase(const std::string& s) {
    size_t pos = s.find('_');
    return pos != std::string::npos && pos > 0 && pos < s.size() - 1;
}

// ALL_CAPS: all alpha chars are uppercase
static bool isAllCaps(const std::string& s) {
    if (s.size() < 2) return false;
    bool hasLetter = false;
    for (char c : s) {
        if (isalpha((unsigned char)c)) {
            hasLetter = true;
            if (islower((unsigned char)c)) return false;
        }
    }
    return hasLetter;
}

// Extract identifiers (non-keyword words) from source
static std::vector<std::string> extractIdentifiers(const std::string& src) {
    std::vector<std::string> ids;
    size_t i = 0;
    while (i < src.size()) {
        if (isalpha((unsigned char)src[i]) || src[i] == '_') {
            std::string word;
            while (i < src.size() &&
                   (isalnum((unsigned char)src[i]) || src[i] == '_'))
                word += src[i++];
            if (!C_KW.count(word))
                ids.push_back(word);
        } else {
            ++i;
        }
    }
    return ids;
}

// Strip comments, count them
static std::string stripComments(const std::string& src,
                                  int& blockCount,
                                  int& lineCount,
                                  int& inlineCount) {
    std::string out;
    out.reserve(src.size());
    blockCount = lineCount = inlineCount = 0;
    size_t i = 0;

    while (i < src.size()) {
        // Block comment /* ... */
        if (i + 1 < src.size() && src[i] == '/' && src[i+1] == '*') {
            ++blockCount;
            i += 2;
            while (i + 1 < src.size() &&
                   !(src[i] == '*' && src[i+1] == '/')) {
                if (src[i] == '\n') out += '\n';
                ++i;
            }
            i += 2;
        }
        // Line comment //
        else if (i + 1 < src.size() && src[i] == '/' && src[i+1] == '/') {
            // Check if there's non-whitespace before // on this line
            size_t lstart = out.rfind('\n');
            std::string before = (lstart == std::string::npos)
                                  ? out : out.substr(lstart + 1);
            bool hasCode = false;
            for (char c : before)
                if (!isspace((unsigned char)c)) { hasCode = true; break; }
            if (hasCode) ++inlineCount;
            else         ++lineCount;
            // Skip to end of line
            while (i < src.size() && src[i] != '\n') ++i;
        }
        else {
            out += src[i++];
        }
    }
    return out;
}

// ═════════════════════════════════════════════════════════════
// PUBLIC FUNCTIONS
// ═════════════════════════════════════════════════════════════

FeatureResult extractFeatures(const std::string& filepath) {
    FeatureResult result;
    result.filepath = filepath;
    result.success  = false;
    result.raw.assign(FEATURE_COUNT, 0.0);

    std::string source;
    if (!readFile(filepath, source)) {
        result.error = "File not found: " + filepath;
        return result;
    }

    // Split into lines
    std::vector<std::string> lines;
    {
        std::istringstream ss(source);
        std::string ln;
        while (std::getline(ss, ln)) lines.push_back(ln);
    }
    int totalLines = (int)lines.size();
    if (totalLines == 0) { result.error = "Empty file"; return result; }

    // Strip comments
    int blockC, lineC, inlineC;
    std::string clean = stripComments(source, blockC, lineC, inlineC);
    int totalComments = blockC + lineC + inlineC;

    // ── [0-3] Identifier naming ───────────────────────────────
    std::vector<std::string> ids = extractIdentifiers(clean);
    // Deduplicate
    std::unordered_set<std::string> seen;
    std::vector<std::string> uniqueIds;
    for (auto& id : ids)
        if (seen.insert(id).second) uniqueIds.push_back(id);

    std::vector<double> nameLens;
    int singleC = 0, camelC = 0, underC = 0, capsC = 0;
    for (auto& id : uniqueIds) {
        nameLens.push_back((double)id.size());
        if (id.size() == 1)    ++singleC;
        if (isCamelCase(id))   ++camelC;
        if (isSnakeCase(id))   ++underC;
        if (isAllCaps(id))     ++capsC;
    }
    int uid = (int)uniqueIds.size();
    result.raw[0]  = safeMean(nameLens);
    result.raw[1]  = safeDivide(singleC, uid);
    result.raw[2]  = safeDivide(camelC,  uid);
    result.raw[3]  = safeDivide(underC,  uid);
    result.raw[13] = safeDivide(capsC,   uid);

    // ── [4-5] Comment style ───────────────────────────────────
    result.raw[4] = safeDivide(totalComments, totalLines);
    result.raw[5] = safeDivide(inlineC, std::max(totalComments, 1));

    // ── [6] Bracket placement ─────────────────────────────────
    int sameLine = 0, nextLine = 0;
    for (int li = 0; li < totalLines; ++li) {
        const std::string& ln = lines[li];
        size_t pPos = ln.rfind(')');
        size_t bPos = ln.find('{');
        if (pPos != std::string::npos && bPos != std::string::npos &&
            bPos > pPos) {
            ++sameLine;
        } else if (pPos != std::string::npos &&
                   bPos == std::string::npos &&
                   li + 1 < totalLines) {
            const std::string& next = lines[li+1];
            size_t s = next.find_first_not_of(" \t");
            if (s != std::string::npos && next[s] == '{') ++nextLine;
        }
    }
    int totalBr = sameLine + nextLine;
    result.raw[6] = (totalBr > 0) ? safeDivide(sameLine, totalBr) : 0.5;

    // ── [7] Loop preference ───────────────────────────────────
    int forC   = countStr(clean, "for(")  + countStr(clean, "for (");
    int whileC = countStr(clean, "while(")+ countStr(clean, "while (");
    int totalL = forC + whileC;
    result.raw[7] = (totalL > 0) ? safeDivide(forC, totalL) : 0.5;

    // ── [8] Operator spacing ──────────────────────────────────
    // Spaced: letter/digit SPACE op SPACE letter/digit
    // Compact: letter/digit op letter/digit
    int spacedC = 0, compactC = 0;
    for (size_t i = 1; i + 1 < clean.size(); ++i) {
        char prev = clean[i-1], cur = clean[i], next = clean[i+1];
        bool isOp = (cur == '=' || cur == '<' || cur == '>' ||
                     cur == '+' || cur == '-' || cur == '*' || cur == '/');
        if (!isOp) continue;
        bool prevAlNum = isalnum((unsigned char)prev);
        bool nextAlNum = isalnum((unsigned char)next);
        bool prevSpace = prev == ' ';
        bool nextSpace = next == ' ';
        if (prevAlNum && nextAlNum) ++compactC;
        else if (prevSpace && nextSpace) ++spacedC;
    }
    int totalOp = spacedC + compactC;
    result.raw[8] = (totalOp > 0) ? safeDivide(spacedC, totalOp) : 0.5;

    // ── [9-10] Line characteristics ───────────────────────────
    std::vector<double> lineLens;
    int blankLines = 0;
    for (const auto& ln : lines) {
        size_t s = ln.find_first_not_of(" \t");
        if (s == std::string::npos) { ++blankLines; continue; }
        if (ln[s] == '/' ) continue; // skip comment lines
        lineLens.push_back((double)ln.size());
    }
    result.raw[9]  = safeMean(lineLens);
    result.raw[10] = safeDivide(blankLines, totalLines);

    // ── [11-12] Function structure ────────────────────────────
    std::vector<int> funcLens;
    int depth = 0, funcStart = -1;
    bool lastWasReturn = false;
    int returnEndC = 0, funcCount = 0;

    for (int li = 0; li < totalLines; ++li) {
        const std::string& ln = lines[li];
        for (char c : ln) {
            if (c == '{') { if (depth == 0) funcStart = li; ++depth; }
            else if (c == '}') {
                --depth;
                if (depth == 0 && funcStart >= 0) {
                    funcLens.push_back(li - funcStart);
                    ++funcCount;
                    if (lastWasReturn) ++returnEndC;
                    funcStart = -1;
                }
            }
        }
        size_t st = ln.find_first_not_of(" \t");
        lastWasReturn = (st != std::string::npos &&
                         ln.substr(st, 6) == "return");
    }
    std::vector<double> fLenD(funcLens.begin(), funcLens.end());
    result.raw[11] = safeMean(fLenD);
    result.raw[12] = (funcCount > 0)
                     ? safeDivide(returnEndC, funcCount) : 0.5;

    // Normalize
    result.normalized = normalizeVector(result.raw);
    result.success = true;
    return result;
}

std::vector<double> normalizeVector(const std::vector<double>& raw) {
    std::vector<double> norm(FEATURE_COUNT, 0.0);
    for (int i = 0; i < FEATURE_COUNT; ++i) {
        if (FEATURE_MAX[i] != 0.0)
            norm[i] = std::min(1.0, raw[i] / FEATURE_MAX[i]);
    }
    return norm;
}

std::vector<double> averageVectors(
    const std::vector<std::vector<double>>& vecs)
{
    if (vecs.empty()) return {};
    std::vector<double> avg(FEATURE_COUNT, 0.0);
    for (const auto& v : vecs)
        for (int i = 0; i < FEATURE_COUNT && i < (int)v.size(); ++i)
            avg[i] += v[i];
    for (auto& val : avg) val /= (double)vecs.size();
    return avg;
}

double euclideanDistance(const std::vector<double>& a,
                         const std::vector<double>& b) {
    if (a.size() != b.size()) return 1.0;
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
        sum += (a[i] - b[i]) * (a[i] - b[i]);
    return sqrt(sum);
}

double styleSimilarity(const std::vector<double>& a,
                       const std::vector<double>& b) {
    double dist    = euclideanDistance(a, b);
    double maxDist = sqrt((double)FEATURE_COUNT);
    return std::max(0.0, 1.0 - (dist / maxDist));
}

std::vector<StyleNote> describeStyle(const std::vector<double>& vec) {
    std::vector<StyleNote> notes;
    if ((int)vec.size() < FEATURE_COUNT) return notes;

    double vl = vec[0];
    if      (vl <= 2) notes.push_back({"Naming", "Prefers very short variable names (1-2 chars)"});
    else if (vl >= 8) notes.push_back({"Naming", "Prefers long descriptive variable names"});
    else              notes.push_back({"Naming", "Uses medium-length variable names"});

    if (vec[1] >= 0.5)
        notes.push_back({"Naming", "Heavy use of single-letter variables"});

    if      (vec[2] >= 0.4) notes.push_back({"Style", "Naming: camelCase"});
    else if (vec[3] >= 0.4) notes.push_back({"Style", "Naming: snake_case"});
    else                    notes.push_back({"Style", "Naming: mixed or unstructured"});

    double cd = vec[4];
    if      (cd == 0)    notes.push_back({"Comments", "Never writes comments"});
    else if (cd <= 0.05) notes.push_back({"Comments", "Rarely writes comments"});
    else if (cd >= 0.20) notes.push_back({"Comments", "Frequently writes comments"});
    else                 notes.push_back({"Comments", "Occasionally writes comments"});

    if      (vec[6] >= 0.7) notes.push_back({"Brackets", "Same-line:  if (x) {"});
    else if (vec[6] <= 0.3) notes.push_back({"Brackets", "Next-line:  if (x) -> {"});

    if      (vec[7] >= 0.8) notes.push_back({"Loops", "Strong preference for for-loops"});
    else if (vec[7] <= 0.2) notes.push_back({"Loops", "Strong preference for while-loops"});
    else                    notes.push_back({"Loops", "Mixed loop usage"});

    if      (vec[8] >= 0.7) notes.push_back({"Spacing", "Spaced operators  (x = y)"});
    else if (vec[8] <= 0.3) notes.push_back({"Spacing", "Compact operators  (x=y)"});

    if      (vec[10] >= 0.2)  notes.push_back({"Layout", "Frequently adds blank lines"});
    else if (vec[10] <= 0.05) notes.push_back({"Layout", "Almost no blank lines"});

    return notes;
}

std::vector<std::string> compareStyles(
    const std::vector<double>& test_vec,
    const std::vector<double>& profile_vec)
{
    std::vector<std::string> notes;
    if ((int)test_vec.size() < FEATURE_COUNT ||
        (int)profile_vec.size() < FEATURE_COUNT) {
        notes.push_back("  Style comparison unavailable.");
        return notes;
    }

    struct Check { int idx; const char* label; double threshold; };
    Check checks[] = {
        // LEXICAL (5)
        {0,  "Variable name length",    1.5},
        {1,  "Single-letter variables", 0.15},
        {2,  "camelCase naming",        0.15},
        {3,  "snake_case naming",       0.15},
        {13, "ALL_CAPS variables",      0.15},
        // LAYOUT (5)
        {4,  "Comment frequency",       0.08},
        {5,  "Inline comment style",    0.20},
        {6,  "Bracket placement",       0.30},
        {9,  "Line length",             0.15},
        {10, "Blank line usage",        0.12},
        // SYNTACTIC (4)
        {7,  "Loop preference",         0.25},
        {8,  "Operator spacing",        0.25},
        {11, "Function length",         0.20},
        {12, "Return placement",        0.25},
    };
    int numChecks = (int)(sizeof(checks)/sizeof(checks[0]));

    int matched = 0, mismatched = 0;
    char buf[256];
    for (int ci = 0; ci < numChecks; ++ci) {
        const Check& c = checks[ci];
        double diff = fabs(test_vec[c.idx] - profile_vec[c.idx]);
        if (diff <= c.threshold) {
            ++matched;
            snprintf(buf, sizeof(buf),
                "  [MATCH]    %s  (profile:%.2f | submission:%.2f)",
                c.label, profile_vec[c.idx], test_vec[c.idx]);
        } else {
            ++mismatched;
            snprintf(buf, sizeof(buf),
                "  [MISMATCH] %s  (profile:%.2f | submission:%.2f)",
                c.label, profile_vec[c.idx], test_vec[c.idx]);
        }
        notes.push_back(std::string(buf));
    }
    snprintf(buf, sizeof(buf),
        "  %d matched / %d inconsistent with student profile.",
        matched, mismatched);
    notes.push_back(std::string(buf));
    return notes;
}