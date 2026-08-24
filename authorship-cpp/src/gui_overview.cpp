// ─────────────────────────────────────────────────────────────
// gui_overview.cpp
// Overview page — the hero/secondary stat cards with their count-up
// animation, the language-detection notice, and the AI summary with
// its inline "Pair N" deep-links.
//
// Split out of gui.cpp unchanged. PairRefMatch, TextPiece and the
// five drawing/layout helpers stay private; drawStatsDashboard,
// drawAISummary, drawResultsHeader, handleOverviewClick, and three
// globals ContentProc touches are visible, all declared in
// gui_common.h.
//
// Assembled from four regions of gui.cpp; order within each is
// unchanged. drawResultsHeader and handleOverviewClick were added in
// Checkpoint 1 of the ContentProc split (moved verbatim, logic
// unchanged) — see CLAUDE.md.
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"

#ifdef _WIN32

// Overview hero-stat count-up (spec §5.7, §6): numerals count up from
// 0 over 600ms, staggered 60ms apart, once per analysis run — not on
// every repaint/scroll. Triggered exactly once in WM_ANALYSIS_COMPLETE.
DWORD g_statsAnimStartTick = 0;
bool  g_statsAnimActive    = false;

// Returns 0..1 eased progress for a hero card at the given stagger
// index (0, 1, 2 → 0ms, 60ms, 120ms delay before its own 600ms ramp).
static float statCountupProgress(int staggerIndex) {
    if (!g_statsAnimActive) return 1.0f;
    DWORD elapsed = GetTickCount() - g_statsAnimStartTick;
    int delay = staggerIndex * STAT_COUNTUP_STAGGER_MS;
    if ((int)elapsed <= delay) return 0.0f;
    float t = (float)(elapsed - delay) / (float)STAT_COUNTUP_MS;
    if (t >= 1.0f) return 1.0f;
    return easeOutCubic(t);
}

