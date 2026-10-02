// ─────────────────────────────────────────────────────────────
// gui_flagged.cpp
// Flagged Pairs page — pair comparison cards (feature table,
// deviations block, authorship cards, AI evidence narrative),
// collapsed/expanded row rendering, and the severity/exact-duplicate
// filter chrome.
//
// Split out of gui.cpp unchanged (Checkpoint 2 of the ContentProc
// split — see CLAUDE.md). Three pieces:
//  1. The entire pre-existing "STAGE 5B — Flagged Pairs and All Pairs
//     Table" section verbatim (gui.cpp's own section-header comment
//     marked this exact boundary already) — 18 functions/constants
//     plus its entry point, drawFlaggedPairs.
//  2 & 3. Nine more private symbols that lived scattered outside that
//     contiguous block in gui.cpp (mostly up near
//     g_pairSeverityFilter/g_dnaCellHits/scoreColor) but are
//     exclusively used by code in this file. All found by compiling
//     this file standalone and resolving every "not declared in this
//     scope" error, not by re-reading the original audit — that
//     audit only covered the contiguous STAGE 5B block, so these were
//     easy to miss:
//       moved here (private, unchanged): SEVERITY_CHIP_BAR_HEIGHT,
//         PAIR_COLLAPSED_ROW_HEIGHT, pairExpandAnimProgress(),
//         showAllFeaturesKey(), dnaStylePair(), dnaDeviationFlags(),
//         dnaValuesFromPair(), drawProgressBar(), scoreColor()
//       promoted to gui_common.h instead (definitions stayed in
//         gui.cpp, since code there uses them too):
//         pairMatchesSeverityFilter(), g_expandedPairs,
//         g_pairSeverityFilter, g_showAllFeaturesKeys,
//         g_copyFindingsFeedbackPairIdx, g_copyFindingsFeedbackUntilTick
// Plus one new function, handleFlaggedPairsClick, consolidating
// ContentProc's WM_LBUTTONDOWN hit-testing for this page.
//
// Everything below other than drawFlaggedPairs and
// handleFlaggedPairsClick stays `static` (private to this file) —
// verified that none of it is used anywhere outside this file.
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"

#ifdef _WIN32

// Moved from gui.cpp (Checkpoint 2 of the ContentProc split) —
// private to this file's Flagged Pairs rendering, unlike their
// neighbors g_pairSeverityFilter/pairMatchesSeverityFilter/
// g_expandedPairs, which stayed in gui.cpp (extern) because code
// there also uses them.
static const int SEVERITY_CHIP_BAR_HEIGHT = 40;
static const int PAIR_COLLAPSED_ROW_HEIGHT = 96;

// Eased (ease-out cubic) 0..1 progress for a pair's in-flight
// expand/collapse animation. 1.0 if the pair isn't currently animating
// (i.e. fully settled at its current state).
static float pairExpandAnimProgress(int pairIdx) {
    auto it = g_pairAnimating.find(pairIdx);
    if (it == g_pairAnimating.end()) return 1.0f;
    DWORD elapsed = GetTickCount() - it->second.startTick;
    float t = (float)elapsed / (float)PAIR_EXPAND_ANIM_MS;
    if (t >= 1.0f) return 1.0f;
    return easeOutCubic(t);
}

// Key = pairIdx*2 + side, side 0 = studentA, 1 = studentB (spec §5.9,
// line 298: per pair, per side "Show all 14 features" disclosure).
static inline int showAllFeaturesKey(int pairIdx, int side) { return pairIdx * 2 + side; }

static DnaStripStyle dnaStylePair() {
    return { S(9), S(2), S(8), S(26), GRAY_400, RISK_HIGH };
}

// Builds the deviation flag vector for one side of a flagged pair,
// from that submission's per-feature match results.
static std::vector<bool> dnaDeviationFlags(const AuthorshipDisplay& s) {
    std::vector<bool> flags(14, false);
    int idx = 0;
    auto absorb = [&](const std::vector<StyleNoteDisplay>& v) {
        for (const auto& n : v) {
            if (idx < 14) flags[idx++] = !n.isMatch;
        }
    };
    absorb(s.lexicalFeatures);
    absorb(s.layoutFeatures);
    absorb(s.syntacticFeatures);
    return flags;
}

// Builds a normalized value vector for one side of a flagged pair
// from its per-feature submission values.
static std::vector<double> dnaValuesFromPair(const AuthorshipDisplay& s) {
    std::vector<double> vals;
    auto absorb = [&](const std::vector<StyleNoteDisplay>& v) {
        for (const auto& n : v) vals.push_back(n.submissionValue);
    };
    absorb(s.lexicalFeatures);
    absorb(s.layoutFeatures);
    absorb(s.syntacticFeatures);
    vals.resize(14, 0.0);
    return vals;
}

static void drawProgressBar(HDC hdc, int x, int y, int w, int h,
                              double pct, COLORREF fillColor)
{
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    float rad = h / 2.0f;

    // Track (rounded pill background)
    GraphicsPath* trackPath = makeRoundRectPath((float)x, (float)y, (float)w, (float)h, rad);
    SolidBrush trackBrush(Color(255, 60, 64, 74));
    g.FillPath(&trackBrush, trackPath);
    delete trackPath;

    // Fill (rounded pill, flat fill — compliance fix item 9: this was
    // previously a LinearGradientBrush explicitly self-commented as
    // "for a subtle glow", which the do-not list prohibits outright.
    // Flat fill keeps the same color and shape, just no gradient.
    int fillW = (int)(w * (pct / 100.0));
    if (fillW > w) fillW = w;
    if (fillW > 2) {
        GraphicsPath* fillPath = makeRoundRectPath((float)x, (float)y, (float)fillW, (float)h, rad);
        Color c1(255, GetRValue(fillColor), GetGValue(fillColor), GetBValue(fillColor));
        SolidBrush fillBrush(c1);
        g.FillPath(&fillBrush, fillPath);
        delete fillPath;
    }

    // Thin border
    GraphicsPath* borderPath = makeRoundRectPath((float)x, (float)y, (float)w, (float)h, rad);
    Pen borderPen(Color(255, 80, 84, 94), 1.0f);
    g.DrawPath(&borderPen, borderPath);
    delete borderPath;
}

// Pick color based on score. Signed for AUTHORSHIP LIKELIHOOD (high =
// matches own profile = good = green) — NOT the same signing as
// pairSeverityColor() (gui.cpp), which is deliberately backwards from
// this for pair SIMILARITY (high = concerning). Do not use this one
// on combinedScore.
static COLORREF scoreColor(double pct) {
    if (pct >= 80.0) return COLOR_SUCCESS;
    if (pct >= 60.0) return UL_GOLD;
    if (pct >= 40.0) return COLOR_WARNING;
    return COLOR_ERROR;
}

// ═════════════════════════════════════════════════════════════
// STAGE 5B — Flagged Pairs and All Pairs Table
// ═════════════════════════════════════════════════════════════

// Draw a single feature row (MATCH/MISMATCH with values)
// Shared column geometry for the full feature table AND the
// Deviations block, so the two never drift apart (spec §5.9: "aligned
// across all rows and both students").
struct FeatureRowCols { int deltaX, thisX, profX, deltaW, thisW, profW, labelLeft, labelRight; };
// Status column width: "✗ MISMATCH" needs ~110px, which on a narrow
// window (two cards side by side) left the feature name no room at all.
// Below ~460px of card width it collapses to just the glyph — the
// row's red tint and accent bar still mark a mismatch.
static bool featureStatusCompact(int width) { return width < S(460); }
static int  featureStatusColW(int width) {
    return featureStatusCompact(width) ? S(18) : S(110);
}

static FeatureRowCols featureRowCols(int x, int width, int statusColW) {
    FeatureRowCols c;
    c.deltaW = S(54); c.thisW = S(54); c.profW = S(54);
    // Compact cards drop the small deviation bar column (PROFILE/THIS
    // already carry the numbers) so the feature name keeps its room.
    if (featureStatusCompact(width)) c.deltaW = 0;
    c.deltaX = x + width - S(8) - c.deltaW;
    c.thisX  = c.deltaX - (c.deltaW ? S(10) : 0) - c.thisW;
    c.profX  = c.thisX - S(10) - c.profW;
    c.labelLeft  = x + S(10) + statusColW + S(10);
    c.labelRight = c.profX - S(14);
    return c;
}

