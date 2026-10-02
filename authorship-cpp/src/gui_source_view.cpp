// ─────────────────────────────────────────────────────────────
// gui_source_view.cpp
// The "View source" modal (MODAL_SOURCE_VIEW): read-only viewer for a
// submission, and compare mode for a flagged pair — gold line-match
// highlight, red style-change highlight, A/B tabs, prev/next jumps,
// "How to read" legend, eased scrolling.
//
// Split out of gui.cpp (2026-10-02), same procedure as the ContentProc
// checkpoints: the state, loading, matching and drawing code moved
// unchanged; ContentProc's inline key/wheel/timer/click branches for
// this modal became the four handlers at the bottom, which ContentProc
// now calls. Display-only: nothing here feeds back into scoring.
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"
#include "../include/app_paths.h"

#ifdef _WIN32

static RECT g_sourceViewCloseBtnRect = {}; // MODAL_SOURCE_VIEW's own close ("x") button
static RECT g_sourceViewLegendBtnRect = {}; // compare mode's "How to read" toggle (empty otherwise)
// MODAL_SOURCE_VIEW compare mode's toolbar hit rects (empty when not
// shown / not clickable) — set by drawSourceViewModal each paint.
static RECT g_sourceViewTabRects[2] = {};
static RECT g_sourceViewPrevRect = {};
static RECT g_sourceViewNextRect = {};
// Compare-mode style-change highlight: dark red fill, bright red left bar,
// light text — readable on the modal's GRAY_900 card.
static const COLORREF SOURCE_STYLE_BG   = RGB(104, 26, 32);
static const COLORREF SOURCE_STYLE_BAR  = RGB(230, 80, 80);
static const COLORREF SOURCE_STYLE_TEXT = RGB(255, 226, 226);
// Compare-mode match highlight: a translucent gold wash (so a line that
// is also red still shows its red underneath) plus a solid gold left bar.
// At ~40% alpha the text keeps >= 4.5:1 contrast on both GRAY_900
// (~6.9:1) and the red fill (~4.9:1).
static const COLORREF SOURCE_MATCH_FILL  = GOLD_500;
static const BYTE     SOURCE_MATCH_ALPHA = 100;
static const COLORREF SOURCE_MATCH_BAR   = GOLD_400;
static const COLORREF SOURCE_MATCH_TEXT  = RGB(255, 244, 214);

// ── "View source" feature: read cap ───────────────────────────────
// Generous for a real student C submission (typically well under
// 100KB) while still bounding the worst case so a pathological file
// can't stall the paint loop or blow up memory. Whichever limit hits
// first wins; either way sourceTruncated is set so the modal says so
// explicitly instead of silently cutting off.
static const std::streamsize SOURCE_VIEW_MAX_BYTES = 4 * 1024 * 1024;
static const size_t          SOURCE_VIEW_MAX_LINES = 5000;

// Reads `path` for MODAL_SOURCE_VIEW, read-only — never touches the
// analysis pipeline, purely a GUI-side file read for display. If `path`
// can't be opened (the submission was moved, deleted, or came from a
// Classroom folder that's since been re-synced to different paths),
// `side.missing` is set with a fallback message instead of a crash or
// a blank panel. `rawOut` (optional) receives the same capped bytes the
// lines were split from, for compare mode's line matching.
static void loadSourceViewSide(const std::wstring& label, const std::wstring& subtitle,
                                const std::string& path, SourceViewSide& side,
                                std::string* rawOut)
{
    side = SourceViewSide{};
    side.title = label;

    std::ifstream f(pathFromUtf8(path), std::ios::binary);
    if (!f) {
        side.missing = true;
        side.subtitle =
            L"This file is no longer at the recorded location — it may "
            L"have been moved, deleted, or the Classroom folder re-synced.";
        return;
    }
    // Read at most the cap, and only past it, so a pathological file
    // never gets fully loaded into memory just to be truncated.
    std::vector<char> buf((size_t)SOURCE_VIEW_MAX_BYTES);
    f.read(buf.data(), SOURCE_VIEW_MAX_BYTES);
    std::streamsize got = f.gcount();
    bool truncated = false;
    if (got == SOURCE_VIEW_MAX_BYTES) {
        char probe;
        if (f.read(&probe, 1)) truncated = true;
    }

    std::string lineBuf;
    auto flushLine = [&]() {
        if (side.lines.size() >= SOURCE_VIEW_MAX_LINES) { truncated = true; return; }
        // Strip a trailing \r (CRLF files) so lines don't show a
        // stray box glyph at the end.
        if (!lineBuf.empty() && lineBuf.back() == '\r') lineBuf.pop_back();
        side.lines.push_back(s2w(lineBuf));
        lineBuf.clear();
    };
    for (std::streamsize i = 0; i < got; ++i) {
        if (side.lines.size() >= SOURCE_VIEW_MAX_LINES) { truncated = true; break; }
        char c = buf[(size_t)i];
        if (c == '\n') flushLine();
        else lineBuf.push_back(c);
    }
    if (!lineBuf.empty() && side.lines.size() < SOURCE_VIEW_MAX_LINES) flushLine();

    side.truncated = truncated;
    side.subtitle  = subtitle;
    if (rawOut) rawOut->assign(buf.data(), (size_t)got);
}

// Opens the MODAL_SOURCE_VIEW modal for a single file (no comparison,
// no highlighting). See loadSourceViewSide for the missing-file case.
void openSourceViewModal(const std::wstring& label, const std::wstring& subtitle,
                          const std::string& path)
{
    SourceViewSide side;
    loadSourceViewSide(label, subtitle, path, side, nullptr);

    AppModal m;
    m.kind            = MODAL_SOURCE_VIEW;
    m.title           = side.title;
    m.sourceLines     = std::move(side.lines);
    m.sourceSubtitle  = side.subtitle;
    m.sourceMissing   = side.missing;
    m.sourceTruncated = side.truncated;

    g_appModal = m;
    g_modalFocusOnPrimary = true;
    if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
}

// ── "View source" compare mode: gold line-match highlight ─────────
// Lines with fewer normalized tokens than this are left out of the
// match entirely (neither highlighted nor used as an LCS anchor) —
// `}`, `i++;`, `return 0;` appear in nearly every submission and
// would drown out the actual evidence.
static const size_t SOURCE_MATCH_MIN_TOKENS = 4;
// Bounds the LCS table ((n+1)*(m+1) uint16 cells, ~8MB at the cap).
// Past it highlighting is skipped with a note rather than stalling.
static const size_t SOURCE_MATCH_MAX_SIG_LINES = 2000;

