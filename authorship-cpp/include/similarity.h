#ifndef SIMILARITY_H
#define SIMILARITY_H
// ─────────────────────────────────────────────────────────────
// similarity.h
// Pairwise similarity analysis engine
// Combines token-based + style-based comparison
// ─────────────────────────────────────────────────────────────

#include <string>
#include <vector>
#include <map>
#include "tokenizer.h"
#include "features.h"

// ── Result for one submission pair ────────────────────────────
struct PairResult {
    std::string studentA;
    std::string studentB;
    std::string pathA;
    std::string pathB;

    double tokenScore;    // 0-100
    double styleScore;    // 0-100
    double combinedScore; // 0-100  (token 60% + style 40%)

    std::string label;    // Exact Duplicate / High / Moderate / Low / Minimal
    std::string color;    // hex

    bool   flagged;       // combined >= 75%
    bool   success;
    std::string error;
};

// ── Summary stats across all pairs ───────────────────────────
struct SummaryStats {
    int    totalPairs;
    int    flaggedCount;
    int    exactCount;
    int    highCount;
    double avgScore;
    double maxScore;
};

// ── Public functions ──────────────────────────────────────────

// Analyze one pair of submissions
PairResult analyzePair(const std::string& nameA, const std::string& pathA,
                       const std::string& nameB, const std::string& pathB);

// Analyze all pairs from a name->path map
// Returns results sorted: flagged first, then by score descending
std::vector<PairResult> analyzeAll(
    const std::map<std::string, std::string>& submissions);

// Filter to flagged pairs only
std::vector<PairResult> flaggedPairs(const std::vector<PairResult>& results);

// Compute summary statistics
SummaryStats summaryStats(const std::vector<PairResult>& results);

#endif // SIMILARITY_H