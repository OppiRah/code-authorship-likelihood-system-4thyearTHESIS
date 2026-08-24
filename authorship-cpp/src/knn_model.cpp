// ─────────────────────────────────────────────────────────────
// knn_model.cpp
// Per-student KNN authorship likelihood scorer
// ─────────────────────────────────────────────────────────────

#include "../include/knn_model.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

// ═════════════════════════════════════════════════════════════
// HELPERS
// ═════════════════════════════════════════════════════════════

std::pair<std::string,std::string> classifyAuthorship(double score) {
    double pct = score * 100.0;
    if (pct >= 80.0) return {"High Likelihood",      "#4caf7d"};
    if (pct >= 60.0) return {"Moderate Likelihood",  "#f0a500"};
    if (pct >= 40.0) return {"Low Likelihood",       "#e05c5c"};
    return               {"Very Low Likelihood",  "#e05c5c"};
}

static double euclidean(const std::vector<double>& a,
                         const std::vector<double>& b) {
    if (a.size() != b.size()) return 1.0;
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
        sum += (a[i] - b[i]) * (a[i] - b[i]);
    return sqrt(sum);
}

static AuthorshipResult makeError(const std::string& msg) {
    AuthorshipResult r;
    r.score     = 0.0;
    r.scorePct  = 0.0;
    r.label     = "Error";
    r.color     = "#9090b0";
    r.profileSize = 0;
    r.success   = false;
    r.error     = msg;
    return r;
}

// ═════════════════════════════════════════════════════════════
// AuthorshipScorer
// ═════════════════════════════════════════════════════════════

AuthorshipScorer::AuthorshipScorer(int k) : k_(k) {}

bool AuthorshipScorer::loadSubmission(const std::string& studentName,
                                       const std::string& filepath) {
    FeatureResult feat = extractFeatures(filepath);
    if (!feat.success) return false;

    StudentProfile& profile = profiles_[studentName];
    profile.name = studentName;
    profile.submissions.push_back(feat);
    rebuildProfile(profile);
    return true;
}

void AuthorshipScorer::rebuildProfile(StudentProfile& profile) {
    // Average normalized vectors across all submissions
    std::vector<std::vector<double>> normVecs;
    for (const auto& sub : profile.submissions)
        if (!sub.normalized.empty())
            normVecs.push_back(sub.normalized);

    if (!normVecs.empty())
        profile.profileVector = averageVectors(normVecs);

    // Rebuild style notes from latest submission
    if (!profile.submissions.empty() &&
        !profile.submissions.back().raw.empty())
        profile.styleNotes = describeStyle(profile.submissions.back().raw);
}

AuthorshipResult AuthorshipScorer::score(const std::string& studentName,
                                          const std::string& filepath) {
    // Find profile
    auto it = profiles_.find(studentName);
    if (it == profiles_.end())
        return makeError("No profile found for: " + studentName);

    const StudentProfile& profile = it->second;
    if (profile.submissions.empty())
        return makeError("Profile for " + studentName + " has no submissions.");

    // Extract features from test file
    FeatureResult test = extractFeatures(filepath);
    if (!test.success)
        return makeError(test.error);
    if (test.normalized.empty())
        return makeError("Could not extract features from: " + filepath);

    // Compute distance to every known submission
    struct DistEntry { std::string path; double dist; };
    std::vector<DistEntry> distances;

    for (const auto& known : profile.submissions) {
        if (known.normalized.empty()) continue;
        double d = euclidean(test.normalized, known.normalized);
        distances.push_back({known.filepath, d});
    }

    if (distances.empty())
        return makeError("No valid submissions to compare against.");

    // Sort ascending — closest first
    std::sort(distances.begin(), distances.end(),
              [](const DistEntry& a, const DistEntry& b){
                  return a.dist < b.dist; });

    // Average distance of k nearest
    int useK = std::min(k_, (int)distances.size());
    double avgDist = 0.0;
    for (int i = 0; i < useK; ++i) avgDist += distances[i].dist;
    avgDist /= useK;

    // Convert to score
    double maxDist   = sqrt((double)FEATURE_COUNT);
    double rawScore  = std::max(0.0, 1.0 - (avgDist / maxDist));

    // Confidence boost — more submissions = more reliable
    double conf      = std::min(1.0, 0.75 + (profile.submissions.size() * 0.05));
    double finalScore = rawScore * conf;

    auto [label, color] = classifyAuthorship(finalScore);

    // Style match notes
    std::vector<std::string> styleMatch;
    if (!profile.profileVector.empty())
        styleMatch = compareStyles(test.normalized, profile.profileVector);

    AuthorshipResult result;
    result.student     = studentName;
    result.filepath    = filepath;
    result.score       = finalScore;
    result.scorePct    = finalScore * 100.0;
    result.label       = label;
    result.color       = color;
    result.profileSize = (int)profile.submissions.size();
    result.styleMatch  = styleMatch;
    result.success     = true;
    result.error       = "";
    return result;
}

PairVerdict AuthorshipScorer::scorePair(const std::string& studentA,
                                         const std::string& fileA,
                                         const std::string& studentB,
                                         const std::string& fileB) {
    PairVerdict verdict;
    verdict.studentA = studentA;
    verdict.studentB = studentB;
    verdict.resultA  = score(studentA, fileA);
    verdict.resultB  = score(studentB, fileB);

    if (!verdict.resultA.success || !verdict.resultB.success) {
        verdict.likelyOriginal = "Unable to determine";
        verdict.likelyCopier   = "Unable to determine";
        verdict.success        = false;
        return verdict;
    }

    if (verdict.resultA.score >= verdict.resultB.score) {
        verdict.likelyOriginal = studentA;
        verdict.likelyCopier   = studentB;
    } else {
        verdict.likelyOriginal = studentB;
        verdict.likelyCopier   = studentA;
    }

    verdict.success = true;
    return verdict;
}

std::vector<std::string> AuthorshipScorer::readyStudents() const {
    std::vector<std::string> result;
    for (const auto& kv : profiles_)
        if (!kv.second.submissions.empty())
            result.push_back(kv.first);
    return result;
}

const StudentProfile* AuthorshipScorer::getProfile(
    const std::string& name) const {
    auto it = profiles_.find(name);
    return (it != profiles_.end()) ? &it->second : nullptr;
}

int AuthorshipScorer::studentCount() const {
    return (int)profiles_.size();
}