// Finds which lines of A and B correspond: each line is normalized by
// tokenizer.cpp's own cleaning + normalizeTokens (per line, so renamed
// variables still match — see normalizeSourceLines), then a line-level
// LCS over the significant lines pairs them up in order. Fills
// partner/matchHighlight/matchRunStarts on both sides; returns the
// matched-line count, or -1 with `note` set if matching couldn't run.
static int computeSourceLineMatches(const std::string& rawA, SourceViewSide& a,
                                     const std::string& rawB, SourceViewSide& b,
                                     std::wstring& note)
{
    SourceViewSide* sides[2] = {&a, &b};
    for (SourceViewSide* s : sides) {
        s->partner.assign(s->lines.size(), -1);
        s->matchHighlight.assign(s->lines.size(), 0);
        s->matchRunStarts.clear();
    }

    std::vector<std::vector<std::string>> normA, normB;
    if (!normalizeSourceLines(rawA, normA) || !normalizeSourceLines(rawB, normB)) {
        note = L"match highlighting unavailable — a file couldn't be split into lines cleanly";
        return -1;
    }

    // Significant lines -> interned ids, so the DP compares ints.
    std::unordered_map<std::string, int> intern;
    auto collect = [&](const std::vector<std::vector<std::string>>& norm, size_t shown,
                       std::vector<int>& idx, std::vector<int>& key) {
        for (size_t i = 0; i < norm.size() && i < shown; ++i) {
            if (norm[i].size() < SOURCE_MATCH_MIN_TOKENS) continue;
            std::string k;
            for (const auto& t : norm[i]) { k += t; k += ' '; }
            auto it = intern.emplace(k, (int)intern.size()).first;
            idx.push_back((int)i);
            key.push_back(it->second);
        }
    };
    std::vector<int> idxA, keyA, idxB, keyB;
    collect(normA, a.lines.size(), idxA, keyA);
    collect(normB, b.lines.size(), idxB, keyB);

    if (idxA.size() > SOURCE_MATCH_MAX_SIG_LINES || idxB.size() > SOURCE_MATCH_MAX_SIG_LINES) {
        note = L"match highlighting skipped — file too long to compare";
        return -1;
    }

    size_t n = idxA.size(), m = idxB.size();
    std::vector<uint16_t> dp((n + 1) * (m + 1), 0);
    auto at = [&](size_t i, size_t j) -> uint16_t& { return dp[i * (m + 1) + j]; };
    for (size_t i = n; i-- > 0;)
        for (size_t j = m; j-- > 0;)
            at(i, j) = (keyA[i] == keyB[j]) ? (uint16_t)(at(i + 1, j + 1) + 1)
                                            : std::max(at(i + 1, j), at(i, j + 1));

    int matched = 0;
    for (size_t i = 0, j = 0; i < n && j < m;) {
        if (keyA[i] == keyB[j]) {
            a.partner[idxA[i]] = idxB[j];
            b.partner[idxB[j]] = idxA[i];
            ++matched; ++i; ++j;
        } else if (at(i + 1, j) >= at(i, j + 1)) ++i;
        else ++j;
    }

    // Runs: consecutive matched significant lines (insignificant lines
    // between them don't break a run, and are filled in so a copied
    // block reads as one solid highlight rather than a striped one).
    auto buildRuns = [](SourceViewSide& s, const std::vector<int>& sig) {
        int prevMatched = -1;
        for (int li : sig) {
            if (s.partner[li] >= 0) {
                if (prevMatched < 0) s.matchRunStarts.push_back(li);
                else for (int k = prevMatched + 1; k < li; ++k) s.matchHighlight[k] = 1;
                s.matchHighlight[li] = 1;
                prevMatched = li;
            } else {
                prevMatched = -1;
            }
        }
    };
    buildRuns(a, idxA);
    buildRuns(b, idxB);
    return matched;
}

// ── "View source": style-change highlight (display-only) ──────────
// Red = a function that breaks this student's OWN established pattern.
// Baseline: the student's profile-building submissions (Activity/Quiz)
// minus the file being viewed (leave-one-out), as a per-feature mean
// and spread. Only the four formatting features measureFunctionStyles
// (features.h) reports are judged — naming is left out because
// activities often hand out variable names. Never fed back into scoring.
static const int    STYLE_MIN_HISTORY = 3;   // prior submissions needed to judge a feature
static const double STYLE_SPREAD_MULT = 2.0; // "changed" = beyond 2x the student's usual spread...
// ...or this floor, whichever is larger (raw units: the same tolerances
// compareStyles uses file-level; line length = 0.15 x 80 chars). Keeps
// a very consistent student from being flagged over noise.
static const double STYLE_FLOOR[BLOCK_STYLE_COUNT] = {0.30, 0.25, 0.20, 12.0};
static const int    STYLE_MIN_CHANGED = 2;   // features that must change together to flag

static std::string normPathKey(std::string p) {
    for (auto& c : p) c = (c == '\\') ? '/' : (char)tolower((unsigned char)c);
    return p;
}

static std::wstring describeStyleChange(int k, double v, double mean) {
    switch (k) {
    case 0:  return v < mean ? L"braces on next line (usually same line)"
                             : L"braces on same line (usually next line)";
    case 1:  return v < mean ? L"operators unspaced (usually spaced)"
                             : L"operators spaced (usually unspaced)";
    case 2:  return v > mean ? L"more inline comments than usual"
                             : L"fewer inline comments than usual";
    default: return L"lines ~" + std::to_wstring((int)(v + 0.5)) +
                    L" chars (usually ~" + std::to_wstring((int)(mean + 0.5)) + L")";
    }
}

// Fills highlight/runStarts/flagReason/flagCount on `s` (or styleNote
// when no judgment can be made) for the file at `path`, owned by
// `student`, whose capped bytes are `raw`.
static void computeStyleChanges(const std::string& raw, const std::string& path,
                                const std::string& student, SourceViewSide& s)
{
    s.highlight.assign(s.lines.size(), 0);
    s.flagReason.assign(s.lines.size(), std::wstring());
    s.runStarts.clear();
    s.flagCount = 0;

    const StudentProfileSummary* prof = nullptr;
    for (const auto& p : g_analysisResults.profiles)
        if (p.name == student) { prof = &p; break; }
    if (!prof) { s.styleNote = L"no style profile for this student"; return; }

    // Leave-one-out: this file never counts toward its own baseline.
    std::string self = normPathKey(path);
    std::vector<const std::vector<double>*> hist;
    for (size_t i = 0; i < prof->submissionRaw.size(); ++i)
        if (i >= prof->submissionPaths.size() || normPathKey(prof->submissionPaths[i]) != self)
            hist.push_back(&prof->submissionRaw[i]);
    if ((int)hist.size() < STYLE_MIN_HISTORY) {
        s.styleNote = L"not enough history to judge (" + std::to_wstring(hist.size()) +
                      (hist.size() == 1 ? L" prior submission" : L" prior submissions") +
                      L", needs " + std::to_wstring(STYLE_MIN_HISTORY) + L")";
        return;
    }

    // Per-feature baseline. Inline-comment ratio only counts
    // submissions that had comments at all (raw[4] > 0) — otherwise
    // its 0 means "undefined", not "never inline".
    bool   judged[BLOCK_STYLE_COUNT];
    double mean[BLOCK_STYLE_COUNT], limit[BLOCK_STYLE_COUNT];
    for (int k = 0; k < BLOCK_STYLE_COUNT; ++k) {
        int f = BLOCK_STYLE_FEATURE[k];
        std::vector<double> vals;
        for (const auto* v : hist)
            if (f != 5 || (*v)[4] > 0.0) vals.push_back((*v)[f]);
        judged[k] = (int)vals.size() >= STYLE_MIN_HISTORY;
        if (!judged[k]) continue;
        double sum = 0.0;
        for (double x : vals) sum += x;
        mean[k] = sum / vals.size();
        double var = 0.0;
        for (double x : vals) var += (x - mean[k]) * (x - mean[k]);
        double sd = std::sqrt(var / (vals.size() - 1));
        limit[k] = std::max(STYLE_SPREAD_MULT * sd, STYLE_FLOOR[k]);
    }

    for (const BlockStyle& b : measureFunctionStyles(raw)) {
        if (b.startLine >= (int)s.lines.size()) break;
        std::wstring reason;
        int changed = 0;
        for (int k = 0; k < BLOCK_STYLE_COUNT; ++k) {
            if (!b.has[k] || !judged[k]) continue;
            if (std::fabs(b.value[k] - mean[k]) <= limit[k]) continue;
            ++changed;
            if (!reason.empty()) reason += L"  ·  ";
            reason += describeStyleChange(k, b.value[k], mean[k]);
        }
        if (changed < STYLE_MIN_CHANGED) continue;
        int end = std::min(b.endLine, (int)s.lines.size() - 1);
        for (int li = b.startLine; li <= end; ++li) s.highlight[li] = 1;
        s.runStarts.push_back(b.startLine);
        s.flagReason[b.startLine] = reason;
        ++s.flagCount;
    }
}

