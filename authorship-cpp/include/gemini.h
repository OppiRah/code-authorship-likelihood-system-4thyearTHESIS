#ifndef GEMINI_H
#define GEMINI_H
// ─────────────────────────────────────────────────────────────
// gemini.h
// Gemini API integration for AI-generated instructor summaries
// ─────────────────────────────────────────────────────────────

#include <string>
#include <vector>
#include "similarity.h"
#include "knn_model.h"
#include "results_data.h"

// ── Result of an AI summary generation attempt ────────────────
struct AISummary {
    std::string text;       // The generated summary
    bool        success;    // True if API call succeeded
    std::string error;      // Error message if failed
    bool        skipped;    // True if no API key found (graceful skip)
};

// ── Public functions (legacy, single-blob summary) ────────────
// Unchanged — this is the pipeline entry point at main.cpp:2201, and the
// separate legacy CLI call site (~main.cpp:2415). Left untouched per the
// out-of-scope decision on the CLI path; the tiered functions below are
// the new call path for the main GUI pipeline.

// Reads API key from api_key.txt
// Returns empty string if file missing or empty
std::string loadApiKey();

// Builds the prompt sent to Gemini based on analysis results
std::string buildPrompt(const std::vector<PairResult>& flagged,
                        AuthorshipScorer& scorer);

// Calls Gemini API and returns the generated summary
// If no API key is found, returns AISummary with skipped=true
AISummary generateAISummary(const std::vector<PairResult>& flagged,
                             AuthorshipScorer& scorer);

// ── Tiered AI outputs (restructure) ────────────────────────────
// Input type is PairAnalysisDisplay, not PairResult — confirmed by the
// investigation (per-feature deviation data does not exist on PairResult;
// it's only reachable via AuthorshipScorer::score(), which has already
// been called and parsed into PairAnalysisDisplay/AuthorshipDisplay/
// StyleNoteDisplay by the time runAnalysisPipeline() reaches the AI call).
// Because that struct already carries scorePct/profileSize/reliability/
// per-feature notes, these functions do NOT take an AuthorshipScorer&
// the way the spec's stub signatures did — there is nothing left for the
// scorer to answer that isn't already sitting on the struct. Flagging
// this signature deviation explicitly since it departs from the spec text.

// Per-pair evidence narration (Tier 1). Also folds in the Tier 3
// reliability/confidence explainer as a prompt-level instruction (no
// separate function, per spec §3 Tier 3).
// NOTE: AIPairNarrative and AIBatchPatterns are defined in results_data.h,
// not here — AnalysisResults needs them as real member types, and
// results_data.h is included by this header, so defining them here would
// create a circular include the moment AnalysisResults grew fields for
// them. They're declared alongside PairAnalysisDisplay/StyleNoteDisplay,
// which are the same kind of plain data-carrier struct.

// Returns a vector PARALLEL to `flagged` (same size, same order) — result[i]
// corresponds to flagged[i]. This is the intended access pattern for
// gui.cpp, which already indexes g_analysisResults.flaggedPairs by position
// (see drawFlaggedPairBlock's pairIdx). All pairs are sent in a single
// batched API call (see gemini.cpp for the rate-limit/batching rationale);
// on any batch-level failure every element carries the same
// success/skipped/error state, since a single HTTP call has no partial
// outcome to report per-pair.
std::vector<AIPairNarrative> generatePairNarratives(
    const std::vector<PairAnalysisDisplay>& flagged);

// Cross-pair pattern detection (Tier 2). Skips (does not call the API)
// when fewer than 2 pairs are flagged, since there is nothing to compare.
AIBatchPatterns generateCrossPairPatterns(
    const std::vector<PairAnalysisDisplay>& flagged);

#endif // GEMINI_H