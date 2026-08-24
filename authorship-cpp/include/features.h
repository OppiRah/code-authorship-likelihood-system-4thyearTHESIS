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