// Opens MODAL_SOURCE_VIEW in compare mode for a flagged pair.
// Gold (line matches) needs both files — either one missing turns it
// off for both sides, with a note. Red (style changes) judges each file
// against its OWN student's profile, independently — a missing file
// only loses its own side's red.
void openSourceCompareModal(const std::string& pathA, const std::wstring& labelA,
                             const std::string& studentA,
                             const std::string& pathB, const std::wstring& labelB,
                             const std::string& studentB,
                             bool showA)
{
    SourceViewSide sa, sb;
    std::string rawA, rawB;
    loadSourceViewSide(labelA, s2w(pathA), pathA, sa, &rawA);
    loadSourceViewSide(labelB, s2w(pathB), pathB, sb, &rawB);

    AppModal m;
    m.kind          = MODAL_SOURCE_VIEW;
    m.sourceCompare = true;

    if (sa.missing || sb.missing) {
        sa.partner.assign(sa.lines.size(), -1); sa.matchHighlight.assign(sa.lines.size(), 0);
        sb.partner.assign(sb.lines.size(), -1); sb.matchHighlight.assign(sb.lines.size(), 0);
        m.sourceMatchNote = std::wstring(L"match highlighting unavailable — File ") +
                            (sa.missing ? L"A" : L"B") + L" couldn't be read";
    } else {
        m.sourceMatchCount = computeSourceLineMatches(rawA, sa, rawB, sb, m.sourceMatchNote);
        if (m.sourceMatchCount < 0) m.sourceMatchCount = 0;
    }

    if (sa.missing) sa.styleNote = L"file couldn't be read";
    else            computeStyleChanges(rawA, pathA, studentA, sa);
    if (sb.missing) sb.styleNote = L"file couldn't be read";
    else            computeStyleChanges(rawB, pathB, studentB, sb);

    SourceViewSide& shown = showA ? sa : sb;
    m.sourceSide       = showA ? 0 : 1;
    m.title            = shown.title;
    m.sourceLines      = std::move(shown.lines);
    m.sourceSubtitle   = shown.subtitle;
    m.sourceMissing    = shown.missing;
    m.sourceTruncated  = shown.truncated;
    m.sourcePartner        = std::move(shown.partner);
    m.sourceMatchHighlight = std::move(shown.matchHighlight);
    m.sourceMatchRunStarts = std::move(shown.matchRunStarts);
    m.sourceHighlight  = std::move(shown.highlight);
    m.sourceRunStarts  = std::move(shown.runStarts);
    m.sourceFlagReason = std::move(shown.flagReason);
    m.sourceFlagCount  = shown.flagCount;
    m.sourceStyleNote  = shown.styleNote;
    m.sourceOther      = std::move(showA ? sb : sa);

    g_appModal = std::move(m);
    g_modalFocusOnPrimary = true;
    if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
}

// Shared geometry for the MODAL_SOURCE_VIEW card — computed the same
// way by drawSourceViewModal (to draw/clip) and by ContentProc's
// WM_MOUSEWHEEL/WM_LBUTTONDOWN handlers (to clamp scroll / hit-test
// close), so the three stay in sync without duplicating magic numbers.
// Fixed size, not text-measured — the whole reason this is a distinct
// kind from MODAL_OK/MODAL_CONFIRM (see the ModalKind enum's comment).
struct SourceViewGeom { RECT modal, header, toolbar, counts, body, closeBtn, legendBtn; int lineH; };
static SourceViewGeom sourceViewGeometry(const RECT& viewport) {
    SourceViewGeom g;
    int modalW = S(860);
    if (modalW > viewport.right - S(20)) modalW = viewport.right - S(20);
    int modalH = viewport.bottom - S(60);
    int minH = S(300);
    if (modalH < minH) modalH = (viewport.bottom > minH) ? minH : viewport.bottom - S(10);

    int mx = (viewport.right - modalW) / 2;
    if (mx < S(10)) mx = S(10);
    int my = (viewport.bottom - modalH) / 2;
    if (my < S(10)) my = S(10);
    g.modal = {mx, my, mx + modalW, my + modalH};

    // Compare mode adds a toolbar row (File A/B tabs, prev/next match)
    // under the title/subtitle, then a row for the match/style counts —
    // they used to share the toolbar row, where the two file tabs left
    // them too little width and they ellipsized ("fits this st…"). The
    // footer also gets a second line in compare mode (color key, then
    // line count + shortcuts). Single-file mode's layout is unchanged.
    int tabRowH  = g_appModal.sourceCompare ? S(36) : 0;
    int countsH  = g_appModal.sourceCompare ? S(24) : 0;
    int toolbarH = tabRowH + countsH;
    int headerH = S(60) + toolbarH;
    int footerH = g_appModal.sourceCompare ? S(42) : S(24);
    g.header = {g.modal.left, g.modal.top, g.modal.right, g.modal.top + headerH};
    g.toolbar = {g.modal.left + S(20), g.modal.top + S(60),
                 g.modal.right - S(20), g.modal.top + S(60) + tabRowH - S(8)};
    g.counts  = {g.modal.left + S(20), g.toolbar.bottom + S(2),
                 g.modal.right - S(20), g.toolbar.bottom + S(2) + countsH - S(4)};
    g.body   = {g.modal.left + S(20), g.header.bottom + S(8),
                g.modal.right - S(20), g.modal.bottom - footerH};

    int closeSz = S(28);
    g.closeBtn = {g.modal.right - S(16) - closeSz, g.modal.top + S(16),
                  g.modal.right - S(16), g.modal.top + S(16) + closeSz};
    // "How to read" sits left of close — compare mode only, since the
    // highlights it explains only exist there.
    g.legendBtn = g_appModal.sourceCompare
        ? RECT{g.closeBtn.left - S(8) - S(128), g.closeBtn.top, g.closeBtn.left - S(8), g.closeBtn.bottom}
        : RECT{};

    g.lineH = S(18);
    return g;
}

