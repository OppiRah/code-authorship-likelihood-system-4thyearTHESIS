// ─────────────────────────────────────────────────────────────
// gui_students.cpp
// Students page — the Classes-grid block sub-tab bar, student card
// grid, and the per-student detail page (style DNA strip, feature
// comparison, flagged-appearance deep-links).
//
// Split out of gui.cpp unchanged (Checkpoint 3 of the ContentProc
// split — see CLAUDE.md). Three functions were already extern-
// prototyped in gui_common.h but still physically defined in
// gui.cpp, non-contiguous with each other (drawTabBar/drawContextStrip
// — SHARED chrome, not Students-specific — sit between
// drawStudentProfiles and drawStudentDetailPage in the original file):
// drawBlockTabBar, drawStudentProfiles, drawStudentDetailPage. Moved
// here verbatim, each cut separately.
//
// Four new functions consolidate ContentProc's Students-page logic
// that was scattered across FOUR different handlers, two of them easy
// to miss because the Students-specific chunk sits outside that
// handler's normal per-page dispatch:
//  - handleStudentsClick: WM_LBUTTONDOWN's four Students blocks
//    (block sub-tabs, sort control, flagged-appearance rows,
//    card/back/report) — return-true/false pattern, same as
//    handleOverviewClick/handleFlaggedPairsClick.
//  - drawStudentsPage: WM_PAINT's Students dispatch (the
//    back/report button-rect setup plus the detail/grid draw call) —
//    returns height used, same convention as every other page's
//    top-level draw function.
//  - handleStudentsDetailMouseMove: WM_MOUSEMOVE's DNA-hover chunk —
//    was mixed into an otherwise page-agnostic handler (its sibling
//    chunk there, empty-state-card hover, is unrelated and stays in
//    ContentProc).
//  - drawStudentDnaTooltip: WM_PAINT's DNA hover tooltip draw — was
//    in the shared-overlay section AFTER the main per-page
//    if/else-if, not inside it, so easy to miss on a first pass.
//
// g_dnaTooltipPos (was static) is the one global promoted to the
// gui_common.h contract for this checkpoint — verified by compiling
// this file standalone and resolving every "not declared" error, not
// assumed from the audit (Checkpoint 2 needed 27 symbols against an
// initial estimate of 3; this file's actual closure is documented
// below once verified).
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"

#ifdef _WIN32

// Moved from gui.cpp (Checkpoint 3 of the ContentProc split) —
// exclusively used by this file's Students-page rendering.
std::vector<std::string> g_blockTabNames;
RECT g_backButtonRect = {};

static DnaStripStyle dnaStyleDetail() {
    return { S(14), S(3), S(10), S(44), GOLD_500, RISK_HIGH };
}

// Draw one student profile card
// Counts how many flagged pairs a student appears in, and their
// highest score. This is the signal the old card was missing — you
// previously had to open a student to discover they appeared in
// eight flagged pairs.
struct StudentFlagSummary { int count; double maxScore; };

static StudentFlagSummary studentFlagSummary(const std::string& studentName) {
    StudentFlagSummary out { 0, 0.0 };
    std::string lower = studentName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    for (const auto& fp : g_analysisResults.flaggedPairs) {
        auto containsCI = [&](const std::string& h) {
            std::string hh = h;
            std::transform(hh.begin(), hh.end(), hh.begin(), ::tolower);
            return hh.find(lower) != std::string::npos;
        };
        if (containsCI(fp.filenameA) || containsCI(fp.filenameB)) {
            ++out.count;
            if (fp.combinedScore > out.maxScore) out.maxScore = fp.combinedScore;
        }
    }
    return out;
}

// Condenses the verbose "[Naming] Uses medium-length variable names"
// notes into short middot-joined phrases, two per line. The bracketed
// category prefix is redundant once the DNA strip shows grouping.
static std::vector<std::wstring> condenseStyleNotes(
    const std::vector<std::string>& notes, size_t maxLines)
{
    std::vector<std::wstring> phrases;
    for (const auto& n : notes) {
        std::string s = n;
        size_t close = s.find(']');
        if (close != std::string::npos && s[0] == '[')
            s = s.substr(close + 1);
        size_t first = s.find_first_not_of(' ');
        if (first != std::string::npos) s = s.substr(first);
        if (!s.empty()) phrases.push_back(s2w(s));
    }

    std::vector<std::wstring> lines;
    for (size_t i = 0; i < phrases.size() && lines.size() < maxLines; i += 2) {
        std::wstring line = phrases[i];
        if (i + 1 < phrases.size()) line += L"  \u00B7  " + phrases[i + 1];
        lines.push_back(line);
    }
    return lines;
}

