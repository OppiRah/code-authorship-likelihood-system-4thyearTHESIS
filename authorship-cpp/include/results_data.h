#ifndef RESULTS_DATA_H
#define RESULTS_DATA_H
// ─────────────────────────────────────────────────────────────
// results_data.h
// Data structures for sharing analysis results with the GUI
// ─────────────────────────────────────────────────────────────

#include <string>
#include <vector>
#include <functional>

// Progress instrumentation (spec §5.5) — shared between main.cpp
// (which reports) and gui.cpp (which consumes). Declared once here
// so both translation units see the exact same enum, rather than
// each declaring their own copy that could silently drift apart.
// This is a pure reporting/control hook: it does not affect scoring,
// weighting, or algorithm behavior. The callback returns false to
// request cancellation (checked only in the pairwise comparison loop,
// the one phase long enough for cancellation to meaningfully matter);
// returning true (or having no callback at all, as in CLI mode) means
// "keep going."
enum class AnalysisPhase {
    LoadingFiles, BuildingProfiles, ComparingPairs, GeneratingSummary
};
using AnalysisProgressCallback =
    std::function<bool(AnalysisPhase phase, int current, int total)>;

// Per-feature comparison note (used for GUI display)
struct StyleNoteDisplay {
    std::string category;
    std::string feature;
    std::string observation;
    double      profileValue;
    double      submissionValue;
    bool        isMatch;
};

struct AuthorshipDisplay {
    std::string filename;
    std::string studentName;
    double      scorePct;
    std::string label;
    int         profileSize;
    std::string reliabilityLabel;
    std::string reliabilityNote;
    std::vector<StyleNoteDisplay> lexicalFeatures;
    std::vector<StyleNoteDisplay> layoutFeatures;
    std::vector<StyleNoteDisplay> syntacticFeatures;
    int matchedCount;
    int totalFeatures;
};

struct PairAnalysisDisplay {
    std::string filenameA;
    std::string filenameB;
    double      tokenScore;
    double      styleScore;
    double      combinedScore;
    std::string similarityLabel;
    AuthorshipDisplay studentA;
    AuthorshipDisplay studentB;
    std::string interpretation;
};

// AI-generated per-pair evidence narration (Tier 1 of the AI summary
// restructure). Defined here rather than in gemini.h so that
// AnalysisResults below can carry a std::vector<AIPairNarrative> without
// gemini.h and results_data.h including each other.
struct AIPairNarrative {
    std::string pairId;      // "<studentA> vs <studentB>", for logging/debugging
    std::string narrative;   // 2-3 sentences
    bool        success;
    std::string error;
    bool        skipped;
};

// AI-generated cross-pair pattern analysis (Tier 2 of the AI summary
// restructure). See AIPairNarrative above for why this lives here.
struct AIBatchPatterns {
    // Each element is one distinct pattern/finding (2-3 sentences), not
    // one flowing paragraph -- rendered as its own row in the Overview
    // tab. Empty when skipped or failed; those states are read from
    // skipped/error below exactly as before, independent of this field.
    std::vector<std::string> findings;
    bool        success;
    std::string error;
    bool        skipped;
};

struct StudentProfileSummary {
    std::string name;
    int         submissionCount;
    std::vector<std::string> styleNotes;

    // The 14 normalized stylometric values (0..1) backing this
    // profile. Surfaced for the Style DNA strip, which renders each
    // value as bar HEIGHT rather than color — making a student's
    // fingerprint a glanceable shape. Empty if unavailable.
    std::vector<double> featureVector;
};

struct PairRow {
    std::string filenameA;
    std::string filenameB;
    double      tokenScore;
    double      styleScore;
    double      combinedScore;
    std::string label;
    bool        flagged;
};

struct AnalysisResults {
    bool        valid;

    int    totalPairs;
    int    flaggedCount;
    int    exactDuplicates;
    int    highSimilarity;
    double maxScore;
    double averageScore;
    std::string dataFolder;
    std::string reportPath;
    std::string timestamp;

    bool        aiSummarySuccess;
    bool        aiSummarySkipped;
    std::string aiSummaryText;
    std::string aiSummaryError;

    // Tiered AI outputs (restructure). Additive alongside the aiSummary*
    // fields above, which remain wired to the legacy CLI path only
    // (main.cpp's commented-"kept for scripting/testing" branch) and are
    // otherwise unused once the GUI pipeline switches to these two.
    // aiPairNarratives is parallel to flaggedPairs below (same size, same
    // order) once populated -- see generatePairNarratives()'s contract.
    std::vector<AIPairNarrative> aiPairNarratives;
    AIBatchPatterns              aiBatchPatterns;

    std::vector<StudentProfileSummary> profiles;
    std::vector<PairAnalysisDisplay>   flaggedPairs;
    std::vector<PairRow>               allPairs;

    // Files that did not match any known category (Classroom title
    // signal or filename pattern) — excluded from profile-building
    // and pairwise comparison, surfaced here for instructor review.
    // This bucket covers BOTH "type unknown" (no exam/activity/quiz
    // signal) and "student unknown" cases — they're treated as one
    // unified "needs instructor attention" list rather than two
    // separate concepts, since the fix is the same either way: the
    // instructor manually assigns the file to a known student.
    struct UnclassifiedFile {
        std::string key;      // bare filename (no extension) — map key
        std::string fullPath; // full disk path, needed for assign/delete
    };
    std::vector<UnclassifiedFile> unclassifiedFiles;

    // Files whose content clearly isn't C (Java, Python, C++, etc.
    // submitted with a .c extension) — excluded from profile-building
    // and pairwise comparison, since neither should be computed from
    // non-C style patterns. Each entry is "filename (suspected: Java)".
    std::vector<std::string> languageMismatchFiles;
};

#endif // RESULTS_DATA_H