// MODAL_SOURCE_VIEW smooth scroll: eases sourceScrollY toward `target`
// (already clamped by the caller) over SOURCE_SCROLL_ANIM_MS with the
// shared easeOutCubic, driven by TIMER_SOURCE_SCROLL. Retargeting while
// in flight restarts from the current position, so repeated wheel
// notches/keypresses glide instead of stuttering. Callers that stack
// input (wheel, arrows, Page) should step from sourceScrollBase().
static void animateSourceScrollTo(HWND hwnd, int target) {
    AppModal& m = g_appModal;
    if (target == (m.sourceScrollAnimating ? m.sourceScrollTarget : m.sourceScrollY)) return;
    m.sourceScrollFrom      = m.sourceScrollY;
    m.sourceScrollTarget    = target;
    m.sourceScrollStartTick = GetTickCount();
    m.sourceScrollAnimating = true;
    SetTimer(hwnd, TIMER_SOURCE_SCROLL, 15, nullptr);
}

// Where the next scroll step should start from: the in-flight target
// if an animation is running (so steps accumulate), else the position.
static int sourceScrollBase() {
    return g_appModal.sourceScrollAnimating ? g_appModal.sourceScrollTarget
                                            : g_appModal.sourceScrollY;
}

// Compare mode: switch the shown file to `side` (0 = A, 1 = B) by
// swapping AppModal's active source* fields with sourceOther. If a
// matched line is on screen, the newly shown file is scrolled so that
// line's partner lands at the same height — so flipping A/B keeps the
// evidence lined up instead of jumping to wherever that file was left.
void swapSourceViewSide(HWND hwnd, int side) {
    AppModal& m = g_appModal;
    if (m.kind != MODAL_SOURCE_VIEW || !m.sourceCompare || side == m.sourceSide) return;
    // Land any in-flight glide first, so the parked file keeps the
    // position it was headed to and alignment uses the settled view.
    if (m.sourceScrollAnimating) m.sourceScrollY = m.sourceScrollTarget;

    RECT vp; GetClientRect(hwnd, &vp);
    SourceViewGeom geom = sourceViewGeometry(vp);
    int viewportH = geom.body.bottom - geom.body.top;
    int alignedScroll = -1;
    for (int i = m.sourceScrollY / geom.lineH;
         i < (int)m.sourcePartner.size() && i * geom.lineH < m.sourceScrollY + viewportH; ++i) {
        if (m.sourcePartner[i] >= 0) {
            alignedScroll = m.sourcePartner[i] * geom.lineH - (i * geom.lineH - m.sourceScrollY);
            if (alignedScroll < 0) alignedScroll = 0;
            break;
        }
    }

    SourceViewSide& o = m.sourceOther;
    std::swap(m.title,            o.title);
    std::swap(m.sourceLines,      o.lines);
    std::swap(m.sourceSubtitle,   o.subtitle);
    std::swap(m.sourceMissing,    o.missing);
    std::swap(m.sourceTruncated,  o.truncated);
    std::swap(m.sourceScrollY,    o.scrollY);
    std::swap(m.sourcePartner,        o.partner);
    std::swap(m.sourceMatchHighlight, o.matchHighlight);
    std::swap(m.sourceMatchRunStarts, o.matchRunStarts);
    std::swap(m.sourceHighlight,  o.highlight);
    std::swap(m.sourceRunStarts,  o.runStarts);
    std::swap(m.sourceFlagReason, o.flagReason);
    std::swap(m.sourceFlagCount,  o.flagCount);
    std::swap(m.sourceStyleNote,  o.styleNote);
    m.sourceSide = side;
    m.sourceScrollAnimating = false; // switching files snaps; the old glide belonged to the other file
    // A matched line on screen: land its partner at the same height, so
    // flipping A/B keeps the evidence lined up. Otherwise each file keeps
    // its own scroll position (drawSourceViewModal clamps either way).
    if (alignedScroll >= 0) m.sourceScrollY = alignedScroll;
    InvalidateRect(hwnd, nullptr, FALSE);
}

// Compare mode: every Prev/Next stop in the shown file — the start of
// each gold matched run and each red flagged function, sorted, no dups.
static std::vector<int> sourceJumpStops() {
    std::vector<int> stops = g_appModal.sourceMatchRunStarts;
    stops.insert(stops.end(), g_appModal.sourceRunStarts.begin(), g_appModal.sourceRunStarts.end());
    std::sort(stops.begin(), stops.end());
    stops.erase(std::unique(stops.begin(), stops.end()), stops.end());
    return stops;
}

// Compare mode: scroll to the next (dir > 0) or previous highlighted
// section (gold or red), leaving a few lines of context above it.
static void jumpSourceViewMatch(HWND hwnd, int dir) {
    AppModal& m = g_appModal;
    if (m.kind != MODAL_SOURCE_VIEW) return;
    std::vector<int> stops = sourceJumpStops();
    if (stops.empty()) return;

    RECT vp; GetClientRect(hwnd, &vp);
    SourceViewGeom geom = sourceViewGeometry(vp);
    const int context = 3;
    int topLine = sourceScrollBase() / geom.lineH;
    int target = -1;
    if (dir > 0) {
        for (int li : stops)
            if (li - context > topLine) { target = li; break; }
    } else {
        for (auto it = stops.rbegin(); it != stops.rend(); ++it)
            if (*it - context < topLine) { target = *it; break; }
    }
    if (target < 0) return;
    int maxScroll = (int)m.sourceLines.size() * geom.lineH - (geom.body.bottom - geom.body.top);
    if (maxScroll < 0) maxScroll = 0;
    int pos = (target - context) * geom.lineH;
    if (pos > maxScroll) pos = maxScroll;
    if (pos < 0) pos = 0;
    animateSourceScrollTo(hwnd, pos);
}

// MODAL_SOURCE_VIEW's own draw path — fixed-size card, clipped and
// independently scrolled body (g_appModal.sourceScrollY, not the
// page's g_scrollY), monospace text, no syntax highlighting (see
// CLAUDE.md's note on why: tokenizer.cpp's Token has no source
// offset, so highlighting here would need a second, separate lexer).
// Fills `r` with SOURCE_MATCH_FILL at SOURCE_MATCH_ALPHA over whatever
// is already drawn there (GDI's FillRect has no alpha). One cached 1x1
// source bitmap, stretched by AlphaBlend — allocated once, not per line.
static void fillSourceMatchWash(HDC hdc, const RECT& r) {
    static HDC s_dc = nullptr;
    if (!s_dc) {
        s_dc = CreateCompatibleDC(nullptr);
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth       = 1;
        bi.bmiHeader.biHeight      = 1;
        bi.bmiHeader.biPlanes      = 1;
        bi.bmiHeader.biBitCount    = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        HBITMAP bmp = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (bmp && bits)
            *(DWORD*)bits = (GetRValue(SOURCE_MATCH_FILL) << 16) |
                            (GetGValue(SOURCE_MATCH_FILL) << 8) | GetBValue(SOURCE_MATCH_FILL);
        SelectObject(s_dc, bmp);
    }
    BLENDFUNCTION bf = {AC_SRC_OVER, 0, SOURCE_MATCH_ALPHA, 0};
    AlphaBlend(hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, s_dc, 0, 0, 1, 1, bf);
}

