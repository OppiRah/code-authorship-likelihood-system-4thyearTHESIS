// ─────────────────────────────────────────────────────────────
// tokenizer.cpp
// Token extraction and similarity scoring for .c source files
// ─────────────────────────────────────────────────────────────

#include "../include/tokenizer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <set>

// ── C Keywords ────────────────────────────────────────────────
static const std::unordered_set<std::string> C_KEYWORDS = {
    "auto","break","case","char","const","continue","default","do",
    "double","else","enum","extern","float","for","goto","if","inline",
    "int","long","register","restrict","return","short","signed",
    "sizeof","static","struct","switch","typedef","union","unsigned",
    "void","volatile","while",
    // common stdlib identifiers treated as keywords
    "printf","scanf","main","include","define","NULL","EOF",
    "malloc","free","strlen","strcpy","strcmp","fprintf","fopen","fclose"
};

// ── Multi-char operators ──────────────────────────────────────
static const std::vector<std::string> MULTI_OPS = {
    "&&","||","==","!=","<=",">=","<<",">>","++","--","->","+=","-=","*=","/="
};

// ── Read entire file into string ──────────────────────────────
static bool readFile(const std::string& path, std::string& out) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

// ── Strip block comments /* ... */ ───────────────────────────
static std::string stripBlockComments(const std::string& src) {
    std::string out;
    out.reserve(src.size());
    size_t i = 0;
    while (i < src.size()) {
        if (i + 1 < src.size() && src[i] == '/' && src[i+1] == '*') {
            i += 2;
            while (i + 1 < src.size() &&
                   !(src[i] == '*' && src[i+1] == '/')) {
                if (src[i] == '\n') out += '\n'; // preserve line counts
                ++i;
            }
            i += 2; // skip */
        } else {
            out += src[i++];
        }
    }
    return out;
}

// ── Strip line comments // ... ────────────────────────────────
static std::string stripLineComments(const std::string& src) {
    std::string out;
    out.reserve(src.size());
    size_t i = 0;
    while (i < src.size()) {
        if (i + 1 < src.size() && src[i] == '/' && src[i+1] == '/') {
            while (i < src.size() && src[i] != '\n') ++i;
        } else {
            out += src[i++];
        }
    }
    return out;
}

// ── Replace string literals with STR ─────────────────────────
static std::string replaceStringLiterals(const std::string& src) {
    std::string out;
    out.reserve(src.size());
    size_t i = 0;
    while (i < src.size()) {
        if (src[i] == '"') {
            out += "\"STR\"";
            ++i;
            while (i < src.size() && src[i] != '"') {
                if (src[i] == '\\') ++i; // skip escape
                ++i;
            }
            if (i < src.size()) ++i; // skip closing "
        } else if (src[i] == '\'') {
            out += "'C'";
            ++i;
            while (i < src.size() && src[i] != '\'') {
                if (src[i] == '\\') ++i;
                ++i;
            }
            if (i < src.size()) ++i;
        } else {
            out += src[i++];
        }
    }
    return out;
}

// ── Strip preprocessor lines ──────────────────────────────────
static std::string stripPreprocessor(const std::string& src) {
    std::string out;
    std::istringstream stream(src);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = line;
        // ltrim
        size_t start = trimmed.find_first_not_of(" \t");
        if (start != std::string::npos && trimmed[start] == '#') {
            out += '\n'; // preserve line count
        } else {
            out += line + '\n';
        }
    }
    return out;
}

// ── Extract raw tokens from cleaned source ────────────────────
static std::vector<std::string> extractTokens(const std::string& src) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < src.size()) {
        // Skip whitespace
        if (std::isspace(src[i])) { ++i; continue; }

        // Multi-char operators
        bool foundMulti = false;
        for (const auto& op : MULTI_OPS) {
            if (src.compare(i, op.size(), op) == 0) {
                tokens.push_back(op);
                i += op.size();
                foundMulti = true;
                break;
            }
        }
        if (foundMulti) continue;

        // Numbers
        if (std::isdigit(src[i])) {
            std::string num;
            while (i < src.size() &&
                   (std::isdigit(src[i]) || src[i] == '.')) {
                num += src[i++];
            }
            tokens.push_back(num);
            continue;
        }

        // Identifiers and keywords
        if (std::isalpha(src[i]) || src[i] == '_') {
            std::string word;
            while (i < src.size() &&
                   (std::isalnum(src[i]) || src[i] == '_')) {
                word += src[i++];
            }
            tokens.push_back(word);
            continue;
        }

        // Single-char operators and punctuation
        char c = src[i++];
        if (c == '+' || c == '-' || c == '*' || c == '/' ||
            c == '%' || c == '=' || c == '<' || c == '>' ||
            c == '!' || c == '&' || c == '|' || c == '^' ||
            c == '~' || c == '?' || c == ':' || c == ';' ||
            c == ',' || c == '.' || c == '(' || c == ')' ||
            c == '[' || c == ']' || c == '{' || c == '}') {
            tokens.push_back(std::string(1, c));
        }
        // anything else (quotes already replaced) — skip
    }
    return tokens;
}