// Hero stat card (spec §5.7) — 44px tabular numerals, 4px left accent
// in the stat's own severity color, one-line gray-400 subtitle.
// `animFrac` is the count-up's current 0..1 progress for this card;
// pass 1.0 for cards that aren't part of the count-up (e.g. redraws
// after the animation has already finished).
static void drawHeroStatCard(HDC hdc, int x, int y, int w, int h,
                               const std::wstring& label,
                               double targetValue, bool isPercent,
                               COLORREF accentColor,
                               const std::wstring& subtitle,
                               float animFrac)
{
    RECT card = {x, y, x + w, y + h};
    drawCard(hdc, card, GRAY_850, GRAY_700, 10, accentColor);

    SetBkMode(hdc, TRANSPARENT);

    double shown = targetValue * animFrac;
    std::wostringstream oss;
    if (isPercent) {
        oss << std::fixed << std::setprecision(1) << shown << L"%";
    } else {
        oss << (long long)(shown + 0.5);
    }

    SetTextColor(hdc, WHITE_);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontStat);
    RECT valRect = {x + S(8), y + S(16), x + w - S(8), y + S(76)};
    DrawTextW(hdc, oss.str().c_str(), -1, &valRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_200);
    RECT lblRect = {x + S(8), y + S(78), x + w - S(8), y + S(100)};
    DrawTextW(hdc, label.c_str(), -1, &lblRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if (!subtitle.empty()) {
        SetTextColor(hdc, GRAY_400);
        RECT subRect = {x + S(8), y + S(100), x + w - S(8), y + h - S(8)};
        DrawTextW(hdc, subtitle.c_str(), -1, &subRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, oldFont);
}

static void drawStatCard(HDC hdc, int x, int y, int w, int h,
                           const std::wstring& label,
                           const std::wstring& value,
                           COLORREF valueColor,
                           const std::wstring& subtitle = L"",
                           COLORREF accentColor = 0)
{
    RECT card = {x, y, x + w, y + h};
    drawCard(hdc, card, BG_PANEL, BORDER_COLOR, 10, accentColor);

    SetBkMode(hdc, TRANSPARENT);

    // Value (large, centered upper area)
    SetTextColor(hdc, valueColor);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontTitle);
    RECT valRect = {x + S(8), y + S(14), x + w - S(8), y + S(68)};
    DrawTextW(hdc, value.c_str(), -1, &valRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, TEXT_DIM);
    RECT lblRect = {x + S(8), y + S(68), x + w - S(8), y + S(90)};
    DrawTextW(hdc, label.c_str(), -1, &lblRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if (!subtitle.empty()) {
        SetTextColor(hdc, RGB(100, 100, 110));
        RECT subRect = {x + S(8), y + S(90), x + w - S(8), y + h - S(8)};
        DrawTextW(hdc, subtitle.c_str(), -1, &subRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, oldFont);
}

int drawStatsDashboard(HDC hdc, int x, int y, int width) {
    int startY = y;
    const AnalysisResults& r = g_analysisResults;
    SetBkMode(hdc, TRANSPARENT);

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBodyNew);
    y += drawEyebrow(hdc, x + S(30), y + S(18), width - S(60), L"SUMMARY STATISTICS");
    y += S(32);

    int padding = S(14);
    int gridW = width - S(60);
    int cardW = (gridW - 2 * padding) / 3;
    int gridX = x + S(30);

    // Subtitle percentages
    std::wostringstream ssFlag, ssDup, ssHigh;
    if (r.totalPairs > 0) {
        ssFlag << std::fixed << std::setprecision(1)
               << (r.flaggedCount * 100.0 / r.totalPairs) << L"% of total pairs";
        ssDup  << std::fixed << std::setprecision(1)
               << (r.exactDuplicates * 100.0 / r.totalPairs) << L"% of total pairs";
        ssHigh << std::fixed << std::setprecision(1)
               << (r.highSimilarity * 100.0 / r.totalPairs) << L"% of total pairs";
    }

    // ── HERO ROW — promoted per spec §5.7: Pairs flagged, High
    // similarity, Max score. 44px numerals, 4px severity-colored
    // left accent, count up once per analysis run. ──────────────
    int heroH = S(140);

    COLORREF flagAccent = r.flaggedCount > 0 ? RISK_MOD : 0;
    COLORREF highAccent = r.highSimilarity > 0 ? RISK_HIGH : 0;

    drawHeroStatCard(hdc, gridX, y, cardW, heroH,
                      L"Pairs flagged", (double)r.flaggedCount, false,
                      flagAccent, ssFlag.str(), statCountupProgress(0));

    drawHeroStatCard(hdc, gridX + cardW + padding, y, cardW, heroH,
                      L"High similarity", (double)r.highSimilarity, false,
                      highAccent, ssHigh.str(), statCountupProgress(1));

    drawHeroStatCard(hdc, gridX + 2*(cardW+padding), y, cardW, heroH,
                      L"Max score", r.maxScore, true,
                      RISK_HIGH, L"Highest similarity detected",
                      statCountupProgress(2));

    y += heroH + padding;

    // ── SECONDARY ROW — demoted per spec: Total pairs, Exact
    // duplicates, Average. Compact, plain gray-850, 24px numerals
    // (g_hFontTitle). Exact Duplicates keeps its click-to-filter
    // behavior — demoting it visually doesn't remove that feature. ──
    int secH = S(96);

    drawStatCard(hdc, gridX, y, cardW, secH,
                 L"Total pairs analyzed",
                 std::to_wstring(r.totalPairs), GRAY_200,
                 L"All possible student pairs");

    g_exactDuplicatesCardRect.left   = gridX + cardW + padding;
    g_exactDuplicatesCardRect.top    = y + g_scrollY;
    g_exactDuplicatesCardRect.right  = gridX + cardW + padding + cardW;
    g_exactDuplicatesCardRect.bottom = y + secH + g_scrollY;

    drawStatCard(hdc, gridX + cardW + padding, y, cardW, secH,
                 L"Exact duplicates",
                 std::to_wstring(r.exactDuplicates),
                 r.exactDuplicates > 0 ? COLOR_ERROR_TEXT : GRAY_200,
                 r.exactDuplicates > 0
                     ? ssDup.str() + L"  \u00B7  tap to filter"
                     : L"0% of total pairs",
                 r.exactDuplicates > 0 ? COLOR_ERROR : 0);

    drawStatCard(hdc, gridX + 2*(cardW+padding), y, cardW, secH,
                 L"Average score",
                 fmtPct(r.averageScore), GRAY_400,
                 L"Across all pairs");

    y += secH + S(12);

    // Compliance fix item 8: the hero/secondary stat cards above are
    // the primary score-showing surface on the whole Overview page
    // and had no disclaimer anywhere on it.
    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_500);
    RECT statsDiscRect = {x, y, x + width, y + S(20)};
    DrawTextW(hdc, L"Statistical evidence only \u00B7 determinations rest with the instructor",
              -1, &statsDiscRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    y += S(24);

    // Unclassified files notice — only shown when relevant. Covers
    // both "type unknown" and "student unknown" cases as ONE unified
    // list — the fix is the same either way: the instructor assigns
    // the file to a known student via the dialog below.
    g_unclassifiedActionRects.clear();
    if (!r.unclassifiedFiles.empty()) {
        int headerH = S(56);
        int rowH = S(34);
        int listH = (int)r.unclassifiedFiles.size() * rowH;
        int noticeH = headerH + listH + S(12);
        RECT notice = {x + S(30), y, x + width - S(30), y + noticeH};
        // Compliance fix item 7: this was UL_GOLD throughout — gold
        // signaling a warning, which the hard rule at this file's top
        // ("gold-500 never signals risk") and the COLOR_WARNING/
        // UL_GOLD changelog fix both exist specifically to prevent.
        // COLOR_WARNING (RISK_MOD) everywhere below instead; tint
        // colors rebased off RISK_MOD rather than gold so the notice
        // doesn't still visually read as gold-branded.
        drawCard(hdc, notice, RGB(48, 36, 20), RGB(95, 68, 35), 10, COLOR_WARNING);

        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, COLOR_WARNING);
        RECT titleR = {x + S(46), y + S(10), x + width - S(46), y + S(28)};
        std::wstring title = L"\u26A0  " + std::to_wstring(r.unclassifiedFiles.size()) +
            L" file" + (r.unclassifiedFiles.size() != 1 ? L"s" : L"") +
            L" need attention";
        DrawTextW(hdc, title.c_str(), -1, &titleR, DT_LEFT | DT_SINGLELINE);

        SetTextColor(hdc, TEXT_DIM);
        RECT bodyR = {x + S(46), y + S(30), x + width - S(46), y + S(48)};
        DrawTextW(hdc,
            L"Unrecognized type, unknown student, or both. Assign to a known "
            L"student or delete.",
            -1, &bodyR, DT_LEFT | DT_WORDBREAK);

        int rowY = y + headerH;
        for (size_t i = 0; i < r.unclassifiedFiles.size(); ++i) {
            const auto& uf = r.unclassifiedFiles[i];

            RECT rowBg = {x + S(40), rowY, x + width - S(40), rowY + rowH - S(4)};
            HBRUSH rowBr = CreateSolidBrush(RGB(38, 28, 16));
            FillRect(hdc, &rowBg, rowBr);
            DeleteObject(rowBr);

            SetTextColor(hdc, TEXT_MAIN);
            RECT nameR = {x + S(50), rowY, x + width - S(220), rowY + rowH - S(4)};
            DrawTextW(hdc, s2w(uf.key).c_str(), -1, &nameR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

            // "Assign" button
            RECT assignBtn = {x + width - S(210), rowY + S(3), x + width - S(120), rowY + rowH - S(8)};
            HPEN aPen = CreatePen(PS_SOLID, 1, COLOR_WARNING);
            HPEN oldAPen = (HPEN)SelectObject(hdc, aPen);
            HBRUSH nullBr = (HBRUSH)GetStockObject(NULL_BRUSH);
            HBRUSH oldABr = (HBRUSH)SelectObject(hdc, nullBr);
            Rectangle(hdc, assignBtn.left, assignBtn.top, assignBtn.right, assignBtn.bottom);
            SelectObject(hdc, oldAPen);
            SelectObject(hdc, oldABr);
            DeleteObject(aPen);
            SetTextColor(hdc, COLOR_WARNING);
            DrawTextW(hdc, L"Assign", -1, &assignBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            UnclassifiedActionRect assignHit;
            assignHit.x = assignBtn.left; assignHit.y = assignBtn.top + g_scrollY;
            assignHit.w = assignBtn.right - assignBtn.left;
            assignHit.h = assignBtn.bottom - assignBtn.top;
            assignHit.fileIdx = (int)i;
            assignHit.isDelete = false;
            g_unclassifiedActionRects.push_back(assignHit);
            addFocusable(g_contentFocusables, FocusKind::UnclassifiedAssign,
                         RECT{assignHit.x, assignHit.y, assignHit.x + assignHit.w, assignHit.y + assignHit.h},
                         (int)i, true);

            // "Delete" button
            RECT deleteBtn = {x + width - S(105), rowY + S(3), x + width - S(45), rowY + rowH - S(8)};
            HPEN dPen = CreatePen(PS_SOLID, 1, COLOR_ERROR_TEXT);
            HPEN oldDPen = (HPEN)SelectObject(hdc, dPen);
            HBRUSH oldDBr = (HBRUSH)SelectObject(hdc, nullBr);
            Rectangle(hdc, deleteBtn.left, deleteBtn.top, deleteBtn.right, deleteBtn.bottom);
            SelectObject(hdc, oldDPen);
            SelectObject(hdc, oldDBr);
            DeleteObject(dPen);
            SetTextColor(hdc, COLOR_ERROR_TEXT);
            DrawTextW(hdc, L"Delete", -1, &deleteBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            UnclassifiedActionRect deleteHit;
            deleteHit.x = deleteBtn.left; deleteHit.y = deleteBtn.top + g_scrollY;
            deleteHit.w = deleteBtn.right - deleteBtn.left;
            deleteHit.h = deleteBtn.bottom - deleteBtn.top;
            deleteHit.fileIdx = (int)i;
            deleteHit.isDelete = true;
            g_unclassifiedActionRects.push_back(deleteHit);
            addFocusable(g_contentFocusables, FocusKind::UnclassifiedDelete,
                         RECT{deleteHit.x, deleteHit.y, deleteHit.x + deleteHit.w, deleteHit.y + deleteHit.h},
                         (int)i, true);

            rowY += rowH;
        }

        y += noticeH + S(20);
    }

    // Language mismatch notice — files whose content clearly isn't C
    // (e.g. Java or Python submitted with a .c extension). This is a
    // DATA QUALITY note, not a similarity finding — recolored to
    // `info` blue (not risk-red) so it doesn't read as an accusation.
    // File list collapsed by default under "Show files" so four
    // filenames don't compete with the statistics above.
    if (!r.languageMismatchFiles.empty()) {
        int baseH = S(56);
        int lineH = S(20);
        int listH = g_langNoticeExpanded
            ? (int)r.languageMismatchFiles.size() * lineH : 0;
        int noticeH = baseH + listH + (g_langNoticeExpanded ? S(8) : 0);
        RECT notice = {x + S(30), y, x + width - S(30), y + noticeH};
        drawCard(hdc, notice, RGB(24, 34, 44), RGB(50, 78, 102), 10, INFO_);

        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, INFO_);
        RECT titleR = {x + S(46), y + S(10), x + width - S(46), y + S(28)};
        std::wstring title = std::to_wstring(r.languageMismatchFiles.size()) +
            L" submission" + (r.languageMismatchFiles.size() != 1 ? L"s" : L"") +
            L" appear to be the wrong language";
        DrawTextW(hdc, title.c_str(), -1, &titleR, DT_LEFT | DT_SINGLELINE);

        SetTextColor(hdc, TEXT_DIM);
        RECT bodyR = {x + S(46), y + S(30), x + width - S(130), y + S(48)};
        DrawTextW(hdc,
            L"Submitted as .c files but the content doesn't look like C. "
            L"Excluded from analysis — a style mismatch here would be "
            L"misleading, not evidence of ghostwriting.",
            -1, &bodyR, DT_LEFT | DT_WORDBREAK);

        // "Show files" / "Hide files" toggle
        SetTextColor(hdc, INFO_);
        RECT toggleR = {x + width - S(120), y + S(10), x + width - S(46), y + S(28)};
        g_langNoticeToggleRect.left   = toggleR.left;
        g_langNoticeToggleRect.top    = toggleR.top + g_scrollY;
        g_langNoticeToggleRect.right  = toggleR.right;
        g_langNoticeToggleRect.bottom = toggleR.bottom + g_scrollY;
        DrawTextW(hdc, g_langNoticeExpanded ? L"Hide files" : L"Show files",
                  -1, &toggleR, DT_RIGHT | DT_SINGLELINE);
        addFocusable(g_contentFocusables, FocusKind::LangNoticeToggle,
                     g_langNoticeToggleRect, 0, true);

        if (g_langNoticeExpanded) {
            int listY = y + baseH;
            for (const auto& entry : r.languageMismatchFiles) {
                RECT entryR = {x + S(46), listY, x + width - S(46), listY + lineH};
                SetTextColor(hdc, TEXT_MAIN);
                DrawTextW(hdc, s2w(entry).c_str(), -1, &entryR,
                          DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
                listY += lineH;
            }
        }

        y += noticeH + S(20);
    }

    SelectObject(hdc, oldFont);
    return y - startY;
}

// Click targets for AI-summary pair deep-links, rebuilt each paint.
std::vector<AISummaryLinkRect> g_aiSummaryLinkRects;

// ── Inline pair-reference links within Tier 2 findings ──────────
// Finding text naturally references pairs in prose ("Pair 7", "pairs 1,
// 2, and 3") without any special prompt instruction asking for it, so
// this detects those references directly from the rendered text rather
// than changing the Tier 2 prompt or relying on Tier 1's positional
// index. Verified against real finding text (including the punctuation-
// glued "(Pair 7)," case and false-positive guards for "pairings"/
// "other pairs") in a standalone harness before being added here.

struct PairRefMatch {
    size_t start;   // character offset into the finding text
    size_t len;     // length of the numeric substring (not "pair"/"pairs")
    int    pairIdx; // 0-based index into flaggedPairs (number - 1)
};

// Scans `text` for "pair"/"pairs" (case-insensitive, word-boundary
// guarded so "pairwise"/"pairings" don't match) followed by a comma/"and"
// separated list of integers, e.g. "Pair 7", "pairs 1, 2, and 3". Only
// numbers in [1, totalPairs] are treated as real references -- anything
// out of range is left as plain text. Returns match offsets for just the
// numeric substrings, not the "pair(s)" word itself.
static std::vector<PairRefMatch> findPairReferences(const std::wstring& text,
                                                       int totalPairs) {
    std::vector<PairRefMatch> matches;
    size_t n = text.size();
    size_t i = 0;
    while (i < n) {
        if (i + 4 <= n &&
            towlower(text[i])   == L'p' && towlower(text[i+1]) == L'a' &&
            towlower(text[i+2]) == L'i' && towlower(text[i+3]) == L'r')
        {
            size_t j = i + 4;
            if (j < n && towlower(text[j]) == L's') ++j; // "pairs"
            if (j < n && (iswalpha(text[j]) || iswdigit(text[j]))) {
                // e.g. "pairwise", "pairings" -- not a real "pair(s)" token
                ++i;
                continue;
            }

            // Walk forward collecting a comma/"and"-separated number list.
            size_t k = j;
            bool foundAny = false;
            while (k < n) {
                size_t scan = k;
                while (scan < n && (iswspace(text[scan]) || text[scan] == L',')) ++scan;
                if (scan + 4 <= n &&
                    towlower(text[scan])   == L'a' && towlower(text[scan+1]) == L'n' &&
                    towlower(text[scan+2]) == L'd' && iswspace(text[scan+3]))
                {
                    scan += 4;
                    while (scan < n && iswspace(text[scan])) ++scan;
                }
                if (scan < n && iswdigit(text[scan])) {
                    size_t numStart = scan;
                    while (scan < n && iswdigit(text[scan])) ++scan;
                    int num = _wtoi(text.substr(numStart, scan - numStart).c_str());
                    if (num >= 1 && num <= totalPairs) {
                        matches.push_back({numStart, scan - numStart, num - 1});
                        foundAny = true;
                    }
                    k = scan;
                    // Only keep scanning the list if a separator follows --
                    // otherwise stop, so an unrelated number later in the
                    // sentence isn't swept into this reference.
                    size_t peek = k;
                    while (peek < n && iswspace(text[peek])) ++peek;
                    bool moreComma = (peek < n && text[peek] == L',');
                    bool moreAnd = (peek + 4 <= n &&
                        towlower(text[peek])   == L'a' && towlower(text[peek+1]) == L'n' &&
                        towlower(text[peek+2]) == L'd' && iswspace(text[peek+3]));
                    if (!moreComma && !moreAnd) break;
                } else {
                    break;
                }
            }
            i = foundAny ? k : i + 4;
            continue;
        }
        ++i;
    }
    return matches;
}

// A "word" (whitespace-delimited run) decomposed into 1-3 pieces, since a
// pair-reference number can be glued to punctuation with no whitespace
// ("(Pair 7),") -- the digits must render/click independently of the
// surrounding characters even though they wrap together as one unit.
struct TextPiece { size_t start; size_t len; bool isLink; int pairIdx; };

// Lays out `text` word-by-word within [x, x+maxWidth), wrapping at
// whitespace boundaries. When draw=true, actually renders each piece
// (GOLD_400 + underline bar for link pieces, GRAY_200 otherwise) and
// appends a clickable rect per link piece to *outLinks; when draw=false,
// performs the identical layout arithmetic without drawing, so a
// measure-only call and a real draw call can never disagree about
// height or line breaks -- this one function is both passes.
// Precondition: caller has already SelectObject'd the desired font into
// hdc (matches the convention used elsewhere in this file).
static int layoutFindingText(HDC hdc, int x, int y, int maxWidth,
                              const std::wstring& text,
                              const std::vector<PairRefMatch>& matches,
                              bool draw,
                              std::vector<AISummaryLinkRect>* outLinks) {
    SIZE spaceSz; GetTextExtentPoint32W(hdc, L" ", 1, &spaceSz);
    int spaceWidth = spaceSz.cx;
    int lineHeight = S(22); // matches the body-text line-height convention
                             // used elsewhere in this file (estimate funcs)

    int cursorX = x, cursorY = y;
    bool firstWordOnLine = true;
    size_t n = text.size();
    size_t matchIdx = 0; // matches is sorted by start, walked in order

    size_t i = 0;
    while (i < n) {
        while (i < n && iswspace(text[i])) ++i; // skip inter-word whitespace
        if (i >= n) break;
        size_t wordStart = i;
        while (i < n && !iswspace(text[i])) ++i;
        size_t wordEnd = i; // [wordStart, wordEnd)

        // Decompose this word into pieces around any overlapping matches.
        std::vector<TextPiece> pieces;
        size_t cursor = wordStart;
        while (matchIdx < matches.size() && matches[matchIdx].start < wordEnd) {
            const auto& m = matches[matchIdx];
            if (m.start > cursor)
                pieces.push_back({cursor, m.start - cursor, false, -1});
            pieces.push_back({m.start, m.len, true, m.pairIdx});
            cursor = m.start + m.len;
            ++matchIdx;
        }
        if (cursor < wordEnd)
            pieces.push_back({cursor, wordEnd - cursor, false, -1});

        // Measure total word width (pieces are contiguous, no gaps).
        int wordWidth = 0;
        for (auto& p : pieces) {
            SIZE sz; GetTextExtentPoint32W(hdc, text.c_str() + p.start, (int)p.len, &sz);
            wordWidth += sz.cx;
        }

        int advance = firstWordOnLine ? 0 : spaceWidth;
        if (!firstWordOnLine && cursorX + advance + wordWidth > x + maxWidth) {
            cursorY += lineHeight;
            cursorX = x;
            firstWordOnLine = true;
            advance = 0;
        }
        cursorX += advance;

        for (auto& p : pieces) {
            SIZE sz; GetTextExtentPoint32W(hdc, text.c_str() + p.start, (int)p.len, &sz);
            if (draw) {
                SetTextColor(hdc, p.isLink ? GOLD_400 : GRAY_200);
                RECT pr = { cursorX, cursorY, cursorX + sz.cx, cursorY + lineHeight };
                DrawTextW(hdc, text.c_str() + p.start, (int)p.len, &pr,
                          DT_LEFT | DT_TOP | DT_SINGLELINE);
                if (p.isLink) {
                    RECT underline = { cursorX, cursorY + lineHeight - S(2),
                                        cursorX + sz.cx, cursorY + lineHeight };
                    HBRUSH ub = CreateSolidBrush(GOLD_400);
                    FillRect(hdc, &underline, ub);
                    DeleteObject(ub);
                    if (outLinks) {
                        AISummaryLinkRect link;
                        // + g_scrollY on the vertical extent, matching the
                        // convention used everywhere else hit-rects are
                        // recorded in content-space during a paint.
                        link.r = { cursorX, cursorY + g_scrollY,
                                   cursorX + sz.cx, cursorY + lineHeight + g_scrollY };
                        link.pairIndex = p.pairIdx;
                        outLinks->push_back(link);
                    }
                }
            }
            cursorX += sz.cx;
        }
        firstWordOnLine = false;
    }

    return (cursorY - y) + lineHeight; // total height including last line
}

// Draw the AI summary box
int drawAISummary(HDC hdc, int x, int y, int width) {
    int startY = y;
    const AnalysisResults& r = g_analysisResults;
    const AIBatchPatterns& bp = r.aiBatchPatterns;

    SetBkMode(hdc, TRANSPARENT);
    g_aiSummaryLinkRects.clear();

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontLabel);
    y += drawEyebrow(hdc, x + S(30), y, width - S(60), L"AI INSTRUCTOR SUMMARY");
    y += S(10);

    if (!bp.success) {
        std::wstring summaryText;
        COLORREF textColor;
        if (bp.skipped) {
            summaryText = !bp.error.empty()
                ? s2w(bp.error) +
                  L"\n\nThe statistical report above is unaffected — "
                  L"this is a garnish, not the product."
                : L"AI summary was not requested for this analysis.\n\n"
                  L"Enable it from the sidebar's 'Include AI summary' "
                  L"checkbox before running analysis.";
            textColor = GRAY_400;
        } else {
            summaryText = L"AI summary unavailable: " + s2w(bp.error) +
                          L"\n\nThe statistical report above is unaffected — "
                          L"this is a garnish, not the product.";
            textColor = RISK_MOD;
        }
        SelectObject(hdc, g_hFontBodyNew);
        RECT measureRect = {x + S(50), y + S(20), x + width - S(40), y + 2000};
        DrawTextW(hdc, summaryText.c_str(), -1, &measureRect,
                  DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
        int textH = measureRect.bottom - measureRect.top;
        int boxH = textH + S(36);
        RECT box = {x + S(30), y, x + width - S(30), y + boxH};
        drawCard(hdc, box, GRAY_850, GRAY_700, 10, INFO_);
        SetTextColor(hdc, textColor);
        RECT textRect = {x + S(48), y + S(18), x + width - S(40), y + boxH - S(10)};
        DrawTextW(hdc, summaryText.c_str(), -1, &textRect, DT_LEFT | DT_WORDBREAK);
        y += boxH + S(24);
        SelectObject(hdc, oldFont);
        return y - startY;
    }

    // Success: Tier 2's cross-pair findings, each rendered as its own row
    // with a small gold accent mark and vertical spacing between items —
    // not one wrapped paragraph (per the discrete-findings redesign).
    // Each finding is laid out via layoutFindingText(), which detects and
    // renders inline "Pair N" references as individually clickable gold+
    // underlined numbers (the AI-summary deep-link, rebuilt against
    // Tier 2's plain prose — see findPairReferences() above). Per-pair
    // evidence (its own separate deep-link target) also lives in each
    // pair's card via Tier 1 — see drawPairNarrative().
    SelectObject(hdc, g_hFontBodyNew);
    const int findingIndent = S(20); // room for the bullet mark before text
    const int findingGap    = S(14); // vertical space between findings
    const int itemMaxWidth  = width - S(96) - findingIndent;
    int totalPairs = (int)r.flaggedPairs.size();

    // First pass: measure total height across all findings. layoutFindingText
    // itself is the shared measure/draw helper, so this pass and the real
    // draw pass below can never disagree about height or line breaks.
    int findingsH = 0;
    for (size_t i = 0; i < bp.findings.size(); ++i) {
        std::wstring item = s2w(bp.findings[i]);
        auto matches = findPairReferences(item, totalPairs);
        int itemH = layoutFindingText(hdc, 0, 0, itemMaxWidth, item, matches,
                                        false, nullptr);
        findingsH += itemH;
        if (i + 1 < bp.findings.size()) findingsH += findingGap;
    }
    int boxH = findingsH + S(64); // same lump top/footer/bottom overhead as before

    RECT box = {x + S(30), y, x + width - S(30), y + boxH};
    drawCard(hdc, box, GRAY_850, GRAY_700, 10, GOLD_500);

    // Second pass: draw each finding with its own gold bullet mark, plus
    // any inline pair-reference links within it.
    int fy = y + S(18);
    for (const auto& finding : bp.findings) {
        std::wstring item = s2w(finding);
        auto matches = findPairReferences(item, totalPairs);

        SetTextColor(hdc, GOLD_500);
        RECT markRect = {x + S(48), fy, x + S(48) + findingIndent, fy + S(20)};
        DrawTextW(hdc, L"\u2022", -1, &markRect, DT_LEFT | DT_TOP | DT_SINGLELINE);

        int itemH = layoutFindingText(hdc, x + S(48) + findingIndent, fy,
                                        itemMaxWidth, item, matches, true,
                                        &g_aiSummaryLinkRects);

        fy += itemH + findingGap;
    }

    // Attribution chip (moved out of the section header per spec)
    SelectObject(hdc, g_hFontMonoSm);
    SetTextColor(hdc, GRAY_500);
    RECT sigRect = {x + S(48), y + boxH - S(50), x + width - S(40), y + boxH - S(30)};
    DrawTextW(hdc, L"Gemini 2.5 Flash Lite  \u00B7  CALSS AI Instructor",
              -1, &sigRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Compliance fix item 8: this AI-generated interpretive text is a
    // score-showing surface (references pair scores and priority cases
    // directly) that had no disclaimer line anywhere.
    SetTextColor(hdc, GRAY_500);
    RECT discRect = {x + S(48), y + boxH - S(28), x + width - S(40), y + boxH - S(10)};
    DrawTextW(hdc, L"Statistical evidence only \u00B7 determinations rest with the instructor",
              -1, &discRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    y += boxH + S(24);
    SelectObject(hdc, oldFont);
    return y - startY;
}

// ─────────────────────────────────────────────────────────────
// Checkpoint 1 of the ContentProc split (see CLAUDE.md backlog):
// Overview's WM_PAINT header draw and WM_LBUTTONDOWN hit-testing,
// moved out of gui.cpp unchanged. drawResultsHeader joins its two
// siblings above (drawStatsDashboard, drawAISummary) as this file's
// third WM_PAINT-called entry point; handleOverviewClick is
// ContentProc's WM_LBUTTONDOWN entry point for this page, called
// exactly where the first of its four original inline blocks used
// to sit.
// ─────────────────────────────────────────────────────────────

// Draw the header section (title bar with timestamp)
int drawResultsHeader(HDC hdc, int x, int y, int width) {
    int startY = y;
    const AnalysisResults& r = g_analysisResults;
    SetBkMode(hdc, TRANSPARENT);

    // Concept 3: Minimal floating card header, not a heavy maroon band
    int cardH = S(72);
    RECT card = {x + S(24), y + S(12), x + width - S(24), y + S(12) + cardH};
    drawCard(hdc, card, BG_PANEL, BORDER_COLOR, 12, UL_MAROON);

    // Small label "ANALYSIS REPORT" in gold uppercase
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, UL_GOLD);
    RECT labelRect = {x + S(42), y + S(20), x + width - S(30), y + S(38)};
    DrawTextW(hdc, L"ANALYSIS REPORT", -1, &labelRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Timestamp + path in muted text
    SetTextColor(hdc, TEXT_DIM);
    RECT subRect = {x + S(42), y + S(40), x + width - S(30), y + S(58)};
    std::wstring sub = L"Generated " + s2w(r.timestamp);
    sub += L"  |  Data: " + s2w(r.dataFolder);
    DrawTextW(hdc, sub.c_str(), -1, &subRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    y += S(12) + cardH + S(8);
    SelectObject(hdc, oldFont);
    return y - startY;
}

// Overview page click handling — exact-duplicates card (deep-links to
// Flagged Pairs with the filter applied), AI-summary "Pair N" links
// (spec §5.7), language-notice "Show files" toggle, and Unclassified
// file Assign/Delete actions. Returns true if the click was handled
// (caller should return 0 without falling through), false otherwise.
// mx/scrolledY are already in the same coordinate space ContentProc's
// other per-page hit-tests use (scrolledY = my + g_scrollY).
bool handleOverviewClick(HWND hwnd, int mx, int scrolledY) {
    // Exact Duplicates card click
    if (g_analysisResults.exactDuplicates > 0) {
        if (mx >= g_exactDuplicatesCardRect.left &&
            mx < g_exactDuplicatesCardRect.right &&
            scrolledY >= g_exactDuplicatesCardRect.top &&
            scrolledY < g_exactDuplicatesCardRect.bottom)
        {
            // Switch to Flagged Pairs tab with filter
            g_currentPage = PAGE_FLAGGED;
            g_filterExactDuplicatesOnly = true;
            g_selectedStudent = -1;
            g_scrollY = 0;
            InvalidateRect(hwnd, nullptr, FALSE);
            return true;
        }
    }

    // AI summary pair deep-links (spec §5.7) — the single highest-value
    // interaction the brief calls out. Also auto-expands the referenced
    // pair (step 8's Flagged Pairs expand/collapse model).
    if (!g_aiSummaryLinkRects.empty()) {
        for (const auto& link : g_aiSummaryLinkRects) {
            if (mx >= link.r.left && mx < link.r.right &&
                scrolledY >= link.r.top && scrolledY < link.r.bottom)
            {
                jumpToFlaggedPair(hwnd, link.pairIndex);
                return true;
            }
        }
    }

    // Language notice "Show files" toggle
    if (!g_analysisResults.languageMismatchFiles.empty() &&
        mx >= g_langNoticeToggleRect.left && mx < g_langNoticeToggleRect.right &&
        scrolledY >= g_langNoticeToggleRect.top && scrolledY < g_langNoticeToggleRect.bottom)
    {
        g_langNoticeExpanded = !g_langNoticeExpanded;
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

    // Unclassified file Assign/Delete clicks
    if (!g_unclassifiedActionRects.empty()) {
        for (const auto& action : g_unclassifiedActionRects) {
            if (mx >= action.x && mx < action.x + action.w &&
                scrolledY >= action.y && scrolledY < action.y + action.h)
            {
                handleUnclassifiedAction(hwnd, action.fileIdx, action.isDelete);
                return true;
            }
        }
    }

    return false;
}

#endif // _WIN32