// Compare mode's "How to read" panel — drawn over the body when
// g_appModal.sourceLegendOpen. Explains gold and red in instructor
// terms. The numbers come from the same constants the highlights use
// (SOURCE_MATCH_MIN_TOKENS, STYLE_*), so the text can't drift from the
// rule. Any click or Esc closes it (ContentProc).
static void drawSourceViewLegend(HDC hdc, const SourceViewGeom& geom) {
    RECT panel = {geom.modal.left + S(12), geom.header.bottom + S(6),
                  geom.modal.right - S(12), geom.modal.bottom - S(10)};
    drawCard(hdc, panel, GRAY_850, UL_GOLD, 10);
    HRGN clip = CreateRectRgn(panel.left, panel.top, panel.right, panel.bottom);
    SelectClipRgn(hdc, clip);

    RECT in = {panel.left + S(22), panel.top + S(16), panel.right - S(22), panel.bottom - S(12)};
    SetBkMode(hdc, TRANSPARENT);

    SelectObject(hdc, g_hFontH2);
    SetTextColor(hdc, UL_GOLD);
    RECT tr = {in.left, in.top, in.right, in.top + S(26)};
    DrawTextW(hdc, L"How to read the highlights", -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, g_hFontLabel);
    SetTextColor(hdc, GRAY_500);
    DrawTextW(hdc, L"CLICK ANYWHERE OR PRESS ESC TO CLOSE", -1, &tr,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    // The caveat goes first: on a short screen the panel clips from the
    // bottom, and this is the one line that must never be cut.
    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, GRAY_400);
    RECT cr ={in.left, tr.bottom + S(4), in.right, tr.bottom + S(26)};
    DrawTextW(hdc, L"Both colors show where to look, not proof of copying. They're display-only "
                   L"and never change any score.", -1, &cr,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    tr.bottom = cr.bottom;

    // One colored section: swatch + eyebrow heading, then wrapped
    // paragraphs. Returns the y just below it.
    auto section = [&](int x, int y, int w, COLORREF color, const wchar_t* heading,
                       const std::vector<std::wstring>& paras) {
        RECT sw = {x, y + S(2), x + S(12), y + S(14)};
        HBRUSH b = CreateSolidBrush(color);
        FillRect(hdc, &sw, b);
        DeleteObject(b);
        SelectObject(hdc, g_hFontLabel);
        SetTextColor(hdc, color);
        RECT hr = {x + S(20), y, x + w, y + S(16)};
        DrawTextW(hdc, heading, -1, &hr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        y += S(22);
        SelectObject(hdc, g_hFontSmall);
        bool prevSub = false;
        for (const auto& p : paras) {
            bool sub = p.size() > 1 && p[0] == L' '; // indented list item
            if (prevSub && !sub) y += S(6);          // end of a list: same gap as a paragraph
            prevSub = sub;
            int px = x + (sub ? S(18) : 0);
            SetTextColor(hdc, GRAY_200);
            RECT m = {px, y, x + w, y};
            DrawTextW(hdc, p.c_str(), -1, &m, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
            RECT d = {px, y, x + w, m.bottom};
            DrawTextW(hdc, p.c_str(), -1, &d, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
            y = m.bottom + (sub ? S(2) : S(8));
        }
        return y;
    };

    int top = tr.bottom + S(12);
    int gap = S(28);
    int colW = (in.right - in.left - gap) / 2;
    std::wstring minTok = std::to_wstring(SOURCE_MATCH_MIN_TOKENS);
    int leftEnd = section(in.left, top, colW, SOURCE_MATCH_BAR, L"GOLD  —  ALSO IN THE OTHER FILE", {
        L"A line is gold when the same line appears in the other student's file, in the same order.",
        L"Variable and function names are ignored, so renaming them doesn't hide a copied line.",
        L"Lines with fewer than " + minTok + L" tokens (like  }  or  return 0; ) are never marked "
        L"— everyone writes those.",
        L"Gold shows where the two files overlap. It can't tell you who copied from whom.",
    });

    std::wstring minChanged = std::to_wstring(STYLE_MIN_CHANGED);
    std::wstring spread     = std::to_wstring((int)STYLE_SPREAD_MULT);
    std::wstring lineFloor  = std::to_wstring((int)STYLE_FLOOR[3]);
    std::wstring minHist    = std::to_wstring(STYLE_MIN_HISTORY);
    int rightEnd = section(in.left + colW + gap, top, colW, SOURCE_STYLE_BAR,
                           L"RED  —  DOESN'T LOOK LIKE THIS STUDENT", {
        L"A function is red when its formatting doesn't match this student's own "
        L"activities and quizzes. CALSS compares 4 habits:",
        L" 1.  Brace placement — {  on the same line or the next line",
        L" 2.  Operator spacing — a = b + c  vs  a=b+c",
        L" 3.  Inline comments — comments at the end of a code line",
        L" 4.  Line length — average characters per line",
        L"A habit has changed only if it's past " + spread + L"x the student's normal "
        L"variation and a minimum (e.g. " + lineFloor + L" characters for line length).",
        L"Red needs " + minChanged + L"+ habits to change in the SAME function — one change "
        L"alone is common in honest work.",
        L"Naming isn't checked: activities often hand out variable names. Needs " + minHist +
        L"+ earlier submissions.",
    });

    int y = std::max(leftEnd, rightEnd) + S(6);
    HPEN pen = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, in.left, y, nullptr);
    LineTo(hdc, in.right, y);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
    section(in.left, y + S(12), in.right - in.left, UL_GOLD, L"READING THEM TOGETHER", {
        L"Gold + red: code that matches the other file AND doesn't look like its owner's "
        L"style — the strongest reason to look closer.",
        L"Gold only: shared code in the student's own style — retyped from a copy, or simply "
        L"a common solution to the same problem.",
        L"Red only: the student's formatting changed, but the code doesn't match the other file.",
        L"No red doesn't clear a file: a copy that differs from the student in only one habit "
        L"isn't marked.",
    });

    SelectClipRgn(hdc, nullptr);
    DeleteObject(clip);
}

void drawSourceViewModal(HDC hdcScreen, const RECT& viewport) {
    static HBRUSH s_styleBgBrush  = CreateSolidBrush(SOURCE_STYLE_BG);
    static HBRUSH s_styleBarBrush = CreateSolidBrush(SOURCE_STYLE_BAR);
    static HBRUSH s_matchBarBrush = CreateSolidBrush(SOURCE_MATCH_BAR);
    SourceViewGeom geom = sourceViewGeometry(viewport);
    drawCard(hdcScreen, geom.modal, GRAY_900, GRAY_700, 14, UL_GOLD);

    SetBkMode(hdcScreen, TRANSPARENT);

    HFONT oldFont = (HFONT)SelectObject(hdcScreen, g_hFontH2);
    SetTextColor(hdcScreen, UL_GOLD);
    int headerRight = (g_appModal.sourceCompare ? geom.legendBtn.left : geom.closeBtn.left) - S(10);
    RECT titleR = {geom.header.left + S(20), geom.header.top + S(10),
                    headerRight, geom.header.top + S(32)};
    DrawTextW(hdcScreen, g_appModal.title.c_str(), -1, &titleR,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SelectObject(hdcScreen, g_hFontSmall);
    SetTextColor(hdcScreen, g_appModal.sourceMissing ? COLOR_WARNING : GRAY_400);
    // Bottom pinned to the fixed 60px title block, not header.bottom —
    // in compare mode the header also holds the toolbar row, and a
    // DT_VCENTER rect spanning both drew the path over the File A/B tabs.
    RECT subR = {geom.header.left + S(20), geom.header.top + S(34),
                  headerRight, geom.header.top + S(56)};
    DrawTextW(hdcScreen, g_appModal.sourceSubtitle.c_str(), -1, &subR,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    HPEN divPen = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldPen = (HPEN)SelectObject(hdcScreen, divPen);
    MoveToEx(hdcScreen, geom.header.left, geom.header.bottom, nullptr);
    LineTo(hdcScreen, geom.header.right, geom.header.bottom);
    SelectObject(hdcScreen, oldPen);
    DeleteObject(divPen);

    // Close button
    drawCard(hdcScreen, geom.closeBtn, GRAY_800, GRAY_700, 6);
    SelectObject(hdcScreen, g_hFontBodyNew);
    SetTextColor(hdcScreen, GRAY_200);
    DrawTextW(hdcScreen, L"✕", -1, &geom.closeBtn,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    g_sourceViewCloseBtnRect = geom.closeBtn;

    // "How to read" button (compare mode) — gold-bordered while its
    // panel is open, so it reads as the toggle that closes it again.
    g_sourceViewLegendBtnRect = geom.legendBtn;
    if (g_appModal.sourceCompare) {
        bool open = g_appModal.sourceLegendOpen;
        drawCard(hdcScreen, geom.legendBtn, open ? MAROON_800 : GRAY_800,
                 open ? UL_GOLD : GRAY_700, 6);
        SelectObject(hdcScreen, g_hFontLabel);
        SetTextColor(hdcScreen, open ? UL_GOLD : GRAY_200);
        DrawTextW(hdcScreen, L"?  How to read", -1, &geom.legendBtn,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // Compare-mode toolbar: File A / File B tabs on the left, match and
    // style counts + prev/next buttons on the right. Hit rects are
    // recorded here, same as the close button above.
    g_sourceViewTabRects[0] = g_sourceViewTabRects[1] = RECT{};
    g_sourceViewPrevRect = g_sourceViewNextRect = RECT{};
    if (g_appModal.sourceCompare) {
        SelectObject(hdcScreen, g_hFontSmall);
        int tx = geom.toolbar.left;
        for (int side = 0; side < 2; ++side) {
            bool active = (side == g_appModal.sourceSide);
            const std::wstring& name = active ? g_appModal.title : g_appModal.sourceOther.title;
            std::wstring label = std::wstring(side == 0 ? L"File A  ·  " : L"File B  ·  ") + name;
            RECT mr = {0, 0, 0, 0};
            DrawTextW(hdcScreen, label.c_str(), -1, &mr, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
            int w = mr.right - mr.left + S(24);
            if (w > S(260)) w = S(260);
            RECT tr = {tx, geom.toolbar.top, tx + w, geom.toolbar.bottom};
            drawCard(hdcScreen, tr, active ? MAROON_800 : GRAY_800,
                     active ? UL_GOLD : GRAY_700, 6);
            SetTextColor(hdcScreen, active ? UL_GOLD : GRAY_400);
            RECT textR = {tr.left + S(12), tr.top, tr.right - S(12), tr.bottom};
            DrawTextW(hdcScreen, label.c_str(), -1, &textR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            g_sourceViewTabRects[side] = tr;
            tx = tr.right + S(8);
        }

        bool hasMatch = !g_appModal.sourceMatchRunStarts.empty();
        bool hasStyle = !g_appModal.sourceRunStarts.empty();
        bool canJump  = hasMatch || hasStyle;
        int bw = S(88);
        RECT nextR = {geom.toolbar.right - bw, geom.toolbar.top,
                      geom.toolbar.right, geom.toolbar.bottom};
        RECT prevR = {nextR.left - S(8) - bw, geom.toolbar.top,
                      nextR.left - S(8), geom.toolbar.bottom};
        const wchar_t* labels[2] = {L"▲  Prev", L"▼  Next"};
        RECT* rs[2] = {&prevR, &nextR};
        COLORREF jumpBorder = hasMatch ? SOURCE_MATCH_BAR : hasStyle ? SOURCE_STYLE_BAR : GRAY_700;
        for (int k = 0; k < 2; ++k) {
            drawCard(hdcScreen, *rs[k], GRAY_800, jumpBorder, 6);
            SetTextColor(hdcScreen, canJump ? GRAY_200 : GRAY_500);
            DrawTextW(hdcScreen, labels[k], -1, rs[k], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        if (canJump) { g_sourceViewPrevRect = prevR; g_sourceViewNextRect = nextR; }

        // Two counts, each in its own highlight's color: "N matching
        // lines" (gold) then the style verdict (red), left-aligned on
        // their own row under the tabs (geom.counts) so neither has to
        // ellipsize.
        int mc = g_appModal.sourceMatchCount;
        std::wstring matchText = !g_appModal.sourceMatchNote.empty()
            ? std::wstring(L"no match highlighting")
            : std::to_wstring(mc) + (mc == 1 ? L" matching line" : L" matching lines");
        int fc = g_appModal.sourceFlagCount;
        std::wstring styleText = !g_appModal.sourceStyleNote.empty()
            ? std::wstring(L"no style judgment")
            : fc == 0 ? std::wstring(L"fits this student's usual style")
            : std::to_wstring(fc) + (fc == 1 ? L" section breaks" : L" sections break") +
              L" this student's pattern";
        styleText = L"  ·  " + styleText;
        RECT mm = {0, 0, 0, 0};
        DrawTextW(hdcScreen, matchText.c_str(), -1, &mm, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
        RECT mr2 = geom.counts;
        mr2.right = mr2.left + (mm.right - mm.left);
        if (mr2.right > geom.counts.right) mr2.right = geom.counts.right;
        SetTextColor(hdcScreen, mc > 0 ? SOURCE_MATCH_BAR : GRAY_500);
        DrawTextW(hdcScreen, matchText.c_str(), -1, &mr2,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        RECT sr = {mr2.right, geom.counts.top, geom.counts.right, geom.counts.bottom};
        SetTextColor(hdcScreen, fc > 0 ? SOURCE_STYLE_BAR : GRAY_500);
        DrawTextW(hdcScreen, styleText.c_str(), -1, &sr,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    // Body — clipped to its rect so long lines/overflow can't bleed
    // into the header or past the card, then either the missing-file
    // fallback message or the scrolled, monospace source text.
    HRGN clipRgn = CreateRectRgn(geom.body.left, geom.body.top,
                                  geom.body.right, geom.body.bottom);
    SelectClipRgn(hdcScreen, clipRgn);

    if (g_appModal.sourceMissing) {
        SelectObject(hdcScreen, g_hFontBodyNew);
        SetTextColor(hdcScreen, COLOR_WARNING);
        RECT msgR = geom.body;
        DrawTextW(hdcScreen, g_appModal.sourceSubtitle.c_str(), -1, &msgR,
                  DT_LEFT | DT_WORDBREAK);
    } else {
        SelectObject(hdcScreen, g_hFontMono);
        SetTextColor(hdcScreen, GRAY_200);

        int contentH = (int)g_appModal.sourceLines.size() * geom.lineH;
        int viewportH = geom.body.bottom - geom.body.top;
        int maxScroll = contentH - viewportH;
        if (maxScroll < 0) maxScroll = 0;
        if (g_appModal.sourceScrollY > maxScroll) g_appModal.sourceScrollY = maxScroll;
        if (g_appModal.sourceScrollY < 0) g_appModal.sourceScrollY = 0;

        int firstLine = g_appModal.sourceScrollY / geom.lineH;
        int lineY = geom.body.top - (g_appModal.sourceScrollY % geom.lineH);
        for (size_t i = (size_t)firstLine;
             i < g_appModal.sourceLines.size() && lineY < geom.body.bottom;
             ++i, lineY += geom.lineH)
        {
            RECT lr = {geom.body.left, lineY, geom.body.right, lineY + geom.lineH};
            bool hl   = i < g_appModal.sourceHighlight.size() && g_appModal.sourceHighlight[i];
            bool gold = i < g_appModal.sourceMatchHighlight.size() && g_appModal.sourceMatchHighlight[i];
            // Compare mode: red fill on functions that break this
            // student's own pattern (see computeStyleChanges), then a
            // translucent gold wash on lines matching the other file
            // (see computeSourceLineMatches) — so a copied line inside a
            // red function shows both. Bars sit side by side, red outer.
            RECT bg = {lr.left - S(8), lr.top, lr.right - S(10), lr.bottom};
            int barX = bg.left;
            if (hl) {
                FillRect(hdcScreen, &bg, s_styleBgBrush);
                RECT bar = {barX, bg.top, barX + S(3), bg.bottom};
                FillRect(hdcScreen, &bar, s_styleBarBrush);
                barX += S(3);
            }
            if (gold) {
                fillSourceMatchWash(hdcScreen, bg);
                RECT bar = {barX, bg.top, barX + S(3), bg.bottom};
                FillRect(hdcScreen, &bar, s_matchBarBrush);
            }
            if (hl || gold) SetTextColor(hdcScreen, hl ? SOURCE_STYLE_TEXT : SOURCE_MATCH_TEXT);
            DrawTextW(hdcScreen, g_appModal.sourceLines[i].c_str(), -1, &lr,
                      DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
            if (hl || gold) SetTextColor(hdcScreen, GRAY_200);

            // Reason label, right-aligned on a flagged function's first
            // line, on its own backing so it stays readable over code.
            if (hl && i < g_appModal.sourceFlagReason.size() &&
                !g_appModal.sourceFlagReason[i].empty()) {
                const std::wstring& why = g_appModal.sourceFlagReason[i];
                HFONT prevF = (HFONT)SelectObject(hdcScreen, g_hFontSmall);
                RECT mr = {0, 0, 0, 0};
                DrawTextW(hdcScreen, why.c_str(), -1, &mr, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
                int maxW = (lr.right - lr.left) * 3 / 5;
                int w = mr.right - mr.left + S(16);
                if (w > maxW) w = maxW;
                RECT pill = {lr.right - S(14) - w, lr.top, lr.right - S(14), lr.bottom};
                FillRect(hdcScreen, &pill, s_styleBarBrush);
                SetTextColor(hdcScreen, RGB(255, 255, 255));
                RECT tr = {pill.left + S(8), pill.top, pill.right - S(8), pill.bottom};
                DrawTextW(hdcScreen, why.c_str(), -1, &tr,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                SetTextColor(hdcScreen, GRAY_200);
                SelectObject(hdcScreen, prevF);
            }
        }

        // Simple scrollbar thumb on the body's right edge — visual
        // confirmation there's more content, mirroring (not reusing;
        // this HDC isn't an HWND) the native scrollbar on the main pane.
        if (maxScroll > 0) {
            int trackX = geom.body.right - S(6);
            // Compare mode: a tick in the track per gold matched run and
            // per red flagged function (red drawn last, on top), so where
            // the evidence sits in a long file is visible at a glance.
            if (contentH > 0) {
                auto ticks = [&](const std::vector<int>& starts, HBRUSH br) {
                    for (int li : starts) {
                        int ty = geom.body.top + (int)((double)li * geom.lineH * viewportH / contentH);
                        RECT tick = {trackX - S(2), ty, trackX + S(6), ty + S(2)};
                        FillRect(hdcScreen, &tick, br);
                    }
                };
                ticks(g_appModal.sourceMatchRunStarts, s_matchBarBrush);
                ticks(g_appModal.sourceRunStarts, s_styleBarBrush);
            }
            int thumbH = (contentH > 0)
                ? (int)((double)viewportH * viewportH / contentH) : viewportH;
            if (thumbH < S(24)) thumbH = S(24);
            if (thumbH > viewportH) thumbH = viewportH;
            int thumbY = geom.body.top + (int)((double)(viewportH - thumbH) *
                          g_appModal.sourceScrollY / maxScroll);
            RECT thumb = {trackX, thumbY, trackX + S(4), thumbY + thumbH};
            HBRUSH tb = CreateSolidBrush(GRAY_500);
            FillRect(hdcScreen, &thumb, tb);
            DeleteObject(tb);
        }
    }

    SelectClipRgn(hdcScreen, nullptr);
    DeleteObject(clipRgn);

    // Footer — line count / truncated notice, skipped when the file's
    // missing (nothing to count).
    if (!g_appModal.sourceMissing) {
        SelectObject(hdcScreen, g_hFontSmall);
        SetTextColor(hdcScreen, GRAY_500);
        std::wstring footer = std::to_wstring(g_appModal.sourceLines.size()) + L" lines";
        if (g_appModal.sourceTruncated)
            footer += L"  ·  truncated — file exceeds the display limit, "
                      L"showing a prefix only";
        if (g_appModal.sourceCompare) {
            // Two lines (footerH is taller in compare mode): the color
            // key first, then line count + keyboard shortcuts — one line
            // held all of it and ran past the modal edge ("N / P: ne…").
            std::wstring key = g_appModal.sourceMatchNote.empty()
                ? L"gold = also in the other file (renamed identifiers count)"
                : g_appModal.sourceMatchNote;
            key += g_appModal.sourceStyleNote.empty()
                ? L"  ·  red = breaks this student's own formatting pattern"
                : L"  ·  " + g_appModal.sourceStyleNote;
            footer += L"  ·  Tab / A / B: switch file  ·  N / P: next / prev highlight  ·  H: how to read";
            int top = geom.body.bottom + S(3);
            RECT keyR  = {geom.modal.left + S(20), top,
                          geom.modal.right - S(20), top + S(18)};
            RECT footR = {geom.modal.left + S(20), top + S(18),
                          geom.modal.right - S(20), top + S(36)};
            DrawTextW(hdcScreen, key.c_str(), -1, &keyR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            DrawTextW(hdcScreen, footer.c_str(), -1, &footR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        } else {
            RECT footR = {geom.modal.left + S(20), geom.body.bottom + S(2),
                           geom.modal.right - S(20), geom.modal.bottom - S(2)};
            DrawTextW(hdcScreen, footer.c_str(), -1, &footR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
    }

    if (g_appModal.sourceCompare && g_appModal.sourceLegendOpen)
        drawSourceViewLegend(hdcScreen, geom);

    SelectObject(hdcScreen, oldFont);
}

// ─────────────────────────────────────────────────────────────
// Input handlers — moved out of ContentProc's WM_KEYDOWN /
// WM_MOUSEWHEEL / WM_TIMER / WM_LBUTTONDOWN modal gates (2026-10-02),
// logic unchanged. ContentProc calls each only while MODAL_SOURCE_VIEW
// is the open modal.
// ─────────────────────────────────────────────────────────────

// Returns true if the key was consumed. Esc that should close the
// modal itself is left to ContentProc's generic modal handling.
bool sourceViewHandleKey(HWND hwnd, WPARAM wParam) {
    // Compare mode: A/B pick the file, N/P jump to the next/previous
    // gold or red highlight (Tab also toggles files — see the main
    // message loop).
    if (g_appModal.sourceCompare) {
        if (wParam == 'A') { swapSourceViewSide(hwnd, 0); return true; }
        if (wParam == 'B') { swapSourceViewSide(hwnd, 1); return true; }
        if (wParam == 'N') { jumpSourceViewMatch(hwnd, +1); return true; }
        if (wParam == 'P') { jumpSourceViewMatch(hwnd, -1); return true; }
        // H toggles "How to read"; Esc closes it before the modal.
        if (wParam == 'H' || (wParam == VK_ESCAPE && g_appModal.sourceLegendOpen)) {
            g_appModal.sourceLegendOpen = !g_appModal.sourceLegendOpen;
            InvalidateRect(hwnd, nullptr, FALSE);
            return true;
        }
    }
    // Scroll keys: arrows/Page/Home/End move sourceScrollY instead of
    // falling through to page-scroll or being swallowed.
    if (wParam == VK_DOWN || wParam == VK_UP || wParam == VK_NEXT ||
        wParam == VK_PRIOR || wParam == VK_HOME || wParam == VK_END)
    {
        RECT vp; GetClientRect(hwnd, &vp);
        SourceViewGeom geom = sourceViewGeometry(vp);
        int viewportH = geom.body.bottom - geom.body.top;
        int contentH = (int)g_appModal.sourceLines.size() * geom.lineH;
        int maxScroll = contentH - viewportH;
        if (maxScroll < 0) maxScroll = 0;

        int pos = sourceScrollBase();
        if (wParam == VK_DOWN)      pos += geom.lineH * 3;
        else if (wParam == VK_UP)    pos -= geom.lineH * 3;
        else if (wParam == VK_NEXT)  pos += viewportH;
        else if (wParam == VK_PRIOR) pos -= viewportH;
        else if (wParam == VK_HOME)  pos = 0;
        else if (wParam == VK_END)   pos = maxScroll;
        if (pos < 0) pos = 0;
        if (pos > maxScroll) pos = maxScroll;

        animateSourceScrollTo(hwnd, pos);
        return true;
    }
    return false;
}

// The wheel scrolls the modal's own sourceScrollY, not the page behind it.
void sourceViewHandleWheel(HWND hwnd, int delta) {
    RECT vp; GetClientRect(hwnd, &vp);
    SourceViewGeom geom = sourceViewGeometry(vp);
    int viewportH = geom.body.bottom - geom.body.top;
    int contentH = (int)g_appModal.sourceLines.size() * geom.lineH;
    int maxScroll = contentH - viewportH;
    if (maxScroll < 0) maxScroll = 0;

    int scrollAmount = (delta / WHEEL_DELTA) * 60;
    int newPos = sourceScrollBase() - scrollAmount;
    if (newPos < 0) newPos = 0;
    if (newPos > maxScroll) newPos = maxScroll;

    animateSourceScrollTo(hwnd, newPos);
}

// TIMER_SOURCE_SCROLL tick — see animateSourceScrollTo. Also
// self-stops if the modal was dismissed mid-glide.
void sourceViewScrollTick(HWND hwnd) {
    AppModal& m = g_appModal;
    if (m.kind != MODAL_SOURCE_VIEW || !m.sourceScrollAnimating) {
        KillTimer(hwnd, TIMER_SOURCE_SCROLL);
        return;
    }
    float t = (float)(GetTickCount() - m.sourceScrollStartTick) / (float)SOURCE_SCROLL_ANIM_MS;
    if (t >= 1.0f) {
        m.sourceScrollY = m.sourceScrollTarget;
        m.sourceScrollAnimating = false;
        KillTimer(hwnd, TIMER_SOURCE_SCROLL);
    } else {
        m.sourceScrollY = m.sourceScrollFrom +
            (int)std::lround((m.sourceScrollTarget - m.sourceScrollFrom) * easeOutCubic(t));
    }
    InvalidateRect(hwnd, nullptr, FALSE);
}

// The modal owns all clicks while open: its close button, "How to
// read", the A/B file tabs and the prev/next jump buttons.
void sourceViewHandleClick(HWND hwnd, int mx, int my) {
    if (mx >= g_sourceViewCloseBtnRect.left && mx < g_sourceViewCloseBtnRect.right &&
        my >= g_sourceViewCloseBtnRect.top && my < g_sourceViewCloseBtnRect.bottom)
    {
        dismissAppModal(false);
        return;
    }
    auto inR = [&](const RECT& r) {
        return mx >= r.left && mx < r.right && my >= r.top && my < r.bottom;
    };
    // "How to read": its button toggles the panel; while
    // the panel is open, any other click just closes it.
    if (inR(g_sourceViewLegendBtnRect) || g_appModal.sourceLegendOpen) {
        g_appModal.sourceLegendOpen = !g_appModal.sourceLegendOpen &&
                                      inR(g_sourceViewLegendBtnRect);
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }
    if (inR(g_sourceViewTabRects[0]))      swapSourceViewSide(hwnd, 0);
    else if (inR(g_sourceViewTabRects[1])) swapSourceViewSide(hwnd, 1);
    else if (inR(g_sourceViewPrevRect))    jumpSourceViewMatch(hwnd, -1);
    else if (inR(g_sourceViewNextRect))    jumpSourceViewMatch(hwnd, +1);
}

#endif // _WIN32
