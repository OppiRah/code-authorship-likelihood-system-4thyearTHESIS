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

// Thousands separators for counts shown in prose ("1,740 pairs").
static std::wstring fmtCount(long long v) {
    std::wstring s = std::to_wstring(v < 0 ? -v : v);
    for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert((size_t)i, L",");
    return v < 0 ? L"-" + s : s;
}

// 1px horizontal / vertical hairlines in GRAY_700 — the only structure
// the Report-sheet Overview uses in place of card boxes.
static void hairlineH(HDC hdc, int x1, int x2, int y) {
    RECT r = {x1, y, x2, y + 1};
    HBRUSH br = CreateSolidBrush(GRAY_700);
    FillRect(hdc, &r, br);
    DeleteObject(br);
}
static void hairlineV(HDC hdc, int x, int y1, int y2) {
    RECT r = {x, y1, x + 1, y2};
    HBRUSH br = CreateSolidBrush(GRAY_700);
    FillRect(hdc, &r, br);
    DeleteObject(br);
}

// One column of the Overview stat row (spec §5.7 44px numerals, in the
// Report-sheet layout): no card, everything left-aligned — the numeral,
// an uppercase label (with a small severity square when `dotColor` is
// set), and a one-line note. `animFrac` is the count-up's current 0..1
// progress; 1.0 once the animation has finished.
static const int STAT_COLUMN_HEIGHT = 100;

