// ─────────────────────────────────────────────────────────────
// similarity.cpp
// Pairwise similarity analysis engine
// token-based (60%) + style-based (40%) = combined score
// ─────────────────────────────────────────────────────────────

#include "../include/similarity.h"
#include <algorithm>
#include <numeric>

// ── Internal error result ─────────────────────────────────────
static PairResult makeErrorPair(const std::string& nameA,
                                 const std::string& nameB,
                                 const std::string& msg) {
    PairResult r;
    r.studentA      = nameA;
    r.studentB      = nameB;
    r.tokenScore    = 0.0;
    r.styleScore    = 0.0;
    r.combinedScore = 0.0;
    r.label         = "Error";
    r.color         = "#9090b0";
    r.flagged       = false;
    r.success       = false;
    r.error         = msg;
    return r;
}

// ═════════════════════════════════════════════════════════════
// PUBLIC FUNCTIONS
// ═════════════════════════════════════════════════════════════

PairResult analyzePair(const std::string& nameA, const std::string& pathA,
                       const std::string& nameB, const std::string& pathB) {
    // ── Token similarity ──────────────────────────────────────
    TokenResult tokA = tokenize(pathA);
    TokenResult tokB = tokenize(pathB);

    if (!tokA.success)
        return makeErrorPair(nameA, nameB, "Cannot read: " + pathA);
    if (!tokB.success)
        return makeErrorPair(nameA, nameB, "Cannot read: " + pathB);

    SimilarityScore simScore = computeSimilarity(tokA, tokB);
    double tokenPct = simScore.combined * 100.0;

    // ── Style similarity ──────────────────────────────────────
    FeatureResult featA = extractFeatures(pathA);
    FeatureResult featB = extractFeatures(pathB);

    double stylePct = 0.0;
    if (featA.success && featB.success &&
        !featA.normalized.empty() && !featB.normalized.empty()) {
        stylePct = styleSimilarity(featA.normalized, featB.normalized) * 100.0;
    }

    // ── Combined score: 60% token + 40% style ────────────────
    double combined = (tokenPct * 0.60) + (stylePct * 0.40);

    // ── Classify ──────────────────────────────────────────────
    auto [label, color] = classifySimilarity(combined / 100.0);

    PairResult result;
    result.studentA      = nameA;
    result.studentB      = nameB;
    result.pathA         = pathA;
    result.pathB         = pathB;
    result.tokenScore    = tokenPct;
    result.styleScore    = stylePct;
    result.combinedScore = combined;
    result.label         = label;
    result.color         = color;
    result.flagged       = (combined >= 75.0);
    result.success       = true;
    result.error         = "";
    return result;
}

std::vector<PairResult> analyzeAll(
    const std::map<std::string, std::string>& submissions)
{
    std::vector<PairResult> results;
    if (submissions.size() < 2) return results;

    // Collect names in a vector for pairwise iteration
    std::vector<std::string> names;
    for (const auto& kv : submissions) names.push_back(kv.first);

    for (size_t i = 0; i < names.size(); ++i) {
        for (size_t j = i + 1; j < names.size(); ++j) {
            const std::string& nA = names[i];
            const std::string& nB = names[j];
            PairResult r = analyzePair(
                nA, submissions.at(nA),
                nB, submissions.at(nB));
            results.push_back(r);
        }
    }

    // Sort: flagged first, then by combined score descending
    std::sort(results.begin(), results.end(),
        [](const PairResult& a, const PairResult& b) {
            if (a.flagged != b.flagged) return a.flagged > b.flagged;
            return a.combinedScore > b.combinedScore;
        });

    return results;
}

std::vector<PairResult> flaggedPairs(const std::vector<PairResult>& results) {
    std::vector<PairResult> out;
    for (const auto& r : results)
        if (r.flagged) out.push_back(r);
    return out;
}

SummaryStats summaryStats(const std::vector<PairResult>& results) {
    SummaryStats s;
    s.totalPairs   = (int)results.size();
    s.flaggedCount = 0;
    s.exactCount   = 0;
    s.highCount    = 0;
    s.avgScore     = 0.0;
    s.maxScore     = 0.0;

    if (results.empty()) return s;

    double sum = 0.0;
    for (const auto& r : results) {
        if (r.flagged)              ++s.flaggedCount;
        if (r.label == "Exact Duplicate") ++s.exactCount;
        if (r.label == "High Similarity") ++s.highCount;
        sum += r.combinedScore;
        if (r.combinedScore > s.maxScore) s.maxScore = r.combinedScore;
    }
    s.avgScore = sum / results.size();
    return s;
}