// ── Normalize tokens ──────────────────────────────────────────
// Replaces user-defined identifiers with VAR_0, VAR_1, etc.
// Keywords and operators stay as-is
static std::vector<std::string> normalizeTokens(
    const std::vector<std::string>& raw,
    std::vector<std::string>& identifiers_out,
    std::map<std::string,int>& kw_freq_out)
{
    std::unordered_map<std::string,std::string> mapping;
    std::vector<std::string> norm;
    int counter = 0;

    for (const auto& tok : raw) {
        if (C_KEYWORDS.count(tok)) {
            norm.push_back(tok);
            kw_freq_out[tok]++;
        }
        else if (!tok.empty() && (std::isalpha(tok[0]) || tok[0] == '_')) {
            // User-defined identifier
            if (!mapping.count(tok)) {
                mapping[tok] = "VAR_" + std::to_string(counter++);
                identifiers_out.push_back(tok);
            }
            norm.push_back(mapping[tok]);
        }
        else if (!tok.empty() && std::isdigit(tok[0])) {
            norm.push_back("NUM");
        }
        else if (tok == "\"STR\"") {
            norm.push_back("STR");
        }
        else {
            norm.push_back(tok); // operator or punctuation
        }
    }
    return norm;
}

// ── LCS length (space-optimized DP) ──────────────────────────
static int lcsLength(const std::vector<std::string>& a,
                     const std::vector<std::string>& b) {
    int m = (int)a.size(), n = (int)b.size();
    // Limit size for performance on large files
    int maxLen = 500;
    if (m > maxLen) m = maxLen;
    if (n > maxLen) n = maxLen;

    std::vector<int> prev(n + 1, 0), curr(n + 1, 0);
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (a[i-1] == b[j-1])
                curr[j] = prev[j-1] + 1;
            else
                curr[j] = std::max(prev[j], curr[j-1]);
        }
        std::swap(prev, curr);
        std::fill(curr.begin(), curr.end(), 0);
    }
    return prev[n];
}

// ═════════════════════════════════════════════════════════════
// PUBLIC FUNCTIONS
// ═════════════════════════════════════════════════════════════

TokenResult tokenize(const std::string& filepath) {
    TokenResult result;
    result.filepath    = filepath;
    result.token_count = 0;
    result.success     = false;

    std::string source;
    if (!readFile(filepath, source)) {
        result.error = "File not found or unreadable: " + filepath;
        return result;
    }

    // Clean the source
    source = stripBlockComments(source);
    source = stripLineComments(source);
    source = replaceStringLiterals(source);
    source = stripPreprocessor(source);

    // Extract raw tokens
    result.raw = extractTokens(source);
    result.token_count = (int)result.raw.size();

    // Normalize
    result.normalized = normalizeTokens(
        result.raw,
        result.identifiers,
        result.keyword_freq
    );

    result.success = true;
    return result;
}

double jaccardSimilarity(const TokenResult& a, const TokenResult& b) {
    if (!a.success || !b.success) return 0.0;

    std::set<std::string> setA(a.normalized.begin(), a.normalized.end());
    std::set<std::string> setB(b.normalized.begin(), b.normalized.end());

    std::vector<std::string> inter, uni;
    std::set_intersection(setA.begin(), setA.end(),
                          setB.begin(), setB.end(),
                          std::back_inserter(inter));
    std::set_union(setA.begin(), setA.end(),
                   setB.begin(), setB.end(),
                   std::back_inserter(uni));

    if (uni.empty()) return 1.0;
    return (double)inter.size() / (double)uni.size();
}

double lcsSimilarity(const TokenResult& a, const TokenResult& b) {
    if (!a.success || !b.success) return 0.0;
    if (a.normalized.empty() && b.normalized.empty()) return 1.0;
    if (a.normalized.empty() || b.normalized.empty()) return 0.0;

    int lcs = lcsLength(a.normalized, b.normalized);
    return (2.0 * lcs) / (a.normalized.size() + b.normalized.size());
}

SimilarityScore computeSimilarity(const TokenResult& a, const TokenResult& b) {
    SimilarityScore score;
    score.jaccard  = jaccardSimilarity(a, b);
    score.lcs      = lcsSimilarity(a, b);
    score.combined = (0.40 * score.jaccard) + (0.60 * score.lcs);

    auto [label, color] = classifySimilarity(score.combined);
    score.label = label;
    score.color = color;
    return score;
}

std::pair<std::string,std::string> classifySimilarity(double score) {
    double pct = score * 100.0;
    if (pct >= 95.0) return {"Exact Duplicate",    "#e05c5c"};
    if (pct >= 80.0) return {"High Similarity",    "#e05c5c"};
    if (pct >= 60.0) return {"Moderate Similarity","#f0a500"};
    if (pct >= 40.0) return {"Low Similarity",     "#4caf7d"};
    return              {"Minimal Similarity",  "#9090b0"};
}

// ── Self-test (run tokenizer.cpp directly) ────────────────────
// Uncomment main below and compile with:
// g++ tokenizer.cpp -o tokenizer_test.exe -std=c++17
// .\tokenizer_test.exe path\to\file1.c path\to\file2.c

/*
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: tokenizer_test.exe file1.c file2.c\n";
        return 1;
    }

    TokenResult r1 = tokenize(argv[1]);
    TokenResult r2 = tokenize(argv[2]);

    std::cout << "File 1: " << argv[1] << "\n";
    std::cout << "  Tokens: " << r1.token_count << "\n";
    std::cout << "  Keywords: ";
    for (auto& kv : r1.keyword_freq)
        std::cout << kv.first << "(" << kv.second << ") ";
    std::cout << "\n";

    std::cout << "File 2: " << argv[2] << "\n";
    std::cout << "  Tokens: " << r2.token_count << "\n\n";

    SimilarityScore s = computeSimilarity(r1, r2);
    std::cout << "Jaccard:  " << s.jaccard  * 100 << "%\n";
    std::cout << "LCS:      " << s.lcs      * 100 << "%\n";
    std::cout << "Combined: " << s.combined * 100 << "% -> " << s.label << "\n";

    return 0;
}
*/