static void drawStatColumn(HDC hdc, int x, int y, int w,
                           const std::wstring& label,
                           double value, bool isPercent,
                           COLORREF dotColor,
                           const std::wstring& note,
                           float animFrac)
{
    double shown = value * animFrac;
    std::wstring num;
    if (isPercent) {
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(1) << shown << L"%";
        num = oss.str();
    } else {
        num = fmtCount((long long)(shown + 0.5));
    }

    SetBkMode(hdc, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontStat);
    SetTextColor(hdc, WHITE_);
    RECT numR = {x, y, x + w, y + S(56)};
    DrawTextW(hdc, num.c_str(), -1, &numR,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    int labelX = x;
    if (dotColor) {
        RECT dot = {x, y + S(66), x + S(8), y + S(74)};
        HBRUSH br = CreateSolidBrush(dotColor);
        FillRect(hdc, &dot, br);
        DeleteObject(br);
        labelX += S(14);
    }
    SelectObject(hdc, g_hFontLabel);
    SetTextColor(hdc, GRAY_200);
    RECT lblR = {labelX, y + S(62), x + w, y + S(78)};
    DrawTextW(hdc, label.c_str(), -1, &lblR,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    if (!note.empty()) {
        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, GRAY_400);
        RECT noteR = {x, y + S(80), x + w, y + S(STAT_COLUMN_HEIGHT)};
        DrawTextW(hdc, note.c_str(), -1, &noteR,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    SelectObject(hdc, oldFont);
}

int drawStatsDashboard(HDC hdc, int x, int y, int width) {
    int startY = y;
    const AnalysisResults& r = g_analysisResults;
    SetBkMode(hdc, TRANSPARENT);

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBodyNew);

    int left  = x + S(30);
    int right = x + width - S(30);

    auto pctOfPairs = [&](int n) -> std::wstring {
        if (r.totalPairs <= 0) return L"";
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(1)
            << (n * 100.0 / r.totalPairs) << L"% of all pairs";
        return oss.str();
    };

    // ── STAT ROW (Report-sheet layout) — four left-aligned columns
    // between hairlines, separated by vertical rules; no cards. The
    // count-up runs once per analysis (statCountupProgress). Total
    // pairs and exact duplicates moved up into the headline sentence
    // (drawResultsHeader), which also carries the exact-duplicates
    // click-to-filter link. ─────────────────────────────────────
    hairlineH(hdc, left, right, y);
    int rowTop = y + S(20);
    int rowH   = S(STAT_COLUMN_HEIGHT);
    int colW   = (right - left) / 4;
    int colPad = S(24);

    struct StatDef {
        const wchar_t* label; double value; bool isPercent;
        COLORREF dot; std::wstring note;
    };
    StatDef stats[4] = {
        { L"FLAGGED",         (double)r.flaggedCount,   false,
          r.flaggedCount > 0 ? RISK_MOD : 0,    pctOfPairs(r.flaggedCount) },
        { L"HIGH SIMILARITY", (double)r.highSimilarity, false,
          r.highSimilarity > 0 ? RISK_HIGH : 0, pctOfPairs(r.highSimilarity) },
        { L"MAX SCORE",       r.maxScore,               true,
          0,                                    L"Highest-scoring pair" },
        { L"AVERAGE SCORE",   r.averageScore,           true,
          0,                                    L"Across all pairs" },
    };
    for (int i = 0; i < 4; ++i) {
        int colX  = left + i * colW;
        int textX = i == 0 ? colX : colX + colPad;
        int textW = (i == 3 ? right : colX + colW) - textX - S(8);
        if (i > 0) hairlineV(hdc, colX, rowTop, rowTop + rowH);
        drawStatColumn(hdc, textX, rowTop, textW, stats[i].label,
                       stats[i].value, stats[i].isPercent, stats[i].dot,
                       stats[i].note, statCountupProgress(i));
    }
    y = rowTop + rowH + S(20);
    hairlineH(hdc, left, right, y);
    y += S(24);

    // ── RISK DISTRIBUTION BAR — every analyzed pair, split by the same
    // High (>=85% combined) / Moderate tiers the Flagged Pairs tab's
    // severity chips use, so the two pages always agree. Note this is
    // NOT the "High similarity" stat above, which counts the separate
    // token-similarity label across all pairs. ────────────────────
    if (r.totalPairs > 0) {
        int highCount = 0, modCount = 0;
        for (const auto& fp : r.flaggedPairs) {
            if (fp.combinedScore >= 85.0) ++highCount; else ++modCount;
        }
        int notFlagged = r.totalPairs - highCount - modCount;
        if (notFlagged < 0) notFlagged = 0;
        int total = highCount + modCount + notFlagged;

        SelectObject(hdc, g_hFontLabel);
        SetTextColor(hdc, GRAY_400);
        std::wstring caption = L"ALL " + fmtCount(r.totalPairs) + L" PAIRS BY RISK";
        RECT capR = {left, y, right, y + S(16)};
        DrawTextW(hdc, caption.c_str(), -1, &capR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        y += S(24);

        struct Seg { int count; COLORREF color; std::wstring label; };
        Seg segs[3] = {
            { highCount,  RISK_HIGH, L"Flagged high  " + fmtCount(highCount) },
            { modCount,   RISK_MOD,  L"Flagged moderate  " + fmtCount(modCount) },
            { notFlagged, GRAY_700,  L"Not flagged  " + fmtCount(notFlagged) },
        };

        int barH = S(8);
        int barW = right - left;
        int segX = left;
        for (int i = 0; i < 3; ++i) {
            if (segs[i].count <= 0) continue;
            int w = (i == 2) ? (left + barW - segX)
                             : (int)((double)segs[i].count * barW / total + 0.5);
            if (w < S(2)) w = S(2);
            RECT segR = {segX, y, segX + w, y + barH};
            HBRUSH br = CreateSolidBrush(segs[i].color);
            FillRect(hdc, &segR, br);
            DeleteObject(br);
            segX += w;
        }
        y += barH + S(12);

        SelectObject(hdc, g_hFontSmall);
        int legX = left;
        for (const auto& s : segs) {
            RECT sq = {legX, y + S(5), legX + S(8), y + S(13)};
            HBRUSH br = CreateSolidBrush(s.color);
            FillRect(hdc, &sq, br);
            DeleteObject(br);
            SIZE sz; GetTextExtentPoint32W(hdc, s.label.c_str(), (int)s.label.size(), &sz);
            SetTextColor(hdc, GRAY_400);
            RECT tr = {legX + S(14), y, legX + S(14) + sz.cx, y + S(18)};
            DrawTextW(hdc, s.label.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            legX = tr.right + S(28);
        }
        y += S(18) + S(36);
    }

    // Unclassified files notice — only shown when relevant. Covers
    // both "type unknown" and "student unknown" cases as ONE unified
    // list — the fix is the same either way: the instructor assigns
    // the file to a known student via the dialog below.
    g_unclassifiedActionRects.clear();
    if (!r.unclassifiedFiles.empty()) {
        const wchar_t* unclBody =
            L"Unrecognized type, unknown student, or both. Assign to a known "
            L"student or delete.";
        SelectObject(hdc, g_hFontSmall);
        RECT unclMeasure = {x + S(46), 0, x + width - S(46), 0};
        DrawTextW(hdc, unclBody, -1, &unclMeasure, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
        int unclBodyH = unclMeasure.bottom - unclMeasure.top;
        int headerH = S(30) + unclBodyH + S(10);
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
        RECT bodyR = {x + S(46), y + S(30), x + width - S(46), y + S(30) + unclBodyH};
        DrawTextW(hdc, unclBody, -1, &bodyR, DT_LEFT | DT_WORDBREAK);

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
        // Box height follows the wrapped body text — it was a fixed
        // one-line rect and cut its second line off on narrow windows.
        const wchar_t* langBody =
            L"Submitted as .c files but the content doesn't look like C. "
            L"Excluded from analysis — a style mismatch here would be "
            L"misleading, not evidence of ghostwriting.";
        SelectObject(hdc, g_hFontSmall);
        RECT langMeasure = {x + S(46), 0, x + width - S(130), 0};
        DrawTextW(hdc, langBody, -1, &langMeasure, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
        int langBodyH = langMeasure.bottom - langMeasure.top;
        int baseH = S(30) + langBodyH + S(12);
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
        RECT bodyR = {x + S(46), y + S(30), x + width - S(130), y + S(30) + langBodyH};
        DrawTextW(hdc, langBody, -1, &bodyR, DT_LEFT | DT_WORDBREAK);

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

// Fallback for findings that name a pair by its students instead of its
// number ("CastroBryan vs DizonCzarina"). The Tier 2 prompt asks for
// "Pair N", but the model doesn't always comply -- when it doesn't, both
// names become links to the highest-scoring flagged pair between those
// two students (flaggedPairs is sorted by score, so the first hit wins).
// Only whitespace-free names are matched, since layoutFindingText's
// pieces can't span a word break.
static std::vector<PairRefMatch> findNamePairReferences(
    const std::wstring& text, const std::vector<PairAnalysisDisplay>& pairs)
{
    std::vector<PairRefMatch> matches;
    if (pairs.empty()) return matches;

    std::wstring lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    // Unordered student-pair key -> first (best) flagged pair index.
    std::map<std::pair<std::wstring, std::wstring>, int> pairByNames;
    std::set<std::wstring> names;
    for (size_t i = 0; i < pairs.size(); ++i) {
        std::wstring a = s2w(pairs[i].studentA.studentName);
        std::wstring b = s2w(pairs[i].studentB.studentName);
        std::transform(a.begin(), a.end(), a.begin(), ::towlower);
        std::transform(b.begin(), b.end(), b.begin(), ::towlower);
        if (a.empty() || b.empty() || a == b) continue;
        auto key = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
        pairByNames.emplace(key, (int)i);
        names.insert(a);
        names.insert(b);
    }

    // Every word-bounded occurrence of a known student name, in order.
    struct NameHit { size_t start; size_t len; std::wstring name; };
    std::vector<NameHit> hits;
    for (const auto& name : names) {
        if (name.find_first_of(L" \t\r\n") != std::wstring::npos) continue;
        size_t pos = 0;
        while ((pos = lower.find(name, pos)) != std::wstring::npos) {
            size_t end = pos + name.size();
            bool leftOk  = pos == 0 || !iswalnum(lower[pos - 1]);
            bool rightOk = end >= lower.size() || !iswalnum(lower[end]);
            if (leftOk && rightOk) hits.push_back({pos, name.size(), name});
            pos = end;
        }
    }
    std::sort(hits.begin(), hits.end(),
              [](const NameHit& l, const NameHit& r) { return l.start < r.start; });

    // Two consecutive names joined by a pair separator form a reference.
    for (size_t i = 0; i + 1 < hits.size(); ++i) {
        const NameHit& h1 = hits[i];
        const NameHit& h2 = hits[i + 1];
        size_t gapStart = h1.start + h1.len;
        if (h2.start < gapStart) continue;
        std::wstring sep = lower.substr(gapStart, h2.start - gapStart);
        sep.erase(0, sep.find_first_not_of(L" \t"));
        size_t last = sep.find_last_not_of(L" \t");
        sep = (last == std::wstring::npos) ? L"" : sep.substr(0, last + 1);
        bool isSep = sep == L"vs" || sep == L"vs." || sep == L"versus" ||
                     sep == L"and" || sep == L"&" || sep == L"↔" ||
                     sep == L"/";
        if (!isSep) continue;

        auto key = h1.name < h2.name ? std::make_pair(h1.name, h2.name)
                                     : std::make_pair(h2.name, h1.name);
        auto it = pairByNames.find(key);
        if (it == pairByNames.end()) continue;
        matches.push_back({h1.start, h1.len, it->second});
        matches.push_back({h2.start, h2.len, it->second});
        ++i; // h2 is consumed; don't let it start another reference
    }
    return matches;
}

// "Pair N" references plus the name-pair fallback, merged into one list
// sorted by offset with overlaps dropped -- layoutFindingText walks it
// in order and assumes no two matches overlap.
static std::vector<PairRefMatch> findAllPairLinks(const std::wstring& text) {
    const auto& pairs = g_analysisResults.flaggedPairs;
    std::vector<PairRefMatch> all = findPairReferences(text, (int)pairs.size());
    std::vector<PairRefMatch> byName = findNamePairReferences(text, pairs);
    all.insert(all.end(), byName.begin(), byName.end());
    std::sort(all.begin(), all.end(),
              [](const PairRefMatch& l, const PairRefMatch& r) { return l.start < r.start; });

    std::vector<PairRefMatch> merged;
    for (const auto& m : all) {
        if (!merged.empty() &&
            m.start < merged.back().start + merged.back().len) continue;
        merged.push_back(m);
    }
    return merged;
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

// Shared with gui_flagged.cpp's per-pair AI Evidence Note.
// gemini.cpp reports HTTP failures as "API returned status 503: {raw
// JSON body}". The instructor needs Gemini's own one-line "message" and
// the status code, not the JSON — extract those when present, otherwise
// pass the error through unchanged.
std::wstring readableAIError(const std::string& err) {
    size_t m = err.find("\"message\"");
    if (m == std::string::npos) return s2w(err);
    size_t colon = err.find(':', m);
    size_t q1 = colon == std::string::npos ? colon : err.find('"', colon + 1);
    if (q1 == std::string::npos) return s2w(err);
    size_t q2 = err.find('"', q1 + 1);
    // gemini.cpp keeps only the first 200 chars of the HTTP body, so a
    // long message (e.g. the 429 quota text) loses its closing quote —
    // take what's there and mark it as cut.
    std::string msg = q2 == std::string::npos
        ? err.substr(q1 + 1) + "…"
        : err.substr(q1 + 1, q2 - q1 - 1);

    size_t st = err.find("status ");
    if (st != std::string::npos) {
        size_t end = st + 7;
        while (end < err.size() && isdigit((unsigned char)err[end])) ++end;
        if (end > st + 7) msg += " (HTTP " + err.substr(st + 7, end - st - 7) + ")";
    }
    return s2w(msg);
}


// "Set up AI…" link (gold, underlined) at (x, y); registers its click
// rect (content-space) in g_aiSetupLinkRects. Returns its height.
static int drawAiSetupLink(HDC hdc, int x, int y) {
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

    // No Gemini key on this computer: say so plainly and offer the
    // settings dialog, instead of a "no API key configured" error.
    if (!aiKeyConfigured()) {
        SelectObject(hdc, g_hFontBodyNew);
        int textX = x + S(30) + S(18);
        const wchar_t* msg =
            L"AI summary is off — no Gemini API key is set up on this computer. "
            L"The statistical report above is unaffected.";
        RECT m = {textX, 0, x + width - S(30), 0};
        DrawTextW(hdc, msg, -1, &m, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
        int textH = m.bottom - m.top;
        int blockH = textH + S(8) + S(22);
        RECT rule = {x + S(30), y, x + S(33), y + blockH};
        HBRUSH rb = CreateSolidBrush(INFO_);
        FillRect(hdc, &rule, rb);
        DeleteObject(rb);
        SetTextColor(hdc, GRAY_400);
        RECT tr = {textX, y, x + width - S(30), y + textH};
        DrawTextW(hdc, msg, -1, &tr, DT_LEFT | DT_WORDBREAK);
        drawAiSetupLink(hdc, textX, y + textH + S(8));
        y += blockH + S(32);
        SelectObject(hdc, oldFont);
        return y - startY;
    }

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
            summaryText = L"AI summary unavailable: " + readableAIError(bp.error) +
                          L"\n\nThe statistical report above is unaffected — "
                          L"this is a garnish, not the product.";
            textColor = RISK_MOD;
        }
        // Report-sheet layout: no card — a 3px rule in the state's color
        // down the left of the message, like the success state below.
        SelectObject(hdc, g_hFontBodyNew);
        int textX = x + S(30) + S(18);
        RECT measureRect = {textX, y, x + width - S(30), y + 2000};
        DrawTextW(hdc, summaryText.c_str(), -1, &measureRect,
                  DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
        int textH = measureRect.bottom - measureRect.top;
        RECT rule = {x + S(30), y, x + S(33), y + textH};
        HBRUSH rb = CreateSolidBrush(bp.skipped ? INFO_ : RISK_MOD);
        FillRect(hdc, &rule, rb);
        DeleteObject(rb);
        SetTextColor(hdc, textColor);
        RECT textRect = {textX, y, x + width - S(30), y + textH};
        DrawTextW(hdc, summaryText.c_str(), -1, &textRect, DT_LEFT | DT_WORDBREAK);
        y += textH + S(32);
        SelectObject(hdc, oldFont);
        return y - startY;
    }

    // Success: Tier 2's cross-pair findings as a numbered list beside a
    // 3px gold rule (Report-sheet layout — no card). Each finding is laid
    // out via layoutFindingText(), which renders inline pair references
    // ("Pair N", or "NameA vs NameB" as a fallback — see
    // findAllPairLinks()) as individually clickable gold+underlined text
    // that deep-links into Flagged Pairs. Per-pair evidence (its own
    // separate deep-link target) also lives in each pair's card via
    // Tier 1 — see drawPairNarrative().
    SelectObject(hdc, g_hFontBodyNew);
    const int ruleX         = x + S(30);
    const int numX          = ruleX + S(18);
    const int findingIndent = S(26); // number column width
    const int findingGap    = S(14); // vertical space between findings
    const int itemX         = numX + findingIndent;
    const int itemMaxWidth  = (x + width - S(30)) - itemX;

    // First pass: measure total height across all findings. layoutFindingText
    // itself is the shared measure/draw helper, so this pass and the real
    // draw pass below can never disagree about height or line breaks.
    int findingsH = 0;
    for (size_t i = 0; i < bp.findings.size(); ++i) {
        std::wstring item = s2w(bp.findings[i]);
        auto matches = findAllPairLinks(item);
        int itemH = layoutFindingText(hdc, 0, 0, itemMaxWidth, item, matches,
                                        false, nullptr);
        findingsH += itemH;
        if (i + 1 < bp.findings.size()) findingsH += findingGap;
    }

    // Second pass: draw each finding with its number, plus any inline
    // pair-reference links within it.
    int fy = y;
    for (size_t i = 0; i < bp.findings.size(); ++i) {
        std::wstring item = s2w(bp.findings[i]);
        auto matches = findAllPairLinks(item);

        SetTextColor(hdc, GOLD_500);
        std::wstring num = std::to_wstring(i + 1);
        RECT numRect = {numX, fy, numX + findingIndent, fy + S(22)};
        DrawTextW(hdc, num.c_str(), -1, &numRect, DT_LEFT | DT_TOP | DT_SINGLELINE);

        int itemH = layoutFindingText(hdc, itemX, fy, itemMaxWidth, item,
                                        matches, true, &g_aiSummaryLinkRects);
        fy += itemH + findingGap;
    }

    // Attribution, aligned with the finding text, below a small gap.
    // The page footer carries the statistical-evidence disclaimer.
    int sigY = y + findingsH + S(14);
    SelectObject(hdc, g_hFontMonoSm);
    SetTextColor(hdc, GRAY_500);
    RECT sigRect = {itemX, sigY, x + width - S(30), sigY + S(18)};
    std::wstring sig = L"Gemini 2.5 Flash Lite  \u00B7  CALSS AI Instructor";
    // Say so when the findings cover only the top of the flagged list
    // (GEMINI_MAX_PAIRS cap), so they aren't read as covering every pair.
    if (r.aiPairsSent > 0 && r.aiPairsSent < (int)r.flaggedPairs.size())
        sig += L"  \u00B7  based on the " + std::to_wstring(r.aiPairsSent) +
               L" highest-scoring of " + std::to_wstring(r.flaggedPairs.size()) +
               L" flagged pairs";
    DrawTextW(hdc, sig.c_str(), -1, &sigRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    RECT rule = {ruleX, y, ruleX + S(3), sigY + S(18)};
    HBRUSH rb = CreateSolidBrush(GOLD_500);
    FillRect(hdc, &rule, rb);
    DeleteObject(rb);

    y = sigY + S(18) + S(32);
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

// Draw the page headline (Report-sheet layout). The run's timestamp and
// data folder already sit in the sticky context strip above, so this
// says what the run found instead of repeating where it came from: a
// one-sentence headline plus a sub-line whose exact-duplicates count is
// a gold link into the Flagged Pairs tab, pre-filtered (the click
// target formerly lived on the Exact Duplicates stat card).
int drawResultsHeader(HDC hdc, int x, int y, int width) {
    int startY = y;
    const AnalysisResults& r = g_analysisResults;
    SetBkMode(hdc, TRANSPARENT);

    int left  = x + S(30);
    int right = x + width - S(30);
    y += S(26);

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontH1);
    SetTextColor(hdc, WHITE_);
    std::wstring headline = r.flaggedCount > 0
        ? fmtCount(r.flaggedCount) + L" of " + fmtCount(r.totalPairs) +
          L" pairs flagged for review"
        : L"No pairs flagged across " + fmtCount(r.totalPairs) + L" pairs";
    RECT hR = {left, y, right, y + S(34)};
    DrawTextW(hdc, headline.c_str(), -1, &hR,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    y += S(38);

    SelectObject(hdc, g_hFontBodyNew);
    int lineH = S(22);
    std::wstring lead = fmtCount(r.highSimilarity) + L" high similarity  ·  ";
    SIZE leadSz; GetTextExtentPoint32W(hdc, lead.c_str(), (int)lead.size(), &leadSz);
    SetTextColor(hdc, GRAY_400);
    RECT leadR = {left, y, left + leadSz.cx, y + lineH};
    DrawTextW(hdc, lead.c_str(), -1, &leadR, DT_LEFT | DT_TOP | DT_SINGLELINE);

    int dupX = left + leadSz.cx;
    std::wstring dup = fmtCount(r.exactDuplicates) + L" exact duplicate" +
                       (r.exactDuplicates == 1 ? L"" : L"s");
    SIZE dupSz; GetTextExtentPoint32W(hdc, dup.c_str(), (int)dup.size(), &dupSz);
    RECT dupR = {dupX, y, dupX + dupSz.cx, y + lineH};
    if (r.exactDuplicates > 0) {
        // Same gold + underline treatment as the AI summary's pair links.
        SetTextColor(hdc, GOLD_400);
        DrawTextW(hdc, dup.c_str(), -1, &dupR, DT_LEFT | DT_TOP | DT_SINGLELINE);
        RECT underline = {dupX, y + lineH - S(2), dupX + dupSz.cx, y + lineH};
        HBRUSH ub = CreateSolidBrush(GOLD_400);
        FillRect(hdc, &underline, ub);
        DeleteObject(ub);
        g_exactDuplicatesCardRect = {dupR.left, dupR.top + g_scrollY,
                                     dupR.right, dupR.bottom + g_scrollY};
    } else {
        SetTextColor(hdc, GRAY_400);
        DrawTextW(hdc, dup.c_str(), -1, &dupR, DT_LEFT | DT_TOP | DT_SINGLELINE);
        g_exactDuplicatesCardRect = {};
    }
    y += lineH + S(22);

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