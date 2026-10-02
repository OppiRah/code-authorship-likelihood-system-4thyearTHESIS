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

// Reads the Gemini API key from Windows Credential Manager (importing a
// legacy api_key.txt once if one is still lying around — see
// secret_store.h). Returns empty string if no key is configured.
std::string loadApiKey();

// AI settings dialog support: is a key stored / store one / remove it /
// test one with a single no-quota request (error filled on failure).
bool gemini_hasKey();
bool gemini_saveKey(const std::string& key);
bool gemini_removeKey();
bool gemini_testKey(const std::string& apiKey, std::string& error);

// At most this many flagged pairs (the highest-scoring ones) are sent
// to Gemini per run. A single section can flag hundreds of pairs, and
// one prompt covering all of them is too large to answer reliably
// (Tier 1 must return exactly one narrative per pair). Applied by
// runAnalysisPipeline(); pairs past the cap get a "not covered" note.
constexpr size_t GEMINI_MAX_PAIRS = 50;

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