static int drawStudentProfileCard(HDC hdc, int x, int y, int w,
                                    const StudentProfileSummary& profile)
{
    SetBkMode(hdc, TRANSPARENT);

    auto noteLines = condenseStyleNotes(profile.styleNotes, 2);
    StudentFlagSummary flags = studentFlagSummary(profile.name);

    // Fixed-height card. The old card sized itself to its bullet count
    // and reserved ~60px of dead space below the last bullet; the grid
    // read as unfinished. Every region below now has a job.
    const int PAD       = S(20);
    const int nameH     = S(26);
    const int dnaH      = S(18);
    const int noteH     = S(19);
    const int footerH   = S(28);
    int cardH = PAD + nameH + S(10) + dnaH + S(12)
              + (int)noteLines.size() * noteH + S(10)
              + footerH + S(6);

    RECT card = {x, y, x + w, y + cardH};
    drawCard(hdc, card, GRAY_850, GRAY_700, 10);

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontH2);
    int cy = y + PAD;

    // Name — white, not gold. Gold is reserved for eyebrows and
    // wayfinding; a heading is a heading.
    SetTextColor(hdc, WHITE_);
    RECT nameRect = {x + PAD, cy, x + w - S(92), cy + nameH};
    DrawTextW(hdc, s2w(profile.name).c_str(), -1, &nameRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Submission count — mono tabular, right aligned
    SelectObject(hdc, g_hFontMonoSm);
    SetTextColor(hdc, GRAY_400);
    std::wstring cnt = std::to_wstring(profile.submissionCount) +
                       (profile.submissionCount == 1 ? L" sub" : L" subs");
    RECT cntRect = {x + w - S(88), cy, x + w - PAD, cy + nameH};
    DrawTextW(hdc, cnt.c_str(), -1, &cntRect,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    cy += nameH + S(10);

    // Style DNA — the fingerprint, as a glanceable barcode
    drawDnaStrip(hdc, x + PAD, cy, profile.featureVector, nullptr, dnaStyleCard());
    cy += dnaH + S(12);

    // Condensed style notes
    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_200);
    for (const auto& line : noteLines) {
        RECT lr = {x + PAD, cy, x + w - PAD, cy + noteH};
        DrawTextW(hdc, line.c_str(), -1, &lr,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        cy += noteH;
    }
    cy += S(10);

    // Hairline
    HPEN sep = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldPen = (HPEN)SelectObject(hdc, sep);
    MoveToEx(hdc, x + PAD, cy, nullptr);
    LineTo(hdc, x + w - PAD, cy);
    SelectObject(hdc, oldPen);
    DeleteObject(sep);
    cy += S(8);

    // Footer — the previously-missing signal. Severity color is
    // earned here, not decorative.
    if (flags.count > 0) {
        COLORREF sev = (flags.maxScore >= 85.0) ? RISK_HIGH
                     : (flags.maxScore >= 70.0) ? RISK_MOD : RISK_LOW;
        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, sev);
        std::wstring flagTxt = L"\u2691  " + std::to_wstring(flags.count) +
            (flags.count == 1 ? L" flagged appearance" : L" flagged appearances");
        RECT fr = {x + PAD, cy, x + w - S(96), cy + S(20)};
        DrawTextW(hdc, flagTxt.c_str(), -1, &fr,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(hdc, g_hFontMonoSm);
        SetTextColor(hdc, sev);
        wchar_t maxBuf[32];
        swprintf(maxBuf, 32, L"max %.1f%%", flags.maxScore);
        RECT mr = {x + w - S(94), cy, x + w - PAD, cy + S(20)};
        DrawTextW(hdc, maxBuf, -1, &mr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    } else {
        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_500);
        RECT fr = {x + PAD, cy, x + w - PAD, cy + S(20)};
        DrawTextW(hdc, L"No flagged appearances", -1, &fr,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, oldFont);
    return cardH;
}

// Draw all student profiles section
// Determines which block/section a student belongs to by finding any
// of their files in the sync manifest and reading its block field.
// Returns "General" if no manifest entry is found (e.g. manually
// imported files never went through Classroom sync, so there's no
// block information available for them).
static std::string determineStudentBlock(
    const std::string& studentName,
    const std::map<std::string,SyncManifestEntry>& manifest)
{
    std::string lower = studentName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    for (const auto& kv : g_analysisResults.allPairs) {
        for (const std::string* fname : {&kv.filenameA, &kv.filenameB}) {
            std::string fl = *fname;
            std::transform(fl.begin(), fl.end(), fl.begin(), ::tolower);
            if (fl.find(lower) == std::string::npos) continue;

            auto hit = manifest.find(*fname + ".c");
            if (hit != manifest.end() && !hit->second.block.empty()) {
                return hit->second.block;
            }
        }
    }
    return "General";
}

// Draws the block sub-tab bar for the Classes page: "All" plus one tab
// per unique block found among current student profiles (e.g.
// "Programming 1 - BSIT Block 1"). Drawn at a FIXED position (like the
// main OVERVIEW/CLASSES/FLAGGED PAIRS bar) so it stays visible while
// the student grid below it scrolls. Rebuilds g_blockTabNames and
// g_blockTabRects fresh each call since the block list depends
// entirely on what's currently synced.
int drawBlockTabBar(HDC hdc, int x, int y, int width) {
    const AnalysisResults& r = g_analysisResults;
    std::map<std::string,SyncManifestEntry> manifest =
        gc_loadSyncManifest(toNarrowGC(g_state.dataFolder));

    std::set<std::string> uniqueBlocks;
    for (const auto& profile : r.profiles) {
        uniqueBlocks.insert(determineStudentBlock(profile.name, manifest));
    }
    g_blockTabNames.assign(uniqueBlocks.begin(), uniqueBlocks.end());
    // "General" (no block info) sorts last, real block names first
    std::sort(g_blockTabNames.begin(), g_blockTabNames.end(),
        [](const std::string& a, const std::string& b) {
            if (a == "General") return false;
            if (b == "General") return true;
            return a < b;
        });

    SetBkMode(hdc, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBodyNew);

    int btbH = S(BLOCK_TAB_BAR_HEIGHT);
    RECT barRect = {x, y, x + width, y + btbH};
    HBRUSH bgBr = CreateSolidBrush(GRAY_850);
    FillRect(hdc, &barRect, bgBr);
    DeleteObject(bgBr);

    HPEN botPen = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldBotPen = (HPEN)SelectObject(hdc, botPen);
    MoveToEx(hdc, x, y + btbH - 1, nullptr);
    LineTo(hdc, x + width, y + btbH - 1);
    SelectObject(hdc, oldBotPen);
    DeleteObject(botPen);

    g_blockTabRects.clear();

    int tabX = x + S(20);
    int tabPad = S(10);
    int tabH = btbH - S(10);
    int tabY = y + S(5);

    // Build the full chip list: "All" first (count = every student),
    // then each block name with its own student count.
    std::map<std::string,int> blockCounts;
    for (const auto& profile : r.profiles) {
        std::string b = determineStudentBlock(profile.name, manifest);
        blockCounts[b]++;
    }

    std::vector<std::pair<int,std::wstring>> chipItems; // {tabIdx, label}
    chipItems.push_back({-1, L"All"});
    for (size_t i = 0; i < g_blockTabNames.size(); ++i)
        chipItems.push_back({(int)i, s2w(g_blockTabNames[i])});

    for (const auto& item : chipItems) {
        int count = (item.first == -1)
            ? (int)r.profiles.size()
            : blockCounts[g_blockTabNames[item.first]];
        std::wstring fullLabel = item.second + L"  (" + std::to_wstring(count) + L")";

        RECT measureR = {0, 0, 0, 0};
        DrawTextW(hdc, fullLabel.c_str(), -1, &measureR, DT_CALCRECT | DT_SINGLELINE);
        int tabW = measureR.right - measureR.left + S(32);

        bool active = (g_currentBlockTab == item.first);
        RECT pillR = {tabX, tabY, tabX + tabW, tabY + tabH};

        // Pill chip: gray-850 rest, maroon-700 active fill, per spec
        // §5.8 ("pill, gray-850 rest, maroon-700 fill when active").
        drawCard(hdc, pillR, active ? MAROON_700 : GRAY_850,
                 active ? GOLD_500 : GRAY_700, tabH / 2);
        SetTextColor(hdc, active ? WHITE_ : GRAY_400);
        DrawTextW(hdc, fullLabel.c_str(), -1, &pillR,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        BlockTabRect hit;
        hit.x = tabX; hit.y = tabY; hit.w = tabW; hit.h = tabH;
        hit.tabIdx = item.first;
        g_blockTabRects.push_back(hit);
        addFocusable(g_contentFocusables, FocusKind::BlockChip,
                     RECT{hit.x, hit.y, hit.x + hit.w, hit.y + hit.h}, hit.tabIdx, false);

        tabX += tabW + tabPad;
    }

    SelectObject(hdc, oldFont);
    return btbH;
}

int drawStudentProfiles(HDC hdc, int x, int y, int width) {
    int startY = y;
    const AnalysisResults& r = g_analysisResults;

    // Clear previous hit regions
    g_studentCardRects.clear();
    // The detail page's DNA strip hit-rects would otherwise go stale
    // here — this function only ever runs when that strip isn't on
    // screen, so this is the one robust place to clear them regardless
    // of which of several navigation paths brought us back to the grid.
    g_dnaCellHits.clear();
    g_dnaHoveredCell = -1;

    SetBkMode(hdc, TRANSPARENT);

    // Large heading: "Student Style Profiles (N)"
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontTitle);
    SetTextColor(hdc, TEXT_MAIN);

    RECT hdrRect = {x + S(28), y + S(12), x + width - S(280), y + S(56)};
    std::wstring fullHdr = L"Student Style Profiles  ("
                         + std::to_wstring(r.profiles.size()) + L")";
    DrawTextW(hdc, fullHdr.c_str(), -1, &hdrRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Sort control (spec §5.8: "Add Sort ▾ (name / submissions /
    // flagged appearances)"). Cycles on click.
    SelectObject(hdc, g_hFontBodyNew);
    const wchar_t* sortLabels[3] = {
        L"Sort: Name \u25BE", L"Sort: Submissions \u25BE", L"Sort: Flagged \u25BE"
    };
    const wchar_t* sortLabel = sortLabels[g_classesSortMode];
    SIZE sortSz; GetTextExtentPoint32W(hdc, sortLabel, (int)wcslen(sortLabel), &sortSz);
    RECT sortR = {x + width - S(28) - sortSz.cx - S(20), y + S(16), x + width - S(28), y + S(40)};
    g_sortControlRect.left   = sortR.left;
    g_sortControlRect.top    = sortR.top + g_scrollY;
    g_sortControlRect.right  = sortR.right;
    g_sortControlRect.bottom = sortR.bottom + g_scrollY;
    drawCard(hdc, sortR, GRAY_800, GRAY_700, 6);
    SetTextColor(hdc, GRAY_200);
    DrawTextW(hdc, sortLabel, -1, &sortR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, TEXT_DIM);
    RECT hintRect = {x + S(28), y + S(30), x + width - S(300), y + S(56)};
    DrawTextW(hdc, L"Click any card to view full profile",
              -1, &hintRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    y += S(64);

    // Group students by block. Students are only grouped for DISPLAY
    // purposes here — this has no effect on scoring or pairwise
    // comparison, which still spans all blocks (a same-code submission
    // across two different sections is still real evidence worth
    // flagging, not noise to isolate away).
    std::map<std::string,SyncManifestEntry> syncManifest =
        gc_loadSyncManifest(toNarrowGC(g_state.dataFolder));

    // Live search filter (spec §5.8) — skip any profile whose name
    // doesn't contain the current search text. Empty filter = show all.
    auto passesSearch = [&](const std::string& name) {
        if (g_searchFilterLower.empty()) return true;
        std::wstring nameLower = s2w(name);
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::towlower);
        return nameLower.find(g_searchFilterLower) != std::wstring::npos;
    };

    // Flagged-appearance count per student, needed for the "Flagged"
    // sort mode. Computed once here rather than per-card.
    std::vector<int> flagCounts(r.profiles.size(), 0);
    for (size_t i = 0; i < r.profiles.size(); ++i)
        flagCounts[i] = studentFlagSummary(r.profiles[i].name).count;

    std::map<std::string, std::vector<int>> byBlock; // block name -> profile indices
    for (size_t i = 0; i < r.profiles.size(); ++i) {
        if (!passesSearch(r.profiles[i].name)) continue;
        std::string block = determineStudentBlock(r.profiles[i].name, syncManifest);
        byBlock[block].push_back((int)i);
    }

    // Apply sort mode within each block group.
    for (auto& kv : byBlock) {
        auto& indices = kv.second;
        switch (g_classesSortMode) {
            case SORT_NAME:
                std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                    return r.profiles[a].name < r.profiles[b].name;
                });
                break;
            case SORT_SUBMISSIONS:
                std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                    return r.profiles[a].submissionCount > r.profiles[b].submissionCount;
                });
                break;
            case SORT_FLAGGED:
                std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                    return flagCounts[a] > flagCounts[b];
                });
                break;
        }
    }

    int padding = S(20);
    int gridX = x + S(30);
    int gridW = width - S(60);
    int colW = (gridW - padding) / 2;

    // "General" (no block info) renders last, real block names first
    // and alphabetically, for a stable, predictable layout.
    std::vector<std::string> blockOrder;
    for (auto& kv : byBlock) if (kv.first != "General") blockOrder.push_back(kv.first);
    std::sort(blockOrder.begin(), blockOrder.end());
    if (byBlock.count("General")) blockOrder.push_back("General");

    // If a specific block sub-tab is selected (not "All"), only render
    // that one block's section — g_blockTabNames was populated by
    // drawBlockTabBar() immediately before this function runs, in the
    // same paint pass, so the index is guaranteed valid here.
    if (g_currentBlockTab >= 0 && g_currentBlockTab < (int)g_blockTabNames.size()) {
        std::string onlyBlock = g_blockTabNames[g_currentBlockTab];
        std::vector<std::string> filtered;
        for (const auto& b : blockOrder)
            if (b == onlyBlock) filtered.push_back(b);
        blockOrder = filtered;
    }

    if (blockOrder.empty() && !g_searchFilterLower.empty()) {
        // No results for this search — say so plainly rather than
        // rendering a silently empty grid.
        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_400);
        RECT emR = {x + S(30), y, x + width - S(30), y + S(40)};
        DrawTextW(hdc, L"No students match your search.", -1, &emR,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        y += S(50);
        SelectObject(hdc, oldFont);
        return y - startY;
    }

    for (const auto& blockName : blockOrder) {
        const auto& indices = byBlock[blockName];

        // Section header for this block
        SelectObject(hdc, g_hFontHeading);
        SetTextColor(hdc, UL_GOLD);
        RECT secR = {x + S(28), y, x + width - S(28), y + S(26)};
        std::wstring secLabel = s2w(blockName) + L"  (" +
            std::to_wstring(indices.size()) + L")";
        DrawTextW(hdc, secLabel.c_str(), -1, &secR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        y += S(24);

        HPEN secPen = CreatePen(PS_SOLID, 1, UL_GOLD);
        HPEN oldSecPen = (HPEN)SelectObject(hdc, secPen);
        MoveToEx(hdc, x + S(28), y, nullptr);
        LineTo(hdc, x + width - S(28), y);
        SelectObject(hdc, oldSecPen);
        DeleteObject(secPen);
        y += S(14);

        int rowHeight = 0;
        bool leftColumn = true;

        for (int idx : indices) {
            const auto& profile = r.profiles[idx];
            int cardX = leftColumn ? gridX : gridX + colW + padding;
            int h = drawStudentProfileCard(hdc, cardX, y, colW, profile);

            StudentCardRect hit;
            hit.x = cardX;
            hit.y = y;
            hit.w = colW;
            hit.h = h;
            hit.studentIdx = idx;
            g_studentCardRects.push_back(hit);
            // Note: g_studentCardRects stores y without +g_scrollY,
            // unlike every other content hit-rect vector in this
            // file — its own click-test (scrolledY comparison) is
            // therefore only correct at g_scrollY==0, a pre-existing
            // inconsistency out of scope for this pass. Computing our
            // own rect here rather than reusing hit.y keeps the focus
            // system's own scroll-into-view/ring math correct
            // regardless.
            addFocusable(g_contentFocusables, FocusKind::StudentCard,
                         RECT{cardX, y + g_scrollY, cardX + colW, y + g_scrollY + h},
                         idx, true);

            if (h > rowHeight) rowHeight = h;

            if (!leftColumn) {
                y += rowHeight + padding;
                rowHeight = 0;
            }
            leftColumn = !leftColumn;
        }

        if (!leftColumn) {
            y += rowHeight + padding;
        }
        y += S(20); // gap before next block section
    }

    y += S(10);
    SelectObject(hdc, oldFont);
    return y - startY;
}

