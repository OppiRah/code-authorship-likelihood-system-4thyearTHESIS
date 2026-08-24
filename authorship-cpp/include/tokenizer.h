#pragma once
// ─────────────────────────────────────────────────────────────
// tokenizer.h
// Token extraction and similarity scoring for .c source files
// ─────────────────────────────────────────────────────────────

#include <string>
#include <vector>
#include <map>

// ── Token types ───────────────────────────────────────────────
enum class TokenType {
    KEYWORD,      // int, for, while, if, printf, etc.
    IDENTIFIER,   // variable/function names → normalized to VAR_N
    NUMBER,       // numeric literals → normalized to NUM
    OPERATOR,     // +, -, =, ==, !=, <=, etc.
    PUNCTUATION,  // { } ( ) [ ] ; , .
    STRING_LIT,   // "hello" → normalized to STR
    UNKNOWN
};

// ── Single token ──────────────────────────────────────────────
struct Token {
    std::string value;      // the actual text
    std::string normalized; // VAR_N, NUM, STR, or original if keyword/operator
    TokenType   type;
};

// ── Result of tokenizing one file ────────────────────────────
struct TokenResult {
    std::string              filepath;
    std::vector<std::string> raw;         // original token values
    std::vector<std::string> normalized;  // normalized token values
    std::map<std::string,int> keyword_freq; // keyword → count
    std::vector<std::string> identifiers; // unique user-defined names
    int                      token_count;
    bool                     success;
    std::string              error;
};

// ── Similarity scores between two files ──────────────────────
struct SimilarityScore {
    double jaccard;   // set-based overlap
    double lcs;       // sequence-aware similarity
    double combined;  // 40% jaccard + 60% lcs
    std::string label;  // Exact Duplicate / High / Moderate / Low / Minimal
    std::string color;  // for future UI use
};

// ── Public functions ──────────────────────────────────────────

// Tokenize a .c source file
TokenResult tokenize(const std::string& filepath);

// Compute Jaccard similarity between two normalized token sets
double jaccardSimilarity(const TokenResult& a, const TokenResult& b);

// Compute LCS-based sequence similarity
double lcsSimilarity(const TokenResult& a, const TokenResult& b);

// Combined similarity score (main function to call)
SimilarityScore computeSimilarity(const TokenResult& a, const TokenResult& b);

// Classify a combined score into a label
std::pair<std::string,std::string> classifySimilarity(double score);