// Column header — "PROFILE" / "THIS" mono-aligned labels (spec §5.9:
// "becomes two right-aligned mono columns under a PROFILE / THIS
// header, aligned across all rows and both students").
static int drawFeatureTableHeader(HDC hdc, int x, int y, int width) {
    FeatureRowCols c = featureRowCols(x, width, featureStatusColW(width));
    SetBkMode(hdc, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontLabel);
    SetTextColor(hdc, GRAY_500);
    RECT profH = {c.profX, y, c.profX + c.profW, y + S(18)};
    DrawTextW(hdc, L"PROFILE", -1, &profH, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    RECT thisH = {c.thisX, y, c.thisX + c.thisW, y + S(18)};
    DrawTextW(hdc, L"THIS", -1, &thisH, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    return S(22);
}

// One feature comparison row. Rewritten per spec §5.9:
//  - match rows: no fill, no "MATCH" word — just a 12px gray-500
//    check glyph (dropped from full-strength text+color, since a
//    match is not the thing the instructor needs to see).
//  - mismatch rows: keep "✗ MISMATCH" full-strength in risk-high,
//    with a filled row background — the two rows that matter should
//    visually dominate the twelve that don't.
//  - PROFILE / THIS values as right-aligned mono tabular columns.
//  - a small horizontal delta indicator showing "this" relative to
//    "profile" so magnitude reads without doing arithmetic.
static int drawFeatureRow(HDC hdc, int x, int y, int width,
                            const StyleNoteDisplay& note)
{
    SetBkMode(hdc, TRANSPARENT);
    int rowH = S(34);
    const int statusColW = featureStatusColW(width);
    FeatureRowCols c = featureRowCols(x, width, statusColW);

    RECT rowRect = {x, y, x + width, y + rowH};
    if (!note.isMatch) {
        HBRUSH br = CreateSolidBrush(RGB(46, 27, 31)); // risk-high tint, dark-safe
        FillRect(hdc, &rowRect, br);
        DeleteObject(br);
        RECT accent = {x, y, x + S(3), y + rowH};
        HBRUSH ab = CreateSolidBrush(RISK_HIGH);
        FillRect(hdc, &accent, ab);
        DeleteObject(ab);
    }

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBodyNew);

    // Status column
    if (note.isMatch) {
        SelectObject(hdc, g_hFontMonoSm);
        SetTextColor(hdc, GRAY_500);
        RECT statusRect = {x + S(10), y, x + S(10) + statusColW, y + rowH};
        DrawTextW(hdc, L"\u2713", -1, &statusRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, g_hFontBodyNew);
    } else {
        SetTextColor(hdc, RISK_HIGH);
        RECT statusRect = {x + S(10), y, x + S(10) + statusColW, y + rowH};
        DrawTextW(hdc, featureStatusCompact(width) ? L"\u2717" : L"\u2717 MISMATCH", -1,
                  &statusRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    // Feature label
    SetTextColor(hdc, WHITE_);
    RECT labelRect = {c.labelLeft, y, c.labelRight, y + rowH};
    DrawTextW(hdc, s2w(note.feature).c_str(), -1, &labelRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // PROFILE / THIS mono tabular values
    SelectObject(hdc, g_hFontMono);
    SetTextColor(hdc, GRAY_400);
    std::wostringstream profOs;
    profOs << std::fixed << std::setprecision(2) << note.profileValue;
    RECT profRect = {c.profX, y, c.profX + c.profW, y + rowH};
    DrawTextW(hdc, profOs.str().c_str(), -1, &profRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, note.isMatch ? GRAY_400 : WHITE_);
    std::wostringstream thisOs;
    thisOs << std::fixed << std::setprecision(2) << note.submissionValue;
    RECT thisRect = {c.thisX, y, c.thisX + c.thisW, y + rowH};
    DrawTextW(hdc, thisOs.str().c_str(), -1, &thisRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    // Delta indicator: a small zero-centered horizontal bar. Values
    // are min-max normalized to [0,1] (tech doc §1), so a raw
    // difference is already a bounded, comparable magnitude — no
    // separate scaling needed.
    if (c.deltaW > 0) {
        int barCY = y + rowH / 2;
        int barHalfW = c.deltaW / 2 - S(4);
        int zeroX = c.deltaX + c.deltaW / 2;
        RECT track = {zeroX - barHalfW, barCY - S(2), zeroX + barHalfW, barCY + S(2)};
        HBRUSH trackBr = CreateSolidBrush(GRAY_700);
        FillRect(hdc, &track, trackBr);
        DeleteObject(trackBr);

        double delta = note.submissionValue - note.profileValue;
        if (delta > 1.0) delta = 1.0;
        if (delta < -1.0) delta = -1.0;
        int fillW = (int)(fabs(delta) * barHalfW);
        COLORREF fillCol = note.isMatch ? GRAY_400 : RISK_HIGH;
        RECT fill = (delta >= 0)
            ? RECT{zeroX, barCY - S(2), zeroX + fillW, barCY + S(2)}
            : RECT{zeroX - fillW, barCY - S(2), zeroX, barCY + S(2)};
        HBRUSH fillBr = CreateSolidBrush(fillCol);
        FillRect(hdc, &fill, fillBr);
        DeleteObject(fillBr);

        HPEN tickPen = CreatePen(PS_SOLID, 1, GRAY_500);
        HPEN oldPen = (HPEN)SelectObject(hdc, tickPen);
        MoveToEx(hdc, zeroX, barCY - S(5), nullptr);
        LineTo(hdc, zeroX, barCY + S(5));
        SelectObject(hdc, oldPen);
        DeleteObject(tickPen);
    }

    SelectObject(hdc, oldFont);
    return rowH + S(2);
}

// Draw a feature category section (Lexical/Layout/Syntactic) — used
// only inside the "Show all 14 features" disclosure now; the above-
// fold view uses drawDeviationsBlock() instead (see below).
static int drawFeatureCategory(HDC hdc, int x, int y, int width,
                                 const std::wstring& title,
                                 const std::vector<StyleNoteDisplay>& features)
{
    if (features.empty()) return 0;

    int startY = y;
    SetBkMode(hdc, TRANSPARENT);

    // Category label
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontLabel);
    SetTextColor(hdc, GOLD_500);
    RECT catRect = {x, y, x + width, y + S(22)};
    DrawTextW(hdc, title.c_str(), -1, &catRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    y += S(26);

    SelectObject(hdc, oldFont);

    // Feature rows
    for (const auto& note : features) {
        y += drawFeatureRow(hdc, x, y, width, note);
    }

    y += S(10);
    return y - startY;
}

// ── Deviations block (spec §5.9, line 298): "mismatches are pulled
// above the fold into a Deviations block so the two rows that matter
// are never buried under twelve that don't." Pulls only the
// mismatching StyleNoteDisplay entries out of a student's three
// feature-category vectors (order-preserving: Lexical, then Layout,
// then Syntactic, matching DNA_DISPLAY_ORDER's grouping) and renders
// them with the same row/column drawing code as the full table, so
// values line up identically whether read here or after expanding
// "Show all 14 features".
static int drawDeviationsBlock(HDC hdc, int x, int y, int width,
                                 const AuthorshipDisplay& s)
{
    std::vector<StyleNoteDisplay> deviations;
    for (const auto& n : s.lexicalFeatures)   if (!n.isMatch) deviations.push_back(n);
    for (const auto& n : s.layoutFeatures)    if (!n.isMatch) deviations.push_back(n);
    for (const auto& n : s.syntacticFeatures) if (!n.isMatch) deviations.push_back(n);

    int startY = y;
    SetBkMode(hdc, TRANSPARENT);

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontLabel);
    SetTextColor(hdc, GOLD_500);
    RECT hdrRect = {x, y, x + width, y + S(20)};
    std::wstring hdrText = L"DEVIATIONS";
    DrawTextW(hdc, hdrText.c_str(), -1, &hdrRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    y += S(24);

    if (deviations.empty()) {
        HFONT bf = (HFONT)SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_400);
        RECT noneRect = {x, y, x + width, y + S(24)};
        DrawTextW(hdc, L"No deviating features — all 14 match the established profile.",
                  -1, &noneRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, bf);
        y += S(28);
        return y - startY;
    }

    y += drawFeatureTableHeader(hdc, x, y, width);
    for (const auto& n : deviations) {
        y += drawFeatureRow(hdc, x, y, width, n);
    }
    y += S(6);
    return y - startY;
}

// Draw one student's authorship analysis card
// Height of the Deviations block without drawing it — must mirror
// drawDeviationsBlock() exactly (same 24/22/36-per-row/6 constants,
// each scaled through S() consistently with the draw function).
static int estimateDeviationsBlockHeight(const AuthorshipDisplay& s) {
    int count = 0;
    for (const auto& n : s.lexicalFeatures)   if (!n.isMatch) ++count;
    for (const auto& n : s.layoutFeatures)    if (!n.isMatch) ++count;
    for (const auto& n : s.syntacticFeatures) if (!n.isMatch) ++count;
    if (count == 0) return S(24) + S(28);
    return S(24) + S(22) + count * S(36) + S(6);
}

static const int SHOW_ALL_FEATURES_ROW_H_DESIGN = 26;

// Height of the full feature table (behind "Show all 14 features"),
// without drawing — mirrors drawFeatureCategory()'s 26/36-per-row/10
// constants exactly, each scaled through S() consistently.
static int estimateFullFeatureTableHeight(const AuthorshipDisplay& s) {
    int h = S(22); // drawFeatureTableHeader()
    if (!s.lexicalFeatures.empty())
        h += S(26) + (int)s.lexicalFeatures.size() * S(36) + S(10);
    if (!s.layoutFeatures.empty())
        h += S(26) + (int)s.layoutFeatures.size() * S(36) + S(10);
    if (!s.syntacticFeatures.empty())
        h += S(26) + (int)s.syntacticFeatures.size() * S(36) + S(10);
    return h;
}

static const int AUTH_CARD_HEADER_H_DESIGN = 206;

// Full card height without drawing — must mirror drawAuthorshipCard()
// exactly (used for viewport culling + the pair block's overall
// height estimate). showAll reflects g_showAllFeaturesKeys for this
// (pairIdx, side).
static int estimateAuthorshipCardHeight(const AuthorshipDisplay& s, int pairIdx, int side) {
    bool showAll = g_showAllFeaturesKeys.count(showAllFeaturesKey(pairIdx, side)) > 0;
    int h = S(AUTH_CARD_HEADER_H_DESIGN);
    h += estimateDeviationsBlockHeight(s);
    h += S(SHOW_ALL_FEATURES_ROW_H_DESIGN);
    if (showAll) h += estimateFullFeatureTableHeight(s);
    return h + S(20);
}

// Draw one student's authorship analysis card. Restructured per spec
// §5.9:
//  - Likelihood line now reads "81.2% · High likelihood · profile
//    from 12 submissions · 12 of 14 features matched" in one place
//    instead of split across two rows.
//  - The score bar animates its width on expand. This reuses the
//    pair block's own 240ms open-animation progress
//    (pairExpandAnimProgress()) rather than a second independent
//    300ms clock — the card is only ever drawn while that animation
//    is already driving repaint ticks, so a separate timer would add
//    complexity without a visible difference in feel. openProgress
//    is 1.0 whenever the pair isn't actively opening.
//  - The 14-feature table moves behind "Show all 14 features"; a
//    Deviations block (mismatches only) sits above the fold instead.
static int drawAuthorshipCard(HDC hdc, int x, int y, int width,
                                const AuthorshipDisplay& s,
                                int pairIdx, int side, float openProgress)
{
    SetBkMode(hdc, TRANSPARENT);

    int key = showAllFeaturesKey(pairIdx, side);
    bool showAll = g_showAllFeaturesKeys.count(key) > 0;

    int devH = estimateDeviationsBlockHeight(s);
    int fullTableH = showAll ? estimateFullFeatureTableHeight(s) : 0;
    int cardH = S(AUTH_CARD_HEADER_H_DESIGN) + devH + S(SHOW_ALL_FEATURES_ROW_H_DESIGN) + fullTableH + S(20);

    // Card background
    RECT card = {x, y, x + width, y + cardH};
    drawCard(hdc, card, BG_PANEL, BORDER_COLOR);

    // Student name
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontH2);
    SetTextColor(hdc, GOLD_500);
    RECT nameRect = {x + S(15), y + S(12), x + width - S(15), y + S(35)};
    DrawTextW(hdc, s2w(s.studentName).c_str(), -1, &nameRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // "View source" — the confirmed motivating case (instructors were
    // still manually opening the .c file to eyeball code even after
    // seeing a flag). Drawn first so the filename rect below can be
    // narrowed to leave room for it; resolved to an actual path at
    // click time via g_analysisResults.flaggedPairs[pairIdx].pathA/B,
    // not stored per-button, since PairAnalysisDisplay already carries
    // it and AuthorshipDisplay doesn't need a second copy.
    std::wstring srcBtnLabel = L"⛶  View source";
    SelectObject(hdc, g_hFontSmall);
    RECT measureSrcBtn = {0, 0, 0, 0};
    DrawTextW(hdc, srcBtnLabel.c_str(), -1, &measureSrcBtn, DT_CALCRECT | DT_SINGLELINE);
    int srcBtnW = measureSrcBtn.right - measureSrcBtn.left + S(24);
    int srcBtnH = S(24);
    int srcBtnX = x + width - S(15) - srcBtnW;
    int srcBtnY = y + S(36);
    RECT srcBtnRect = {srcBtnX, srcBtnY, srcBtnX + srcBtnW, srcBtnY + srcBtnH};

    drawCard(hdc, srcBtnRect, GRAY_800, GOLD_500, 6);
    SetTextColor(hdc, GOLD_500);
    DrawTextW(hdc, srcBtnLabel.c_str(), -1, &srcBtnRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SourceViewBtnRect svHit;
    svHit.r = { srcBtnX, srcBtnY + g_scrollY, srcBtnX + srcBtnW, srcBtnY + srcBtnH + g_scrollY };
    svHit.pairIdx = pairIdx;
    svHit.isA = (side == 0);
    g_sourceViewBtnRects.push_back(svHit);

    // Filename — narrowed to end just before the button above instead
    // of the full card width.
    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_400);
    RECT fileRect = {x + S(15), y + S(38), srcBtnX - S(10), y + S(58)};
    DrawTextW(hdc, s2w(s.filename).c_str(), -1, &fileRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Score
    SelectObject(hdc, g_hFontH2);
    COLORREF scoreCol = scoreColor(s.scorePct);
    SetTextColor(hdc, scoreCol);
    RECT scoreRect = {x + S(15), y + S(63), x + width - S(15), y + S(91)};
    std::wstring scoreText = fmtPct(s.scorePct) + L"  \u00b7  " + s2w(s.label);
    DrawTextW(hdc, scoreText.c_str(), -1, &scoreRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Detail line — profile size + match count folded into one line
    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_400);
    RECT detailRect = {x + S(15), y + S(91), x + width - S(15), y + S(111)};
    std::wstring detailText = L"Profile from " + std::to_wstring(s.profileSize) +
                                L" submission";
    if (s.profileSize != 1) detailText += L"s";
    detailText += L"  \u00b7  " + std::to_wstring(s.matchedCount) + L" of " +
                   std::to_wstring(s.totalFeatures) + L" features matched";
    DrawTextW(hdc, detailText.c_str(), -1, &detailRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Progress bar — width animates in on expand (see function comment)
    double barPct = s.scorePct * (double)openProgress;
    drawProgressBar(hdc, x + S(15), y + S(115), width - S(30), S(10), barPct, scoreCol);

    // Reliability note
    SelectObject(hdc, g_hFontMonoSm);
    SetTextColor(hdc, INFO_);
    RECT relRect = {x + S(15), y + S(131), x + width - S(15), y + S(149)};
    std::wstring relText = L"Reliability: " + s2w(s.reliabilityLabel);
    DrawTextW(hdc, relText.c_str(), -1, &relRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Separator
    HPEN pen = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, x + S(15), y + S(158), nullptr);
    LineTo(hdc, x + width - S(15), y + S(158));
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    // Compact Style DNA strip (compliance fix, item 3: severity/
    // mismatch signaling loses its "shape" redundant channel once a
    // pair is expanded — the collapsed row shows both students' DNA
    // strips, but the expanded authorship card previously drew none
    // at all. Reuses the existing dnaValuesFromPair/dnaDeviationFlags/
    // dnaStyleCard plumbing already used elsewhere; not new capability.
    SelectObject(hdc, g_hFontLabel);
    SetTextColor(hdc, GRAY_500);
    RECT dnaLabelRect = {x + S(15), y + S(164), x + width - S(15), y + S(178)};
    DrawTextW(hdc, L"STYLE DNA", -1, &dnaLabelRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    {
        DnaStripStyle cardDnaStyle = dnaStyleCard();
        std::vector<bool> devFlags = dnaDeviationFlags(s);
        drawDnaStrip(hdc, x + S(15), y + S(180), dnaValuesFromPair(s), &devFlags,
                     cardDnaStyle, false, false);
    }

    int featX = x + S(15);
    int featW = width - S(30);
    int featY = y + S(AUTH_CARD_HEADER_H_DESIGN);

    // Deviations block — above the fold (spec §5.9)
    featY += drawDeviationsBlock(hdc, featX, featY, featW, s);

    // "Show all 14 features" disclosure
    {
        SelectObject(hdc, g_hFontBodyNew);
        RECT discRect = {featX, featY, featX + featW, featY + S(SHOW_ALL_FEATURES_ROW_H_DESIGN)};
        SetTextColor(hdc, GOLD_500);
        std::wstring discText = showAll
            ? L"\u25b4  Hide full feature table"
            : L"\u25be  Show all 14 features";
        DrawTextW(hdc, discText.c_str(), -1, &discRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        ShowAllFeaturesRect hit;
        hit.r = { discRect.left, discRect.top + g_scrollY,
                   discRect.right, discRect.bottom + g_scrollY };
        hit.key = key;
        g_showAllFeaturesRects.push_back(hit);
        addFocusable(g_contentFocusables, FocusKind::ShowAllFeaturesToggle, hit.r, key, true);

        featY += S(SHOW_ALL_FEATURES_ROW_H_DESIGN);
    }

    if (showAll) {
        featY += drawFeatureTableHeader(hdc, featX, featY, featW);
        featY += drawFeatureCategory(hdc, featX, featY, featW,
                                       L"LEXICAL", s.lexicalFeatures);
        featY += drawFeatureCategory(hdc, featX, featY, featW,
                                       L"LAYOUT", s.layoutFeatures);
        featY += drawFeatureCategory(hdc, featX, featY, featW,
                                       L"SYNTACTIC", s.syntacticFeatures);
    }

    SelectObject(hdc, oldFont);
    return cardH;
}

// ── Head-to-head comparison (expanded pair) ─────────────────────
// Replaces the two side-by-side authorship cards with ONE mirrored
// comparison: a symmetric table split by a line at the exact center: student
//     A fills the left half (reading in toward the line), student B
// the right half (reading out from it), and each feature's name sits
// centered ON the line. Deviating features come first — features both
// students deviate on at the very top, tagged BOTH — and the features
// that match on both sides fold into one line. Mismatch cells use the
// original row treatment (tint + accent bar + "✗ MISMATCH").
// One function both draws (draw=true) and measures (draw=false), so
// the pair block's height can never disagree with what is drawn.

static const int CMP_HEADER_H_DESIGN = 190;
static const int CMP_ROW_H_DESIGN    = 34;

// Band color behind an expanded pair (see drawFlaggedPairBlock); the
// feature names "cut" the center line by painting this behind them.
static const COLORREF PAIR_BAND_BG = GRAY_850;

// A memory DC for measuring wrapped text when nothing is being drawn
// (height estimates run outside WM_PAINT).
static HDC measureDC() {
    static HDC dc = CreateCompatibleDC(nullptr);
    return dc;
}

struct CompareRow {
    std::wstring name;
    const StyleNoteDisplay* a = nullptr;
    const StyleNoteDisplay* b = nullptr;
    bool devA() const { return a && !a->isMatch; }
    bool devB() const { return b && !b->isMatch; }
};

// Pairs up the two students' feature rows by feature name (both sides
// carry the same 14 features in the same category order), then orders
// them: deviating on both sides, deviating on one side, matching on
// both — stable within each group, so category order is kept.
static std::vector<CompareRow> buildCompareRows(const PairAnalysisDisplay& p) {
    std::vector<CompareRow> rows;
    auto addSide = [&](const AuthorshipDisplay& s, bool isA) {
        for (const auto* v : {&s.lexicalFeatures, &s.layoutFeatures, &s.syntacticFeatures})
            for (const auto& n : *v) {
                std::wstring nm = s2w(n.feature);
                size_t idx = rows.size();
                for (size_t i = 0; i < rows.size(); ++i)
                    if (rows[i].name == nm) { idx = i; break; }
                if (idx == rows.size()) { rows.push_back(CompareRow{}); rows.back().name = nm; }
                if (isA) rows[idx].a = &n; else rows[idx].b = &n;
            }
    };
    addSide(p.studentA, true);
    addSide(p.studentB, false);
    auto rank = [](const CompareRow& r) {
        return (r.devA() && r.devB()) ? 0 : (r.devA() || r.devB()) ? 1 : 2;
    };
    std::stable_sort(rows.begin(), rows.end(),
                     [&](const CompareRow& l, const CompareRow& r) { return rank(l) < rank(r); });
    return rows;
}

// One student's half-width header. mirror = student A (left half):
// everything right-aligned toward the center line, with View source
// next to the line; B (right half) is the mirror image.
static void drawCompareHeaderCell(HDC hdc, int x, int y, int w,
                                  const AuthorshipDisplay& s, int pairIdx, int side,
                                  float openProgress, bool mirror)
{
    SetBkMode(hdc, TRANSPARENT);
    UINT align = mirror ? DT_RIGHT : DT_LEFT;

    // View source — on the name line, at the end nearest the center
    std::wstring srcLabel = L"⛶  View source";
    SelectObject(hdc, g_hFontSmall);
    RECT ms = {0, 0, 0, 0};
    DrawTextW(hdc, srcLabel.c_str(), -1, &ms, DT_CALCRECT | DT_SINGLELINE);
    int btnW = ms.right - ms.left + S(24);
    RECT btn = mirror ? RECT{x + w - btnW, y + S(5), x + w, y + S(31)}
                      : RECT{x, y + S(5), x + btnW, y + S(31)};
    drawCard(hdc, btn, GRAY_800, GOLD_500, 6);
    SetTextColor(hdc, GOLD_500);
    DrawTextW(hdc, srcLabel.c_str(), -1, &btn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SourceViewBtnRect svHit;
    svHit.r = { btn.left, btn.top + g_scrollY, btn.right, btn.bottom + g_scrollY };
    svHit.pairIdx = pairIdx;
    svHit.isA = (side == 0);
    g_sourceViewBtnRects.push_back(svHit);

    // Name
    SelectObject(hdc, g_hFontH1);   // large, the first thing you read
    SetTextColor(hdc, WHITE_);
    RECT nameR = mirror ? RECT{x, y, btn.left - S(12), y + S(36)}
                        : RECT{btn.right + S(12), y, x + w, y + S(36)};
    DrawTextW(hdc, s2w(s.studentName).c_str(), -1, &nameR,
              align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    // File name
    SelectObject(hdc, g_hFontMonoSm);
    SetTextColor(hdc, GRAY_500);
    RECT fileR = {x, y + S(38), x + w, y + S(56)};
    DrawTextW(hdc, s2w(s.filename).c_str(), -1, &fileR,
              align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    // Score · likelihood
    COLORREF scoreCol = scoreColor(s.scorePct);
    SelectObject(hdc, g_hFontButton);
    SetTextColor(hdc, scoreCol);
    RECT scoreR = {x, y + S(60), x + w, y + S(82)};
    std::wstring scoreText = fmtPct(s.scorePct) + L"  ·  " + s2w(s.label);
    DrawTextW(hdc, scoreText.c_str(), -1, &scoreR,
              align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Slim score bar, filling from the center line outward
    {
        RECT track = {x, y + S(87), x + w, y + S(95)};  // 8px score bar
        HBRUSH tb = CreateSolidBrush(GRAY_700);
        FillRect(hdc, &track, tb);
        DeleteObject(tb);
        double pct = s.scorePct * (double)openProgress;
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        int fw = (int)(w * pct / 100.0);
        RECT fill = mirror ? RECT{x + w - fw, track.top, x + w, track.bottom}
                           : RECT{x, track.top, x + fw, track.bottom};
        HBRUSH fb = CreateSolidBrush(scoreCol);
        FillRect(hdc, &fill, fb);
        DeleteObject(fb);
    }

    // Profile size · features matched
    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_400);
    std::wstring detail = std::to_wstring(s.profileSize) +
        (s.profileSize == 1 ? L" submission in profile" : L" submissions in profile") +
        L"  ·  " + std::to_wstring(s.matchedCount) + L" of " +
        std::to_wstring(s.totalFeatures) + L" features match";
    RECT detR = {x, y + S(103), x + w, y + S(123)};
    DrawTextW(hdc, detail.c_str(), -1, &detR, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Reliability
    SelectObject(hdc, g_hFontMonoSm);
    SetTextColor(hdc, INFO_);
    RECT relR = {x, y + S(125), x + w, y + S(143)};
    std::wstring rel = L"Reliability: " + s2w(s.reliabilityLabel);
    DrawTextW(hdc, rel.c_str(), -1, &relR, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Style DNA (deviating cells in red), aligned to the same side
    // Style DNA, a step up from the list rows' strip (wider cells, taller
    // track) — emphasized, but still quieter than the name and score.
    DnaStripStyle ds = { S(6), S(2), S(8), S(26), GRAY_400, RISK_HIGH };
    int dnaX = mirror ? x + w - dnaStripWidth(ds) : x;
    std::vector<bool> devFlags = dnaDeviationFlags(s);
    drawDnaStrip(hdc, dnaX, y + S(152), dnaValuesFromPair(s), &devFlags, ds, false, false);
}

// One student's half of a feature row, in the original row treatment:
// a mismatch gets a tinted fill across that student's half, a 3px
// accent bar on its outer edge, and a "✗ MISMATCH" status label next
// to the center line; a match stays plain with a gray ✓. B (mirror =
// false) reads out from the center line — status, "profile → this",
// gap bar at the far right; A (mirror = true) is the mirror image.
static void drawCompareCell(HDC hdc, int x, int y, int w, int rowH,
                            const StyleNoteDisplay* n, bool mirror)
{
    SetBkMode(hdc, TRANSPARENT);
    if (!n) {
        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_500);
        RECT r = {x, y, x + w, y + rowH};
        DrawTextW(hdc, L"—", -1, &r, (mirror ? DT_RIGHT : DT_LEFT) | DT_VCENTER | DT_SINGLELINE);
        return;
    }
    bool dev = !n->isMatch;

    // Mismatch highlight: tinted fill + accent bar on the outer edge
    if (dev) {
        RECT fill = {x, y + 1, x + w, y + rowH - 1};
        HBRUSH fb = CreateSolidBrush(RGB(46, 27, 31));  // risk-high tint, dark-safe
        FillRect(hdc, &fill, fb);
        DeleteObject(fb);
        RECT accent = mirror ? RECT{x, y + 1, x + S(3), y + rowH - 1}
                             : RECT{x + w - S(3), y + 1, x + w, y + rowH - 1};
        HBRUSH ab = CreateSolidBrush(RISK_HIGH);
        FillRect(hdc, &accent, ab);
        DeleteObject(ab);
    }

    wchar_t pBuf[16], tBuf[16];
    swprintf(pBuf, 16, L"%.2f", n->profileValue);
    swprintf(tBuf, 16, L"%.2f", n->submissionValue);
    const wchar_t* arrow = L"  →  ";
    // Status: full "✗ MISMATCH" label when the half is wide enough,
    // just the glyph on narrow windows.
    const wchar_t* status = dev ? (w >= S(270) ? L"✗ MISMATCH" : L"✗") : L"✓";

    SIZE ss, ps, as, ts;
    SelectObject(hdc, g_hFontBodyNew);
    GetTextExtentPoint32W(hdc, status, (int)wcslen(status), &ss);
    GetTextExtentPoint32W(hdc, arrow, (int)wcslen(arrow), &as);
    SelectObject(hdc, g_hFontMono);
    GetTextExtentPoint32W(hdc, pBuf, (int)wcslen(pBuf), &ps);
    GetTextExtentPoint32W(hdc, tBuf, (int)wcslen(tBuf), &ts);
    int innerPad = S(10);   // from the center-side edge
    int statusGap = S(14);
    int valuesW = ps.cx + as.cx + ts.cx;

    // Status sits next to the center line; values continue outward.
    int sx, vx;
    if (mirror) {
        sx = x + w - innerPad - ss.cx;
        vx = sx - statusGap - valuesW;
    } else {
        sx = x + innerPad;
        vx = sx + ss.cx + statusGap;
    }

    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, dev ? RISK_HIGH : GRAY_500);
    RECT sr = {sx, y, sx + ss.cx + S(2), y + rowH};
    DrawTextW(hdc, status, -1, &sr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    int cx = vx;
    SelectObject(hdc, g_hFontMono);
    SetTextColor(hdc, GRAY_400);
    RECT pr = {cx, y, cx + ps.cx + S(2), y + rowH};
    DrawTextW(hdc, pBuf, -1, &pr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    cx += ps.cx;
    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_500);
    RECT ar = {cx, y, cx + as.cx, y + rowH};
    DrawTextW(hdc, arrow, -1, &ar, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    cx += as.cx;
    SelectObject(hdc, g_hFontMono);
    SetTextColor(hdc, dev ? WHITE_ : GRAY_400);
    RECT tr = {cx, y, cx + ts.cx + S(2), y + rowH};
    DrawTextW(hdc, tBuf, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Gap bar at the outer edge, when there's room for it
    int usedW = innerPad + ss.cx + statusGap + valuesW;
    int barW = S(56);
    if (w - usedW >= barW + S(20)) {
        int barCY = y + rowH / 2;
        int half = barW / 2;
        int zeroX = mirror ? x + S(10) + half : x + w - S(10) - half;
        RECT track = {zeroX - half, barCY - S(2), zeroX + half, barCY + S(2)};
        HBRUSH tb = CreateSolidBrush(GRAY_700);
        FillRect(hdc, &track, tb);
        DeleteObject(tb);
        double d = n->submissionValue - n->profileValue;
        if (d > 1.0) d = 1.0;
        if (d < -1.0) d = -1.0;
        int fw = (int)(fabs(d) * half);
        RECT fill = (d >= 0) ? RECT{zeroX, barCY - S(2), zeroX + fw, barCY + S(2)}
                             : RECT{zeroX - fw, barCY - S(2), zeroX, barCY + S(2)};
        HBRUSH fb = CreateSolidBrush(dev ? RISK_HIGH : GRAY_500);
        FillRect(hdc, &fill, fb);
        DeleteObject(fb);
        RECT tick = {zeroX, barCY - S(5), zeroX + 1, barCY + S(5)};
        HBRUSH kb = CreateSolidBrush(GRAY_500);
        FillRect(hdc, &tick, kb);
        DeleteObject(kb);
    }
}

// Column geometry for the mirrored table.
struct CompareCols { int centerX, nameW, aX, aW, bX, bW; };
static CompareCols compareColumns(int x, int width) {
    CompareCols c;
    c.centerX = x + width / 2;
    c.nameW = width * 22 / 100;
    if (c.nameW < S(150)) c.nameW = S(150);
    if (c.nameW > S(230)) c.nameW = S(230);
    int gap = S(14);
    c.aX = x;
    c.aW = (c.centerX - c.nameW / 2 - gap) - x;
    c.bX = c.centerX + c.nameW / 2 + gap;
    c.bW = (x + width) - c.bX;
    return c;
}

static void drawCompareRow(HDC hdc, int x, int y, int width, const CompareCols& c,
                           const CompareRow& r)
{
    SetBkMode(hdc, TRANSPARENT);
    int rowH = S(CMP_ROW_H_DESIGN);
    bool any  = r.devA() || r.devB();
    bool both = r.devA() && r.devB();

    // Feature name centered on the line (the line is "cut" behind it)
    SelectObject(hdc, any ? g_hFontButton : g_hFontBodyNew);
    SIZE ns; GetTextExtentPoint32W(hdc, r.name.c_str(), (int)r.name.size(), &ns);
    int nameMax = c.nameW - S(8);
    int tagW = 0;
    if (both) {
        HFONT f = (HFONT)SelectObject(hdc, g_hFontLabel);
        SIZE ts; GetTextExtentPoint32W(hdc, L"BOTH", 4, &ts);
        tagW = ts.cx + S(14);
        SelectObject(hdc, f);
    }
    int groupW = (ns.cx < nameMax - (tagW ? tagW + S(6) : 0) ? ns.cx : nameMax - (tagW ? tagW + S(6) : 0))
               + (tagW ? tagW + S(6) : 0);
    int gx = c.centerX - groupW / 2;
    RECT cut = {gx - S(6), y + S(4), gx + groupW + S(6), y + rowH - S(4)};
    HBRUSH bg = CreateSolidBrush(PAIR_BAND_BG);
    FillRect(hdc, &cut, bg);
    DeleteObject(bg);

    SetTextColor(hdc, any ? WHITE_ : GRAY_400);
    RECT lr = {gx, y, gx + groupW - (tagW ? tagW + S(6) : 0), y + rowH};
    DrawTextW(hdc, r.name.c_str(), -1, &lr,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    if (both) {
        // BOTH tag — the same feature departs from each student's own
        // profile in this pair: the strongest signal on the page.
        SelectObject(hdc, g_hFontLabel);
        RECT pill = {gx + groupW - tagW, y + (rowH - S(20)) / 2,
                     gx + groupW, y + (rowH - S(20)) / 2 + S(20)};
        drawCard(hdc, pill, GRAY_900, RISK_HIGH, S(10));
        SetTextColor(hdc, RISK_HIGH);
        DrawTextW(hdc, L"BOTH", -1, &pill, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    drawCompareCell(hdc, c.aX, y, c.aW, rowH, r.a, true);
    drawCompareCell(hdc, c.bX, y, c.bW, rowH, r.b, false);

    // Row separators on each side (not across the name)
    HBRUSH lb = CreateSolidBrush(GRAY_800);
    RECT l1 = {x, y + rowH - 1, cut.left, y + rowH};
    RECT l2 = {cut.right, y + rowH - 1, x + width, y + rowH};
    FillRect(hdc, &l1, lb);
    FillRect(hdc, &l2, lb);
    DeleteObject(lb);
}

static int drawPairComparison(HDC hdc, int x, int y, int width,
                              const PairAnalysisDisplay& pair, int pairIdx,
                              float openProgress, bool draw)
{
    int startY = y;
    SetBkMode(hdc, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBodyNew);

    std::vector<CompareRow> rows = buildCompareRows(pair);

    y += S(4);

    // ── 2. Mirrored table: A | feature on the center line | B ──
    CompareCols c = compareColumns(x, width);
    int tableTop = y;
    int lineGap = S(18);   // headers stop this far from the center line

    if (draw) {
        int aHdrW = c.centerX - lineGap - x;
        int bHdrX = c.centerX + lineGap;
        drawCompareHeaderCell(hdc, x, y, aHdrW, pair.studentA, pairIdx, 0, openProgress, true);
        drawCompareHeaderCell(hdc, bHdrX, y, (x + width) - bHdrX, pair.studentB, pairIdx, 1,
                              openProgress, false);
    }
    y += S(CMP_HEADER_H_DESIGN);

    // Column labels
    if (draw) {
        SelectObject(hdc, g_hFontLabel);
        SetTextColor(hdc, GRAY_500);
        RECT ca = {c.aX, y, c.aX + c.aW, y + S(24)};
        RECT cf = {c.centerX - c.nameW / 2, y, c.centerX + c.nameW / 2, y + S(24)};
        RECT cb = {c.bX, y, c.bX + c.bW, y + S(24)};
        DrawTextW(hdc, L"PROFILE → THIS FILE", -1, &ca, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        // "FEATURE" sits on the line too — cut the line behind it
        SIZE fs; GetTextExtentPoint32W(hdc, L"FEATURE", 7, &fs);
        RECT fcut = {c.centerX - fs.cx / 2 - S(8), y, c.centerX + fs.cx / 2 + S(8), y + S(24)};
        HBRUSH bg = CreateSolidBrush(PAIR_BAND_BG);
        FillRect(hdc, &fcut, bg);
        DeleteObject(bg);
        DrawTextW(hdc, L"FEATURE", -1, &cf, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        DrawTextW(hdc, L"PROFILE → THIS FILE", -1, &cb, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        HBRUSH rb = CreateSolidBrush(GRAY_700);
        RECT r1 = {x, y + S(26), fcut.left, y + S(27)};
        RECT r2 = {fcut.right, y + S(26), x + width, y + S(27)};
        FillRect(hdc, &r1, rb);
        FillRect(hdc, &r2, rb);
        DeleteObject(rb);
    }
    y += S(28);

    // ── 3. Rows: deviations first, matches folded ──
    int rowH = S(CMP_ROW_H_DESIGN);
    int key = showAllFeaturesKey(pairIdx, 0);
    bool showAll = g_showAllFeaturesKeys.count(key) > 0;
    int matchingN = 0, deviatingN = 0;
    for (const auto& r : rows) {
        if (r.devA() || r.devB()) ++deviatingN; else ++matchingN;
    }

    // The center line goes in first; names / labels cut it as they draw.
    int rowsTop = y;
    int rowsH = (deviatingN == 0 ? rowH : deviatingN * rowH) +
                (showAll ? matchingN * rowH : 0);
    if (draw) {
        RECT line = {c.centerX, tableTop, c.centerX + 1, rowsTop + rowsH};
        HBRUSH lb = CreateSolidBrush(GRAY_700);
        FillRect(hdc, &line, lb);
        DeleteObject(lb);
        // re-cut the "FEATURE" label area, which was drawn before the line
        SelectObject(hdc, g_hFontLabel);
        SIZE fs; GetTextExtentPoint32W(hdc, L"FEATURE", 7, &fs);
        RECT fcut = {c.centerX - fs.cx / 2 - S(8), rowsTop - S(28),
                     c.centerX + fs.cx / 2 + S(8), rowsTop - S(4)};
        HBRUSH bg = CreateSolidBrush(PAIR_BAND_BG);
        FillRect(hdc, &fcut, bg);
        DeleteObject(bg);
        SetTextColor(hdc, GRAY_500);
        RECT cf = {c.centerX - c.nameW / 2, rowsTop - S(28), c.centerX + c.nameW / 2, rowsTop - S(4)};
        DrawTextW(hdc, L"FEATURE", -1, &cf, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    if (deviatingN == 0) {
        if (draw) {
            SelectObject(hdc, g_hFontBodyNew);
            SetTextColor(hdc, GRAY_400);
            SIZE ts;
            const wchar_t* none = L"No feature departs from either student’s profile.";
            GetTextExtentPoint32W(hdc, none, (int)wcslen(none), &ts);
            RECT cut = {c.centerX - ts.cx / 2 - S(8), y + S(4), c.centerX + ts.cx / 2 + S(8), y + rowH - S(4)};
            HBRUSH bg = CreateSolidBrush(PAIR_BAND_BG);
            FillRect(hdc, &cut, bg);
            DeleteObject(bg);
            RECT nr = {x, y, x + width, y + rowH};
            DrawTextW(hdc, none, -1, &nr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        y += rowH;
    }
    for (const auto& r : rows) {
        if (!(r.devA() || r.devB())) continue;
        if (draw) drawCompareRow(hdc, x, y, width, c, r);
        y += rowH;
    }
    if (showAll) {
        for (const auto& r : rows) {
            if (r.devA() || r.devB()) continue;
            if (draw) drawCompareRow(hdc, x, y, width, c, r);
            y += rowH;
        }
    }

    if (matchingN > 0) {
        int toggleH = S(34);
        if (draw) {
            SelectObject(hdc, g_hFontBodyNew);
            SetTextColor(hdc, GOLD_500);
            std::wstring t = showAll
                ? L"▴  Hide the " + std::to_wstring(matchingN) + L" features that match on both sides"
                : L"▾  Show the " + std::to_wstring(matchingN) + L" features that match on both sides";
            SIZE ts; GetTextExtentPoint32W(hdc, t.c_str(), (int)t.size(), &ts);
            RECT tr = {c.centerX - ts.cx / 2, y, c.centerX + ts.cx / 2 + S(4), y + toggleH};
            DrawTextW(hdc, t.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            ShowAllFeaturesRect hit;
            hit.r = { tr.left, tr.top + g_scrollY, tr.right, tr.bottom + g_scrollY };
            hit.key = key;
            g_showAllFeaturesRects.push_back(hit);
            addFocusable(g_contentFocusables, FocusKind::ShowAllFeaturesToggle, hit.r, key, true);
        }
        y += toggleH;
    }
    y += S(6);

    SelectObject(hdc, oldFont);
    return y - startY;
}

// Draw comparative authorship analysis box, including the "Copy
// findings" clipboard button (spec §5.9: "Add a small Copy findings
// button that puts a plain-text summary of that pair on the
// clipboard"). pairIdx is the stable index into flaggedPairs (for the
// hit-rect vector / feedback state); pairNumber is the on-screen
// sequential number used in the copied text and the pair header.
static int drawComparativeBox(HDC hdc, int x, int y, int width,
                                const PairAnalysisDisplay& pair,
                                int pairIdx, int pairNumber)
{
    int startY = y;
    SetBkMode(hdc, TRANSPARENT);

    // Measure text height
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBody);
    std::wstring interp = s2w(pair.interpretation);
    if (interp.empty()) {
        interp = L"Both submissions show similar style consistency.";
    }

    RECT measureRect = {x + S(50), y + S(35), x + width - S(30), y + S(500)};
    DrawTextW(hdc, interp.c_str(), -1, &measureRect,
              DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
    int textH = measureRect.bottom - measureRect.top;
    int boxH = S(50) + textH + S(40);  // header + text + footer

    // Box background (maroon-tinted)
    RECT box = {x, y, x + width, y + boxH};
    HBRUSH boxBr = CreateSolidBrush(RGB(55, 35, 42));
    FillRect(hdc, &box, boxBr);
    DeleteObject(boxBr);

    // Gold left accent
    RECT leftBar = {x, y, x + S(4), y + boxH};
    HBRUSH goldBr = CreateSolidBrush(UL_GOLD);
    FillRect(hdc, &leftBar, goldBr);
    DeleteObject(goldBr);

    // Border
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(90, 60, 70));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH nullBr = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, nullBr);
    Rectangle(hdc, box.left, box.top, box.right, box.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(pen);

    // Title
    SetTextColor(hdc, UL_GOLD);
    SelectObject(hdc, g_hFontHeading);
    RECT titleRect = {x + S(50), y + S(12), x + width - S(170), y + S(35)};
    DrawTextW(hdc, L"COMPARATIVE AUTHORSHIP ANALYSIS",
              -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Copy findings button — top-right of the box
    {
        bool justCopied = (g_copyFindingsFeedbackPairIdx == pairIdx &&
                            GetTickCount() < g_copyFindingsFeedbackUntilTick);
        std::wstring btnLabel = justCopied ? L"\u2713  Copied" : L"\u2398  Copy findings";

        SelectObject(hdc, g_hFontSmall);
        RECT measureBtn = {0, 0, 0, 0};
        DrawTextW(hdc, btnLabel.c_str(), -1, &measureBtn, DT_CALCRECT | DT_SINGLELINE);
        int btnW = measureBtn.right - measureBtn.left + S(28);
        int btnH = S(26);
        int btnX = x + width - S(20) - btnW;
        int btnY = y + S(12);
        RECT btnRect = {btnX, btnY, btnX + btnW, btnY + btnH};

        drawCard(hdc, btnRect,
                 justCopied ? COLOR_SUCCESS_BG_ALT : UL_MAROON,
                 justCopied ? COLOR_SUCCESS : UL_GOLD, 6);
        SetTextColor(hdc, justCopied ? COLOR_SUCCESS : UL_GOLD);
        DrawTextW(hdc, btnLabel.c_str(), -1, &btnRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        CopyFindingsBtnRect hit;
        hit.r = { btnX, btnY + g_scrollY, btnX + btnW, btnY + btnH + g_scrollY };
        hit.pairIdx = pairIdx;
        g_copyFindingsBtnRects.push_back(hit);
        addFocusable(g_contentFocusables, FocusKind::CopyFindingsBtn, hit.r, pairIdx, true);
    }

    // Interpretation text
    SetTextColor(hdc, TEXT_MAIN);
    SelectObject(hdc, g_hFontBody);
    RECT interpRect = {x + S(50), y + S(40), x + width - S(30), y + S(40) + textH};
    DrawTextW(hdc, interp.c_str(), -1, &interpRect,
              DT_LEFT | DT_WORDBREAK);

    // Footer disclaimer
    SetTextColor(hdc, TEXT_DIM);
    SelectObject(hdc, g_hFontSmall);
    RECT discRect = {x + S(50), y + S(45) + textH, x + width - S(30), y + boxH - S(8)};
    DrawTextW(hdc,
        L"This report provides statistical evidence only. "
        L"All academic integrity determinations rest with the instructor.",
        -1, &discRect, DT_LEFT | DT_WORDBREAK);

    SelectObject(hdc, oldFont);
    return boxH;
}

// Draws a 96px collapsed summary row (spec §5.9 mockup):
//   #1  94.9%   Rah  ↔  RanueeeOmega            Final Exam    HIGH  ▾
//       [DNA strip A]      [DNA strip B]         token 96.7 · style 92.3
//                                                 N features deviate
// "N features deviate" counts unique feature positions (0..13, display
// order) where EITHER side mismatches — the two sides compare the
// same 14-feature schema against their own profiles, so a position
// can deviate for one student without the other; the row-level count
// unions rather than picking one side arbitrarily. Clicking anywhere
// on the row toggles expansion — the whole row is the hit target,
// same "whole card is the click target" pattern as the Classes-page
// student cards.
static int drawFlaggedPairCollapsedRow(HDC hdc, int x, int y, int width,
                                         const PairAnalysisDisplay& pair,
                                         int pairNumber, int pairIdx)
{
    int rowH = S(PAIR_COLLAPSED_ROW_HEIGHT);
    SetBkMode(hdc, TRANSPARENT);

    COLORREF sevColor = pairSeverityColor(pair.combinedScore);
    const wchar_t* sevLabel = pairSeverityLabel(pair.combinedScore);

    RECT rowRect = {x, y, x + width, y + rowH};
    drawCard(hdc, rowRect, GRAY_850, GRAY_700, 8);

    // Severity-colored left accent bar (spec: "severity color owns
    // the left accent bar and the score")
    RECT accent = {x, y, x + S(4), y + rowH};
    HBRUSH accentBr = CreateSolidBrush(sevColor);
    FillRect(hdc, &accent, accentBr);
    DeleteObject(accentBr);

    int cx = x + S(20);

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontMonoSm);

    // #N
    SetTextColor(hdc, GRAY_500);
    RECT numRect = {cx, y + S(10), cx + S(40), y + S(30)};
    DrawTextW(hdc, (L"#" + std::to_wstring(pairNumber)).c_str(), -1, &numRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Score, severity color
    SelectObject(hdc, g_hFontH2);
    SetTextColor(hdc, sevColor);
    RECT scoreRect = {cx + S(38), y + S(6), cx + S(130), y + S(32)};
    DrawTextW(hdc, fmtPct(pair.combinedScore).c_str(), -1, &scoreRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Names
    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, WHITE_);
    std::wstring names = s2w(pair.studentA.studentName) + L"  \u2194  " +
                          s2w(pair.studentB.studentName);
    RECT namesRect = {cx + S(138), y + S(6), x + width - S(260), y + S(32)};
    DrawTextW(hdc, names.c_str(), -1, &namesRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Period + severity badge + chevron, right side
    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, GRAY_400);
    std::wstring period = periodDisplayLabelGC(extractPeriodGC(pair.filenameA));
    RECT periodRect = {x + width - S(250), y + S(6), x + width - S(110), y + S(32)};
    DrawTextW(hdc, period.c_str(), -1, &periodRect,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, sevColor);
    std::wstring sevUpper = sevLabel;
    for (auto& ch : sevUpper) ch = towupper(ch);
    RECT sevRect = {x + width - S(105), y + S(6), x + width - S(40), y + S(32)};
    DrawTextW(hdc, sevUpper.c_str(), -1, &sevRect,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, GRAY_400);
    RECT chevRect = {x + width - S(34), y + S(6), x + width - S(14), y + S(32)};
    DrawTextW(hdc, L"\u25be", -1, &chevRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    // Two Style DNA strips, one per student — shared shape visible
    // before anything is read (spec §5.9). Existing dnaValuesFromPair/
    // dnaDeviationFlags/dnaStylePair() plumbing (built for this exact
    // case per the in-line comment at their definition) was unused
    // until now — tech doc §5 confirms flagged pairs never drew DNA
    // strips before this pass. trackHits=false: too many of these on
    // screen at once for the single shared hover buffer to mean
    // anything here (that buffer is built for one strip at a time).
    DnaStripStyle pairDnaStyle = dnaStylePair();
    int dnaW = dnaStripWidth(pairDnaStyle);
    int dnaX1 = cx;
    int dnaX2 = dnaX1 + dnaW + S(20);
    int dnaY = y + S(50);

    std::vector<bool> devA = dnaDeviationFlags(pair.studentA);
    std::vector<bool> devB = dnaDeviationFlags(pair.studentB);
    drawDnaStrip(hdc, dnaX1, dnaY, dnaValuesFromPair(pair.studentA), &devA,
                 pairDnaStyle, false, false);
    drawDnaStrip(hdc, dnaX2, dnaY, dnaValuesFromPair(pair.studentB), &devB,
                 pairDnaStyle, false, false);

    // Sub-text: "token X · style Y" and "N features deviate", right
    // of the strips
    int subX = dnaX2 + dnaW + S(24);
    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, GRAY_400);
    std::wstring scoresLine = L"token " + fmtPct(pair.tokenScore) +
                                L" \u00b7 style " + fmtPct(pair.styleScore);
    RECT scoresLineRect = {subX, dnaY - S(2), x + width - S(20), dnaY + S(16)};
    DrawTextW(hdc, scoresLine.c_str(), -1, &scoresLineRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    int deviateCount = 0;
    for (size_t i = 0; i < devA.size(); ++i)
        if (devA[i] || (i < devB.size() && devB[i])) ++deviateCount;
    std::wstring deviateLine = std::to_wstring(deviateCount) +
        (deviateCount == 1 ? L" feature deviates" : L" features deviate");
    RECT deviateRect = {subX, dnaY + S(12), x + width - S(20), dnaY + S(30)};
    DrawTextW(hdc, deviateLine.c_str(), -1, &deviateRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);

    CollapsedPairRowRect hit;
    hit.r = { x, y + g_scrollY, x + width, y + rowH + g_scrollY };
    hit.pairIdx = pairIdx;
    g_collapsedPairRowRects.push_back(hit);
    addFocusable(g_contentFocusables, FocusKind::CollapsedPairRow, hit.r, pairIdx, true);

    return rowH;
}

// Draw one flagged pair, fully expanded. pairIdx (stable flaggedPairs
// index) drives the per-side "Show all 14 features" state and the
// Copy findings button's feedback state; openProgress (0..1, eased)
// scales the authorship cards' score-bar width while the pair is
// actively opening (see drawAuthorshipCard's comment).
// ── Tier 1 AI evidence narrative (per pair) ─────────────────────
// Renders near the Deviations blocks inside each expanded pair card,
// per spec: "evidence commentary, not a replacement for the raw feature
// table." Mirrors drawAISummary's three-state handling (success /
// skipped / failed) rather than collapsing "not requested" and "call
// failed" into a single gray box -- skipped and failed get visually
// distinct treatment, same as the Overview tab's AI box.

// Resolves what text/color to show for pairIdx's Tier 1 note, shared by
// the draw function and its height estimator below so the two can never
// disagree about which state applies (same reasoning as the comment at
// drawDeviationsBlock: "so the two never drift apart").
static void resolvePairNarrativeText(int pairIdx, std::wstring& text,
                                      COLORREF& color, bool& isSuccess) {
    if (!aiKeyConfigured()) {
        text = L"AI evidence notes are off — no Gemini API key is set up on this computer.";
        color = GRAY_400;
        isSuccess = false;
        return;
    }
    const AnalysisResults& r = g_analysisResults;
    bool haveEntry = pairIdx >= 0 && pairIdx < (int)r.aiPairNarratives.size();
    const AIPairNarrative* n = haveEntry ? &r.aiPairNarratives[pairIdx] : nullptr;

    if (n && n->success) {
        text = s2w(n->narrative);
        color = GRAY_200;
        isSuccess = true;
        return;
    }
    isSuccess = false;
    if (n && !n->skipped && !n->error.empty()) {
        // A real failure: the batch call was attempted and failed, not
        // just "not requested." Distinct RISK_MOD treatment.
        text = L"AI evidence narration unavailable: " + readableAIError(n->error);
        color = RISK_MOD;
        return;
    }
    // Skipped, or no entry at all -- main.cpp leaves aiPairNarratives as
    // an empty vector when AI wasn't requested for the run, so "no entry"
    // and "explicitly skipped" both land here, using whatever reason
    // string is available.
    text = (n && !n->error.empty())
        ? s2w(n->error)
        : L"AI evidence narration was not requested for this analysis.";
    color = GRAY_400;
}


// "Set up AI…" link (gold, underlined) at (x, y); registers its click
// rect (content-space) in g_aiSetupLinkRects. Returns its height.
static int drawAiSetupLinkF(HDC hdc, int x, int y) {
    HFONT of = (HFONT)SelectObject(hdc, g_hFontBodyNew);
    const wchar_t* lt = L"Set up AI\u2026";
    SIZE ls; GetTextExtentPoint32W(hdc, lt, (int)wcslen(lt), &ls);
    RECT lr = {x, y, x + ls.cx, y + S(22)};
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, GOLD_400);
    DrawTextW(hdc, lt, -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT ul = {lr.left, lr.bottom - S(2), lr.right, lr.bottom};
    HBRUSH ub = CreateSolidBrush(GOLD_400);
    FillRect(hdc, &ul, ub);
    DeleteObject(ub);
    g_aiSetupLinkRects.push_back(RECT{lr.left, lr.top + g_scrollY, lr.right, lr.bottom + g_scrollY});
    SelectObject(hdc, of);
    return S(22);
}

static int drawPairNarrative(HDC hdc, int x, int y, int width, int pairIdx) {
    int startY = y;
    std::wstring bodyText; COLORREF textColor; bool success;
    resolvePairNarrativeText(pairIdx, bodyText, textColor, success);

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontLabel);
    y += drawEyebrow(hdc, x, y, width, L"AI EVIDENCE NOTE");
    y += S(8);

    SelectObject(hdc, g_hFontBodyNew);
    RECT measureRect = {x + S(16), y + S(14), x + width - S(16), y + 2000};
    DrawTextW(hdc, bodyText.c_str(), -1, &measureRect,
              DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
    int textH = measureRect.bottom - measureRect.top;
    bool noKey = !aiKeyConfigured();
    int boxH = textH + S(28) + (noKey ? S(28) : 0);

    RECT box = {x, y, x + width, y + boxH};
    drawCard(hdc, box, GRAY_850, GRAY_700, 8, success ? GOLD_500 : INFO_);
    SetTextColor(hdc, textColor);
    RECT textRect = {x + S(16), y + S(14), x + width - S(16), y + S(14) + textH};
    DrawTextW(hdc, bodyText.c_str(), -1, &textRect, DT_LEFT | DT_WORDBREAK);
    if (noKey) drawAiSetupLinkF(hdc, x + S(16), y + S(14) + textH + S(6));

    y += boxH;
    SelectObject(hdc, oldFont);
    return y - startY;
}

// Height estimate mirrors drawPairNarrative's layout exactly (eyebrow +
// S(8) + box padding S(28)/S(14)) using the same "~80 chars per line"
// heuristic already used elsewhere in this file for un-drawn text
// (see estimateFlaggedPairBlockHeight's interpretation-height estimate).
static int estimatePairNarrativeHeight(int pairIdx) {
    std::wstring text; COLORREF color; bool success;
    resolvePairNarrativeText(pairIdx, text, color, success);
    int lines = (int)text.size() / 80 + 1;
    int boxH = lines * S(22) + S(28);
    return S(18) + S(8) + boxH; // eyebrow height (~18) + gap + box
}

static int estimateFlaggedPairBlockHeight(const PairAnalysisDisplay& pair, int pairIdx,
                                          int width);

// ── Expanded pair: "gold spine" section ─────────────────────────
// No maroon accordion bar and no box: the open pair keeps the list
// row's own header line (#N · score · names · exam · severity · ▴),
// sits on a band one shade lighter than the page with a 3px gold rule
// down its left edge, and its content (comparison, AI note,
// comparative analysis) is centered at a comfortable max width.
static const int PAIR_EXP_HEADER_H   = 50;
static const int PAIR_EXP_SUBLINE_H  = 30;
static const int PAIR_EXP_INSET_L    = 24;  // left inset (clears the spine)
static const int PAIR_EXP_INSET_R    = 20;
static const int PAIR_EXP_CONTENT_MAX = 1100;

// Content column of an expanded pair, centered in the band.
static void pairExpandedContentCol(int x, int width, int& cx, int& cw) {
    int ix = x + S(PAIR_EXP_INSET_L);
    int iw = width - S(PAIR_EXP_INSET_L) - S(PAIR_EXP_INSET_R);
    cw = iw < S(PAIR_EXP_CONTENT_MAX) ? iw : S(PAIR_EXP_CONTENT_MAX);
    cx = ix + (iw - cw) / 2;
}

// Exact heights of the two text-sized boxes, measured with the same
// font/width/flags their draw functions use.
static int measurePairNarrativeHeight(HDC hdc, int width, int pairIdx) {
    std::wstring text; COLORREF color; bool success;
    resolvePairNarrativeText(pairIdx, text, color, success);
    HFONT of = (HFONT)SelectObject(hdc, g_hFontBodyNew);
    RECT m = {S(16), 0, width - S(16), 2000};
    DrawTextW(hdc, text.c_str(), -1, &m, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
    SelectObject(hdc, of);
    return S(16) /* eyebrow */ + S(8) + (m.bottom - m.top) + S(28) +
           (aiKeyConfigured() ? 0 : S(28));  // "Set up AI…" line
}

static int measureComparativeBoxHeight(HDC hdc, int width, const PairAnalysisDisplay& pair) {
    std::wstring interp = s2w(pair.interpretation);
    if (interp.empty()) interp = L"Both submissions show similar style consistency.";
    HFONT of = (HFONT)SelectObject(hdc, g_hFontBody);
    RECT m = {S(50), S(35), width - S(30), S(500)};
    DrawTextW(hdc, interp.c_str(), -1, &m, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
    SelectObject(hdc, of);
    return S(50) + (m.bottom - m.top) + S(40);
}

// Full expanded height — the exact sum drawFlaggedPairBlock lays out.
static int pairExpandedHeight(HDC hdc, int x, int width,
                              const PairAnalysisDisplay& pair, int pairIdx) {
    int cx, cw;
    pairExpandedContentCol(x, width, cx, cw);
    return S(PAIR_EXP_HEADER_H) + S(PAIR_EXP_SUBLINE_H) + S(14)
         + drawPairComparison(hdc, cx, 0, cw, pair, pairIdx, 1.0f, false) + S(15)
         + measurePairNarrativeHeight(hdc, cw, pairIdx) + S(15)
         + measureComparativeBoxHeight(hdc, cw, pair) + S(24);
}

static int drawFlaggedPairBlock(HDC hdc, int x, int y, int width,
                                  const PairAnalysisDisplay& pair,
                                  int pairNumber, int pairIdx, float openProgress)
{
    int startY = y;
    SetBkMode(hdc, TRANSPARENT);
    int totalH = estimateFlaggedPairBlockHeight(pair, pairIdx, width);

    // Band + gold spine
    {
        RECT band = {x, y, x + width, y + totalH};
        HBRUSH bb = CreateSolidBrush(PAIR_BAND_BG);
        FillRect(hdc, &band, bb);
        DeleteObject(bb);
        RECT spine = {x, y, x + S(3), y + totalH};
        HBRUSH sb = CreateSolidBrush(GOLD_500);
        FillRect(hdc, &spine, sb);
        DeleteObject(sb);
    }

    COLORREF sevColor = pairSeverityColor(pair.combinedScore);
    const wchar_t* sevLabel = pairSeverityLabel(pair.combinedScore);
    int hx = x + S(PAIR_EXP_INSET_L);
    int headerH = S(PAIR_EXP_HEADER_H);

    // Heading — the old accordion header's wording on a maroon wash
    // instead of a bar: solid maroon at the gold spine, fading to
    // transparent across ~70% of the width, no edges or rounded shape.
    // NOTE: this is a gradient, which the design "do not" list rules out
    // (the old header was moved from a gradient to flat maroon for that
    // reason, compliance fix item 9) — chosen deliberately by the user
    // 2026-10-01; flat alternatives were the maroon rule / text heading.
    {
        Graphics g(hdc);
        int washL = x + S(3);
        int washW = (width - S(3)) * 70 / 100;
        LinearGradientBrush wash(
            PointF((REAL)washL - 1.0f, 0.0f), PointF((REAL)(washL + washW + 1), 0.0f),
            Color(255, GetRValue(MAROON_800), GetGValue(MAROON_800), GetBValue(MAROON_800)),
            Color(0,   GetRValue(MAROON_800), GetGValue(MAROON_800), GetBValue(MAROON_800)));
        // Brush span is a pixel wider than the fill on each side so the
        // edges never wrap back to the opposite color.
        g.FillRectangle(&wash, washL, y, washW, headerH);
    }
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontH2);
    {
        std::wstring label = L"FLAGGED PAIR #" + std::to_wstring(pairNumber) +
                             L"  \u2014  " + s2w(pair.similarityLabel);
        SetTextColor(hdc, GOLD_500);
        RECT labelR = {hx, y, x + width - S(140), y + headerH};
        DrawTextW(hdc, label.c_str(), -1, &labelR,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    SetTextColor(hdc, sevColor);
    RECT scoreRect = {x + width - S(140), y, x + width - S(40), y + headerH};
    DrawTextW(hdc, fmtPct(pair.combinedScore).c_str(), -1, &scoreRect,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    (void)sevLabel;

    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GOLD_500);
    RECT chevRect = {x + width - S(34), y, x + width - S(14), y + headerH};
    DrawTextW(hdc, L"\u25b4", -1, &chevRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    // The header line collapses the pair (symmetric with the row click)
    ExpandedPairHeaderRect headerHit;
    headerHit.r = { x, y + g_scrollY, x + width, y + headerH + g_scrollY };
    headerHit.pairIdx = pairIdx;
    g_expandedPairHeaderRects.push_back(headerHit);
    addFocusable(g_contentFocusables, FocusKind::ExpandedPairHeader,
                 headerHit.r, pairIdx, true);
    y += headerH;

    // Sub-line: label + sub-scores + file names, quiet
    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, GRAY_400);
    std::wstring sub = L"Token " + fmtPct(pair.tokenScore) +
        L"   ·   Style " + fmtPct(pair.styleScore) +
        L"   ·   Combined " + fmtPct(pair.combinedScore) +
        L"   ·   " + s2w(pair.filenameA) + L"  vs  " + s2w(pair.filenameB);
    RECT subRect = {hx, y, x + width - S(PAIR_EXP_INSET_R), y + S(PAIR_EXP_SUBLINE_H)};
    DrawTextW(hdc, sub.c_str(), -1, &subRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    y += S(PAIR_EXP_SUBLINE_H);
    {
        RECT rule = {hx, y + S(6), x + width - S(PAIR_EXP_INSET_R), y + S(7)};
        HBRUSH rb = CreateSolidBrush(GRAY_700);
        FillRect(hdc, &rule, rb);
        DeleteObject(rb);
    }
    y += S(14);

    // Centered content column
    int cx, cw;
    pairExpandedContentCol(x, width, cx, cw);

    y += drawPairComparison(hdc, cx, y, cw, pair, pairIdx, openProgress, true);
    y += S(15);
    y += drawPairNarrative(hdc, cx, y, cw, pairIdx);
    y += S(15);
    y += drawComparativeBox(hdc, cx, y, cw, pair, pairIdx, pairNumber);
    y += S(24);

    SelectObject(hdc, oldFont);
    (void)startY;
    return totalH;  // == y - startY by construction (pairExpandedHeight)
}

// Exact expanded height (same layout as drawFlaggedPairBlock), cached
// per pair until the width or its "show matching features" toggle
// changes — so the list only measures pairs that are actually open.
static int estimateFlaggedPairBlockHeight(const PairAnalysisDisplay& pair, int pairIdx,
                                          int width) {
    struct Cached { const PairAnalysisDisplay* p; int width; bool showAll; bool aiKey; int h; };
    static std::map<int, Cached> cache;
    bool showAll = g_showAllFeaturesKeys.count(showAllFeaturesKey(pairIdx, 0)) > 0;
    bool aiKey = aiKeyConfigured();
    auto it = cache.find(pairIdx);
    if (it == cache.end() || it->second.p != &pair || it->second.width != width ||
        it->second.showAll != showAll || it->second.aiKey != aiKey) {
        int h = pairExpandedHeight(measureDC(), 0, width, pair, pairIdx);
        it = cache.insert_or_assign(pairIdx, Cached{&pair, width, showAll, aiKey, h}).first;
    }
    return it->second.h;
}


// Draw all flagged pairs section
int drawFlaggedPairs(HDC hdc, int x, int y, int width)
{
    int startY = y;
    const AnalysisResults& r = g_analysisResults;

    SetBkMode(hdc, TRANSPARENT);

    // Rebuilt fresh every paint — same pattern as g_blockTabRects /
    // g_dnaCellHits elsewhere in this file.
    g_collapsedPairRowRects.clear();
    g_expandedPairHeaderRects.clear();
    g_showAllFeaturesRects.clear();
    g_copyFindingsBtnRects.clear();
    g_sourceViewBtnRects.clear();

    // Count exact duplicates
    int exactCount = 0;
    for (const auto& fp : r.flaggedPairs)
        if (fp.similarityLabel == "Exact Duplicate") ++exactCount;

    // Section header — large title
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontTitle);
    SetTextColor(hdc, TEXT_MAIN);
    std::wstring fullHdr = g_filterExactDuplicatesOnly
        ? L"Exact Duplicates  (" + std::to_wstring(exactCount) + L")"
        : L"Flagged Pairs  (" + std::to_wstring(r.flaggedPairs.size()) + L")";
    RECT hdrRect = {x + S(28), y + S(12), x + width - S(200), y + S(52)};
    DrawTextW(hdc, fullHdr.c_str(), -1, &hdrRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    y += S(58);

    if (r.flaggedPairs.empty()) {
        // No flagged pairs message
        SelectObject(hdc, g_hFontBody);
        SetTextColor(hdc, COLOR_SUCCESS);
        RECT noneRect = {x + S(30), y, x + width - S(30), y + S(60)};
        HBRUSH okBr = CreateSolidBrush(COLOR_SUCCESS_BG_ALT);
        FillRect(hdc, &noneRect, okBr);
        DeleteObject(okBr);

        RECT textRect = {x + S(50), y + S(10), x + width - S(50), y + S(50)};
        DrawTextW(hdc,
            L"No pairs were flagged for review. "
            L"All submissions appear stylistically consistent with their authors.",
            -1, &textRect, DT_LEFT | DT_VCENTER | DT_WORDBREAK);
        y += S(70);
        SelectObject(hdc, oldFont);
        return y - startY;
    }

    // Draw each flagged pair (with viewport culling + filter)
    int innerX = x + S(30);
    int innerW = width - S(60);

    int pairNum = 0;  // Sequential number for filtered view
    for (size_t i = 0; i < r.flaggedPairs.size(); ++i) {
        const auto& pair = r.flaggedPairs[i];

        // Toolbar filter dropdown (flaggedPairVisible).
        if (!flaggedPairVisible(pair)) continue;
        ++pairNum;

        int pairIdx = (int)i;
        bool isExpanded = g_expandedPairs.count(pairIdx) > 0;
        int collapsedH = S(PAIR_COLLAPSED_ROW_HEIGHT);
        auto animIt = g_pairAnimating.find(pairIdx);
        bool animating = animIt != g_pairAnimating.end();
        // Only expanded / animating pairs need their full height — the
        // other ~700 rows are a fixed collapsed height, and measuring
        // the comparison table for each of them every frame was what
        // made this tab lag.
        int fullH = (isExpanded || animating)
            ? estimateFlaggedPairBlockHeight(pair, pairIdx, innerW) : collapsedH;
        float prog = pairExpandAnimProgress(pairIdx);

        int blockH;
        if (!animating) {
            blockH = isExpanded ? fullH : collapsedH;
        } else if (animIt->second.opening) {
            blockH = collapsedH + (int)((fullH - collapsedH) * prog);
        } else {
            blockH = fullH - (int)((fullH - collapsedH) * prog);
        }

        // Deep-link target: record where this pair sits so ContentProc
        // can scroll to it after this paint (see jumpToFlaggedPair).
        if (pairIdx == g_pendingScrollPairIdx)
            g_pendingScrollPairOffset = y - startY;

        bool visible = (y + blockH >= -200) && (y <= g_viewportH + 200);

        if (visible) {
            if (!animating) {
                if (isExpanded) {
                    drawFlaggedPairBlock(hdc, innerX, y, innerW, pair, pairNum, pairIdx, 1.0f);
                } else {
                    drawFlaggedPairCollapsedRow(hdc, innerX, y, innerW, pair, pairNum, pairIdx);
                }
            } else {
                // Mid-animation: draw the full expanded block, clipped
                // to the current animated height, so it reveals (or
                // collapses) top-down over 240ms instead of snapping
                // instantly. GDI has no built-in height-interpolated
                // layout primitive, so a clip region against the
                // fully-drawn expanded content is the practical
                // equivalent within Win32/GDI+ (spec §6: "Expand /
                // collapse... 240ms").
                HRGN clipRgn = CreateRectRgn(innerX, y, innerX + innerW, y + blockH);
                SelectClipRgn(hdc, clipRgn);
                drawFlaggedPairBlock(hdc, innerX, y, innerW, pair, pairNum, pairIdx,
                                      animIt->second.opening ? prog : 1.0f);
                SelectClipRgn(hdc, nullptr);
                DeleteObject(clipRgn);
            }
        }
        y += blockH + S(10);
    }

    if (pairNum == 0) {
        // Filter/search matched nothing — say so instead of an empty page.
        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_400);
        RECT emR = {x + S(30), y, x + width - S(30), y + S(40)};
        DrawTextW(hdc, L"No pairs match this filter.",
                  -1, &emR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        y += S(50);
    }

    SelectObject(hdc, oldFont);
    return y - startY;
}

// Shared by the list above and the toolbar's "Showing N of M" count.
bool flaggedPairVisible(const PairAnalysisDisplay& pair) {
    if (g_filterExactDuplicatesOnly && pair.similarityLabel != "Exact Duplicate")
        return false;
    return pairMatchesSeverityFilter(pair.combinedScore, g_pairSeverityFilter);
}

int countVisibleFlaggedPairs() {
    int n = 0;
    for (const auto& fp : g_analysisResults.flaggedPairs)
        if (flaggedPairVisible(fp)) ++n;
    return n;
}

// Flagged Pairs page click handling — collapsed-row expand, expanded-
// header collapse, "Show all 14 features" disclosure toggle, "Copy
// findings" button, severity filter chip, and clear-filter button.
// Returns true if the click was handled (caller should return 0
// without falling through), false otherwise. mx/scrolledY are already
// in the same coordinate space ContentProc's other per-page hit-tests
// use (scrolledY = my + g_scrollY). Moved out of ContentProc's
// WM_LBUTTONDOWN unchanged (Checkpoint 2 of the ContentProc split) —
// the six original blocks were already contiguous there, so no
// reordering was needed, unlike Checkpoint 1's Overview blocks. Each
// block's original hitX/break/post-loop-return shape is preserved as-is.
bool handleFlaggedPairsClick(HWND hwnd, int mx, int scrolledY) {
    // Collapsed row click → expand (spec §5.9)
    if (!g_collapsedPairRowRects.empty()) {
        bool hitRow = false;
        for (const auto& row : g_collapsedPairRowRects) {
            if (mx >= row.r.left && mx < row.r.right &&
                scrolledY >= row.r.top && scrolledY < row.r.bottom)
            {
                togglePairExpanded(hwnd, row.pairIdx);
                hitRow = true;
                break;
            }
        }
        if (hitRow) return true;
    }

    // Expanded block's header click → collapse
    if (!g_expandedPairHeaderRects.empty()) {
        bool hitHeader = false;
        for (const auto& hdr : g_expandedPairHeaderRects) {
            if (mx >= hdr.r.left && mx < hdr.r.right &&
                scrolledY >= hdr.r.top && scrolledY < hdr.r.bottom)
            {
                togglePairExpanded(hwnd, hdr.pairIdx);
                hitHeader = true;
                break;
            }
        }
        if (hitHeader) return true;
    }

    // "Show all 14 features" disclosure toggle
    if (!g_showAllFeaturesRects.empty()) {
        bool hitDisc = false;
        for (const auto& disc : g_showAllFeaturesRects) {
            if (mx >= disc.r.left && mx < disc.r.right &&
                scrolledY >= disc.r.top && scrolledY < disc.r.bottom)
            {
                toggleShowAllFeatures(hwnd, disc.key);
                hitDisc = true;
                break;
            }
        }
        if (hitDisc) return true;
    }

    // "Copy findings" button
    if (!g_copyFindingsBtnRects.empty()) {
        bool hitCopy = false;
        for (const auto& btn : g_copyFindingsBtnRects) {
            if (mx >= btn.r.left && mx < btn.r.right &&
                scrolledY >= btn.r.top && scrolledY < btn.r.bottom)
            {
                copyFindingsForPair(hwnd, btn.pairIdx);
                hitCopy = true;
                break;
            }
        }
        if (hitCopy) return true;
    }

    // "View source" button
    if (!g_sourceViewBtnRects.empty()) {
        bool hitSrc = false;
        for (const auto& btn : g_sourceViewBtnRects) {
            if (mx >= btn.r.left && mx < btn.r.right &&
                scrolledY >= btn.r.top && scrolledY < btn.r.bottom)
            {
                openSourceViewForFlaggedPair(hwnd, btn.pairIdx, btn.isA);
                hitSrc = true;
                break;
            }
        }
        if (hitSrc) return true;
    }

    return false;
}

#endif // _WIN32