// ── Student detail page ──────────────────────────────────────
int drawStudentDetailPage(HDC hdc, int x, int y, int width,
                                   int studentIdx)
{
    const AnalysisResults& r = g_analysisResults;
    if (studentIdx < 0 || studentIdx >= (int)r.profiles.size()) return 0;
    const auto& profile = r.profiles[studentIdx];

    int startY = y;
    SetBkMode(hdc, TRANSPARENT);
    g_flaggedAppearanceRowRects.clear();

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBodyNew);

    // ── TOP ROW: Back link + title + Generate button ──────────
    SetTextColor(hdc, GOLD_500);
    RECT backRect = {x + S(28), y + S(16), x + S(220), y + S(38)};
    DrawTextW(hdc, L"\u2190  Back to Classes", -1, &backRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    // Note: g_backButtonRect (used by the mouse click handler) is set
    // separately in WM_PAINT using a fixed pageContentStartY-based
    // value rather than this actually-drawn backRect, so it silently
    // stops matching once the page is scrolled — a pre-existing bug
    // out of scope here. Building our own focus rect straight from
    // backRect (with the same +g_scrollY convention as every other
    // content item) keeps the focus system correct regardless.
    addFocusable(g_contentFocusables, FocusKind::BackButton,
                 RECT{backRect.left, backRect.top + g_scrollY,
                      backRect.right, backRect.bottom + g_scrollY},
                 0, true);

    // Generate + Export sit together top-right, matching spec's
    // "Generate similarity report stays top-right, next to the
    // global Export"
    int btnW = S(260); int btnH = S(34);
    int btnX = x + width - btnW - S(28);
    int btnY_local = y + S(12);
    RECT btnRect = {btnX, btnY_local, btnX + btnW, btnY_local + btnH};

    g_reportButtonRect.left   = btnX;
    g_reportButtonRect.top    = btnY_local + g_scrollY;
    g_reportButtonRect.right  = btnX + btnW;
    g_reportButtonRect.bottom = btnY_local + btnH + g_scrollY;
    addFocusable(g_contentFocusables, FocusKind::ReportButton, g_reportButtonRect, 0, true);

    drawCard(hdc, btnRect, GRAY_800, GRAY_700, 8);
    SetTextColor(hdc, GRAY_200);
    DrawTextW(hdc, L"\U0001F4C4  Generate Similarity Report", -1, &btnRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    y += S(50);

    // ── STUDENT NAME + SUBTITLE ───────────────────────────────
    SelectObject(hdc, g_hFontDisplay);
    SetTextColor(hdc, WHITE_);
    RECT nameRect = {x + S(28), y, x + width - S(28), y + S(48)};
    DrawTextW(hdc, s2w(profile.name).c_str(), -1, &nameRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    y += S(50);

    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_400);
    RECT subRect = {x + S(28), y, x + width - S(28), y + S(20)};
    std::wstring sub = L"Style profile built from ";
    sub += std::to_wstring(profile.submissionCount);
    sub += L" submission";
    if (profile.submissionCount != 1) sub += L"s";
    DrawTextW(hdc, sub.c_str(), -1, &subRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    y += S(30);

    // ── STYLE DNA STRIP — full-width, labeled (spec §5.8: "Style
    // DNA strip full-width at the top under the name") ───────────
    drawEyebrow(hdc, x + S(28), y, width - S(56), L"STYLE DNA");
    y += S(22);
    {
        DnaStripStyle detailStyle = dnaStyleDetail();
        int stripW = width - S(56);
        // Stretch the fixed-cell-width detail style to fill the
        // available width proportionally, so it reads as a genuine
        // full-width strip rather than a fixed-size one floating
        // in extra space.
        int naturalW = dnaStripWidth(detailStyle);
        if (naturalW > 0 && naturalW < stripW) {
            float scale = (float)stripW / (float)naturalW;
            detailStyle.cellW    = (int)(detailStyle.cellW * scale);
            detailStyle.cellGap  = (int)(detailStyle.cellGap * scale);
            detailStyle.groupGap = (int)(detailStyle.groupGap * scale);
        }
        drawDnaStrip(hdc, x + S(28), y, profile.featureVector, nullptr, detailStyle, true);
        y += detailStyle.height + S(20);
    }

    // Gold divider
    HBRUSH lineBr = CreateSolidBrush(GOLD_500);
    RECT line = {x + S(28), y, x + width - S(28), y + 1};
    FillRect(hdc, &line, lineBr);
    DeleteObject(lineBr);
    y += S(18);

    // ── COLLECT DATA ─────────────────────────────────────────
    std::string slower = profile.name;
    for (auto& c : slower) c = (char)tolower((unsigned char)c);
    auto containsCI = [&](const std::string& h, const std::string& n) {
        std::string hh = h;
        for (auto& c : hh) c = (char)tolower((unsigned char)c);
        return hh.find(n) != std::string::npos;
    };

    std::vector<std::string> files;
    std::set<std::string> seenFiles;
    for (const auto& pair : g_analysisResults.allPairs) {
        if (containsCI(pair.filenameA, slower))
            if (seenFiles.insert(pair.filenameA).second) files.push_back(pair.filenameA);
        if (containsCI(pair.filenameB, slower))
            if (seenFiles.insert(pair.filenameB).second) files.push_back(pair.filenameB);
    }

    // Load the Classroom sync manifest (if any) so we can show the
    // instructor-facing assignment title ("Midterm Exam") instead of
    // the internally-encoded filename used for parsing. Files that
    // weren't synced via Classroom (manually imported) simply won't
    // have an entry and fall back to showing the raw filename.
    std::map<std::string,SyncManifestEntry> syncManifest =
        gc_loadSyncManifest(toNarrowGC(g_state.dataFolder));

    std::vector<std::pair<int,const PairAnalysisDisplay*>> related; // {flaggedPairs index, ptr}
    for (int fpi = 0; fpi < (int)g_analysisResults.flaggedPairs.size(); ++fpi) {
        const auto& fp = g_analysisResults.flaggedPairs[fpi];
        if (containsCI(fp.filenameA, slower) || containsCI(fp.filenameB, slower))
            related.push_back({fpi, &fp});
    }

    // ── 3-COLUMN CARD LAYOUT ─────────────────────────────────
    int gap     = S(14);
    int colBase = x + S(28);
    int totalW  = width - S(56);
    int c1W     = (int)(totalW * 0.38);
    int c2W     = (int)(totalW * 0.26);
    int c3W     = totalW - c1W - c2W - gap * 2;
    int c1X     = colBase;
    int c2X     = colBase + c1W + gap;
    int c3X     = colBase + c1W + c2W + gap * 2;

    // Calculate max column height
    int notesH  = (int)profile.styleNotes.size() * S(26) + S(50);
    if (notesH < S(100)) notesH = S(100);
    int filesH  = (int)files.size() * S(24) + S(50);
    if (filesH < S(80)) filesH = S(80);
    int flagsH  = (int)related.size() * S(34) + S(50);
    if (flagsH < S(80)) flagsH = S(80);
    int colH    = notesH > filesH ? (notesH > flagsH ? notesH : flagsH)
                                  : (filesH > flagsH ? filesH : flagsH);

    // ── COL 1: Style Fingerprint ──────────────────────────────
    RECT c1Card = {c1X, y, c1X + c1W, y + colH};
    drawCard(hdc, c1Card, GRAY_850, GRAY_700, 10);

    // Card header
    drawEyebrow(hdc, c1X + S(14), y + S(12), c1W - S(28), L"STYLE FINGERPRINT");
    HPEN c1Sep = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldC1 = (HPEN)SelectObject(hdc, c1Sep);
    MoveToEx(hdc, c1X + S(14), y + S(33), nullptr);
    LineTo(hdc, c1X + c1W - S(14), y + S(33));
    SelectObject(hdc, oldC1); DeleteObject(c1Sep);

    int ny = y + S(40);
    SelectObject(hdc, g_hFontBodyNew);
    if (profile.styleNotes.empty()) {
        SetTextColor(hdc, GRAY_400);
        RECT emR = {c1X + S(14), ny, c1X + c1W - S(14), ny + S(40)};
        DrawTextW(hdc, L"No style notes available.", -1, &emR,
                  DT_LEFT | DT_WORDBREAK);
    } else {
        for (const auto& note : profile.styleNotes) {
            SetTextColor(hdc, GOLD_500);
            RECT dotR = {c1X + S(14), ny, c1X + S(26), ny + S(24)};
            DrawTextW(hdc, L"\u2022", -1, &dotR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SetTextColor(hdc, WHITE_);
            RECT nR = {c1X + S(26), ny, c1X + c1W - S(14), ny + S(24)};
            DrawTextW(hdc, s2w(note).c_str(), -1, &nR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_WORDBREAK);
            ny += S(26);
        }
    }

    // ── COL 2: Submissions Used ───────────────────────────────
    RECT c2Card = {c2X, y, c2X + c2W, y + colH};
    drawCard(hdc, c2Card, GRAY_850, GRAY_700, 10);

    drawEyebrow(hdc, c2X + S(14), y + S(12), c2W - S(28), L"SUBMISSIONS USED");
    HPEN c2Sep = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldC2 = (HPEN)SelectObject(hdc, c2Sep);
    MoveToEx(hdc, c2X + S(14), y + S(33), nullptr);
    LineTo(hdc, c2X + c2W - S(14), y + S(33));
    SelectObject(hdc, oldC2); DeleteObject(c2Sep);

    int fy = y + S(40);
    SelectObject(hdc, g_hFontBodyNew);
    if (files.empty()) {
        SetTextColor(hdc, GRAY_400);
        RECT emR = {c2X + S(14), fy, c2X + c2W - S(14), fy + S(40)};
        DrawTextW(hdc, L"No files captured.", -1, &emR, DT_LEFT | DT_WORDBREAK);
    } else {
        for (const auto& fname : files) {
            SetTextColor(hdc, GRAY_200);
            RECT fR = {c2X + S(14), fy, c2X + c2W - S(14), fy + S(22)};

            // Look up the human-readable assignment title if this file
            // came through Classroom sync; otherwise show the raw
            // filename (e.g. for manually imported files).
            auto manifestHit = syncManifest.find(fname + ".c");
            std::wstring displayLabel = (manifestHit != syncManifest.end())
                ? s2w(manifestHit->second.title)
                : s2w(fname);

            DrawTextW(hdc, displayLabel.c_str(), -1, &fR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            fy += S(24);
        }
    }

    // ── COL 3: Flagged Appearances ────────────────────────────
    RECT c3Card = {c3X, y, c3X + c3W, y + colH};
    drawCard(hdc, c3Card, GRAY_850, GRAY_700, 10);

    drawEyebrow(hdc, c3X + S(14), y + S(12), c3W - S(28), L"FLAGGED APPEARANCES");
    HPEN c3Sep = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldC3 = (HPEN)SelectObject(hdc, c3Sep);
    MoveToEx(hdc, c3X + S(14), y + S(33), nullptr);
    LineTo(hdc, c3X + c3W - S(14), y + S(33));
    SelectObject(hdc, oldC3); DeleteObject(c3Sep);

    int ry = y + S(40);
    if (related.empty()) {
        SetTextColor(hdc, RISK_LOW);
        SelectObject(hdc, g_hFontBodyNew);
        RECT emR = {c3X + S(14), ry, c3X + c3W - S(14), ry + S(60)};
        DrawTextW(hdc, L"No flagged pairs. Submissions appear consistent.",
                  -1, &emR, DT_LEFT | DT_WORDBREAK);
    } else {
        for (const auto& item : related) {
            int flaggedIdx = item.first;
            const auto* pair = item.second;
            COLORREF sev = pairSeverityColor(pair->combinedScore);
            const wchar_t* sevLabel = pairSeverityLabel(pair->combinedScore);

            int rowH = S(34);
            RECT rowR = {c3X + S(12), ry, c3X + c3W - S(12), ry + rowH - S(2)};

            bool hovered = false; // hover-state tracking omitted for
                                    // this pass; click works regardless
            HBRUSH rowBg = CreateSolidBrush(GRAY_800);
            FillRect(hdc, &rowR, rowBg);
            DeleteObject(rowBg);

            // Severity pill, left
            SelectObject(hdc, g_hFontMonoSm);
            SIZE pillSz; GetTextExtentPoint32W(hdc, sevLabel, (int)wcslen(sevLabel), &pillSz);
            RECT pillR = {rowR.left + S(6), rowR.top + S(5), rowR.left + S(6) + pillSz.cx + S(16), rowR.bottom - S(5)};
            HBRUSH pb = CreateSolidBrush(sev);
            HPEN pp = CreatePen(PS_SOLID, 1, sev);
            HBRUSH ob = (HBRUSH)SelectObject(hdc, pb);
            HPEN op = (HPEN)SelectObject(hdc, pp);
            RoundRect(hdc, pillR.left, pillR.top, pillR.right, pillR.bottom, S(8), S(8));
            SelectObject(hdc, ob); SelectObject(hdc, op);
            DeleteObject(pb); DeleteObject(pp);
            SetTextColor(hdc, WHITE_);
            DrawTextW(hdc, sevLabel, -1, &pillR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Score, right — mono tabular per spec §4.2
            SetTextColor(hdc, sev);
            RECT scR = {c3X + c3W - S(72), rowR.top, c3X + c3W - S(14), rowR.bottom};
            DrawTextW(hdc, fmtPct(pair->combinedScore).c_str(), -1, &scR,
                      DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

            // Filenames, middle-truncated, between pill and score
            SelectObject(hdc, g_hFontMonoSm);
            SetTextColor(hdc, GRAY_400);
            RECT vsR = {pillR.right + S(10), rowR.top, scR.left - S(8), rowR.bottom};
            std::wstring vs = s2w(pair->filenameA) + L" vs " + s2w(pair->filenameB);
            std::wstring vsTrunc = truncateMiddle(hdc, vs, vsR.right - vsR.left);
            DrawTextW(hdc, vsTrunc.c_str(), -1, &vsR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            FlaggedAppearanceRowRect hit;
            hit.r = { rowR.left, rowR.top + g_scrollY, rowR.right, rowR.bottom + g_scrollY };
            hit.flaggedPairIndex = flaggedIdx;
            g_flaggedAppearanceRowRects.push_back(hit);
            addFocusable(g_contentFocusables, FocusKind::FlaggedAppearanceRow,
                         hit.r, hit.flaggedPairIndex, true);

            ry += rowH;
        }
    }

    y += colH + S(30);

    // Compliance fix item 8: this page shows a per-student authorship
    // score (in the third panel above) and had no disclaimer anywhere.
    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_500);
    RECT detailDiscRect = {x + S(28), y, x + width - S(28), y + S(20)};
    DrawTextW(hdc, L"Statistical evidence only \u00B7 determinations rest with the instructor",
              -1, &detailDiscRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    y += S(24);

    SelectObject(hdc, oldFont);
    return y - startY;
}

// Students page click handling — block sub-tab bar (Classes grid view
// only), sort control (grid only), flagged-appearance row deep-links
// (detail view only), card click / back button / report button.
// Returns true if the click was handled (caller should return 0
// without falling through), false otherwise. Moved out of
// ContentProc's WM_LBUTTONDOWN — the four original blocks were NOT
// contiguous there (interleaved with the cancel-analysis/empty-state
// global blocks and the already-extracted Overview/Flagged Pairs
// calls); see CLAUDE.md for the resulting reordering and why it's
// safe. scrolledY is computed locally (my + g_scrollY) rather than
// taken as a parameter, since the call site in ContentProc sits
// before that computation now happens there.
bool handleStudentsClick(HWND hwnd, int mx, int my) {
    int scrolledY = my + g_scrollY;

    // Block sub-tab click? (Classes page grid view only — fixed Y
    // zone right below the main tab bar, not scrolled.)
    if (g_selectedStudent < 0 &&
        my >= S(4) + S(TAB_BAR_HEIGHT) + S(CONTEXT_STRIP_HEIGHT) &&
        my < S(4) + S(TAB_BAR_HEIGHT) + S(CONTEXT_STRIP_HEIGHT) + S(BLOCK_TAB_BAR_HEIGHT))
    {
        for (const auto& tab : g_blockTabRects) {
            if (mx >= tab.x && mx < tab.x + tab.w &&
                my >= tab.y && my < tab.y + tab.h)
            {
                selectBlockTab(hwnd, tab.tabIdx);
                return true;
            }
        }
    }

    // Classes grid: Sort control click — cycles Name → Submissions →
    // Flagged → Name (spec §5.8).
    if (g_selectedStudent < 0 &&
        mx >= g_sortControlRect.left && mx < g_sortControlRect.right &&
        scrolledY >= g_sortControlRect.top && scrolledY < g_sortControlRect.bottom)
    {
        g_classesSortMode = (ClassesSortMode)((g_classesSortMode + 1) % 3);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

    // Student detail page: Flagged Appearances rows are clickable,
    // deep-linking to Flagged Pairs (spec §5.8). Auto-expands the
    // target pair (step 8's Flagged Pairs expand/collapse model).
    if (g_selectedStudent >= 0 && !g_flaggedAppearanceRowRects.empty()) {
        for (const auto& row : g_flaggedAppearanceRowRects) {
            if (mx >= row.r.left && mx < row.r.right &&
                scrolledY >= row.r.top && scrolledY < row.r.bottom)
            {
                jumpToFlaggedPair(hwnd, row.flaggedPairIndex);
                return true;
            }
        }
    }

    // Card click → open detail, OR back button/report button in
    // detail view.
    if (g_selectedStudent >= 0) {
        // In detail view, check for back button
        if (mx >= g_backButtonRect.left && mx < g_backButtonRect.right &&
            scrolledY >= g_backButtonRect.top &&
            scrolledY < g_backButtonRect.bottom)
        {
            closeStudentDetail(hwnd);
            return true;
        }
        // Check for "Generate Similarity Report" button
        if (mx >= g_reportButtonRect.left && mx < g_reportButtonRect.right &&
            scrolledY >= g_reportButtonRect.top &&
            scrolledY < g_reportButtonRect.bottom)
        {
            generateAndOpenStudentReport();
            return true;
        }
    } else {
        // In grid view, check for card clicks
        for (const auto& card : g_studentCardRects) {
            if (mx >= card.x && mx < card.x + card.w &&
                scrolledY >= card.y && scrolledY < card.y + card.h)
            {
                openStudentDetail(hwnd, card.studentIdx);
                return true;
            }
        }
    }

    return false;
}

// WM_PAINT's Students dispatch — button-rect setup (detail view only)
// plus the detail-page/grid draw call. Returns height used, same
// convention as every other page's top-level draw function
// (drawFlaggedPairs, drawStatsDashboard, etc.). pageContentStartY is
// the un-scrolled Y the fixed header ends at (needed for the button
// rects, which are NOT relative to the scrolled contentY); contentY
// is contentX/contentY/contentW's usual scrolled position, passed
// through to drawStudentDetailPage/drawStudentProfiles unchanged.
int drawStudentsPage(HDC hdc, int contentX, int contentY, int contentW,
                      int pageContentStartY) {
    if (g_selectedStudent >= 0) {
        g_backButtonRect.left   = contentX + 30;
        g_backButtonRect.top    = pageContentStartY + 15;
        g_backButtonRect.right  = contentX + 250;
        g_backButtonRect.bottom = pageContentStartY + 40;
        // Report button — right side, ~280px wide, 36px tall, y+8
        int btnW = 280;
        int btnH = 36;
        g_reportButtonRect.left   = contentX + contentW - btnW - 30;
        g_reportButtonRect.top    = pageContentStartY + 8;
        g_reportButtonRect.right  = contentX + contentW - 30;
        g_reportButtonRect.bottom = pageContentStartY + 8 + btnH;
        return drawStudentDetailPage(hdc, contentX, contentY, contentW,
                                      g_selectedStudent);
    } else {
        int used = drawStudentProfiles(hdc, contentX, contentY, contentW);
        for (auto& card : g_studentCardRects) card.y -= 0;
        return used;
    }
}

// Style DNA strip hover (spec §4.4: detail-page strip is "interactive
// on hover"). Guarded on g_selectedStudent rather than just checking
// the hit buffer non-empty — that buffer can go briefly stale when
// navigating away from the detail page via a path that doesn't
// repaint it (e.g. switching tabs directly), so this is the one check
// guaranteed correct regardless of which navigation path was taken.
// Moved out of ContentProc's WM_MOUSEMOVE, which is otherwise
// page-agnostic — its other chunk (empty-state card hover) is
// unrelated and stays there.
void handleStudentsDetailMouseMove(HWND hwnd, int mx, int my) {
    if (g_selectedStudent >= 0 && !g_dnaCellHits.empty()) {
        int found = -1;
        for (size_t i = 0; i < g_dnaCellHits.size(); ++i) {
            const auto& c = g_dnaCellHits[i];
            if (mx >= c.r.left && mx < c.r.right &&
                my >= c.r.top  && my < c.r.bottom) { found = (int)i; break; }
        }
        if (found != g_dnaHoveredCell) {
            g_dnaHoveredCell = found;
            g_dnaTooltipPos = { mx, my };
            InvalidateRect(hwnd, nullptr, FALSE);
        } else if (found >= 0) {
            // Still hovering the same cell — keep the tooltip
            // position tracking the cursor.
            g_dnaTooltipPos = { mx, my };
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    }
}

// Style DNA hover tooltip (spec §4.4). Moved out of ContentProc's
// WM_PAINT shared-overlay section — it sits AFTER the main per-page
// if/else-if there (alongside drawToasts/drawAppModal/
// drawHelpCarousel), not inside it, so it's easy to miss on a first
// read. Extracted in place, no reordering relative to its neighbors.
void drawStudentDnaTooltip(HDC hdc, const RECT& rect) {
    if (g_selectedStudent >= 0 &&
        g_dnaHoveredCell >= 0 && g_dnaHoveredCell < (int)g_dnaCellHits.size())
    {
        const auto& hit = g_dnaCellHits[g_dnaHoveredCell];
        SetBkMode(hdc, TRANSPARENT);
        HFONT ttFont = (HFONT)SelectObject(hdc, g_hFontBodyNew);

        wchar_t pctBuf[16];
        swprintf(pctBuf, 16, L"%.0f%%", hit.value * 100.0);
        std::wstring ttText = hit.name + L"  \u00B7  " + pctBuf;

        SIZE ttSz;
        GetTextExtentPoint32W(hdc, ttText.c_str(), (int)ttText.length(), &ttSz);
        int ttPad = 10;
        int ttW = ttSz.cx + ttPad * 2;
        int ttH = ttSz.cy + ttPad;

        // Position above the cursor, clamped so it never runs off the
        // top or right edge of the content area.
        int ttX = g_dnaTooltipPos.x - ttW / 2;
        int ttY = g_dnaTooltipPos.y - ttH - 12;
        if (ttX < 4) ttX = 4;
        if (ttX + ttW > rect.right - 4) ttX = rect.right - 4 - ttW;
        if (ttY < 4) ttY = g_dnaTooltipPos.y + 16; // flip below if no room above

        RECT ttRect = { ttX, ttY, ttX + ttW, ttY + ttH };
        HBRUSH ttBg = CreateSolidBrush(GRAY_950);
        HPEN   ttPen = CreatePen(PS_SOLID, 1, GOLD_500);
        HBRUSH oldTtBg = (HBRUSH)SelectObject(hdc, ttBg);
        HPEN   oldTtPen = (HPEN)SelectObject(hdc, ttPen);
        RoundRect(hdc, ttRect.left, ttRect.top, ttRect.right, ttRect.bottom, 6, 6);
        SelectObject(hdc, oldTtBg);
        SelectObject(hdc, oldTtPen);
        DeleteObject(ttBg);
        DeleteObject(ttPen);

        SetTextColor(hdc, WHITE_);
        DrawTextW(hdc, ttText.c_str(), -1, &ttRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, ttFont);
    }
}

#endif // _WIN32
