#ifndef FEATURES_H
#define FEATURES_H
// ─────────────────────────────────────────────────────────────
// features.h
// 14 coding style feature extraction from .c source files
// ─────────────────────────────────────────────────────────────

#include <string>
#include <vector>

// ── Constants ─────────────────────────────────────────────────
#define FEATURE_COUNT 14

// Max expected value per feature for 0-1 normalization
extern const double FEATURE_MAX[FEATURE_COUNT];
extern const char*  FEATURE_LABELS[FEATURE_COUNT];

// ── Structs ───────────────────────────────────────────────────
struct FeatureResult {
    std::string         filepath;
    std::vector<double> raw;        // raw feature values
    std::vector<double> normalized; // values scaled 0-1
    bool                success;
    std::string         error;
};

struct StyleNote {
    std::string feature;
    std::string observation;
};

// ── Per-function style (View source "style change" highlight) ──
// Display-only — NOT used by the scoring pipeline. Measures the four
// formatting features a student controls themselves (naming is left
// out: activities often hand out variable names) on each function of
// a file, using the same definitions as extractFeatures(), so a block's
// values are directly comparable to the student's per-submission raw
// vectors. `has[k]` is false when the block has too little of that
// construct to judge (e.g. no braces, fewer than 3 operators).
static const int BLOCK_STYLE_COUNT = 4;
extern const int BLOCK_STYLE_FEATURE[BLOCK_STYLE_COUNT]; // indices into the 14

struct BlockStyle {
    int    startLine;   // 0-based, the function's header line
    int    endLine;     // 0-based, inclusive (the closing brace)
    double value[BLOCK_STYLE_COUNT];
    bool   has[BLOCK_STYLE_COUNT];
};

// Functions shorter than this many lines are skipped (too little signal).
static const int BLOCK_STYLE_MIN_LINES = 5;

std::vector<BlockStyle> measureFunctionStyles(const std::string& source);

// ── Public functions ──────────────────────────────────────────
FeatureResult           extractFeatures (const std::string& filepath);
std::vector<double>     normalizeVector (const std::vector<double>& raw);
std::vector<double>     averageVectors  (const std::vector<std::vector<double>>& vecs);
double                  euclideanDistance(const std::vector<double>& a, const std::vector<double>& b);
double                  styleSimilarity (const std::vector<double>& a, const std::vector<double>& b);
std::vector<StyleNote>  describeStyle   (const std::vector<double>& vec);
std::vector<std::string> compareStyles  (const std::vector<double>& test_vec,
                                         const std::vector<double>& profile_vec);

#endif // FEATURES_H