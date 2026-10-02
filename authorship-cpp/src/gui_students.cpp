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

// ── Flagged Appearances panel scroll (student detail page) ─────────
// A student can appear in dozens of flagged pairs; drawing every row
// made the detail page very long. The panel now shows at most
// FA_MAX_VISIBLE_ROWS rows in its own viewport with its own scrollbar
// (mouse wheel over it, a draggable thumb, click-on-track to page).
// Rects below are window-space as drawn this frame.
static const int FA_MAX_VISIBLE_ROWS = 10;
static int  g_faScrollY      = 0;
static int  g_faScrollMax    = 0;
static int  g_faStudent      = -1;   // scroll resets when the student changes
static RECT g_faListRect     = {};   // row viewport
static RECT g_faTrackRect    = {};   // scrollbar track (empty = no scrollbar)
static RECT g_faThumbRect    = {};
static bool g_faDragging     = false;
static int  g_faDragStartY   = 0;
static int  g_faDragStartScroll = 0;

static void faSetScroll(HWND hwnd, int v) {
    if (v < 0) v = 0;
    if (v > g_faScrollMax) v = g_faScrollMax;
    if (v != g_faScrollY) {
        g_faScrollY = v;
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

// Mouse wheel over the panel scrolls the panel; at either end the
// wheel falls through to the page (returns false) so it never traps.
bool handleStudentsWheel(HWND hwnd, int mx, int my, int delta) {
    if (g_selectedStudent < 0 || g_faScrollMax <= 0) return false;
    RECT area = g_faListRect;
    if (g_faTrackRect.right > area.right) area.right = g_faTrackRect.right;
    if (mx < area.left || mx >= area.right || my < area.top || my >= area.bottom) return false;
    int step = (delta / WHEEL_DELTA) * S(34) * 3;
    int want = g_faScrollY - step;
    if ((want < 0 && g_faScrollY == 0) || (want > g_faScrollMax && g_faScrollY == g_faScrollMax))
        return false;
    faSetScroll(hwnd, want);
    return true;
}

void handleStudentsMouseUp() {
    if (g_faDragging) {
        g_faDragging = false;
        ReleaseCapture();
        if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
    }
}

static DnaStripStyle dnaStyleDetail() {
    return { S(14), S(3), S(10), S(44), GOLD_500, RISK_HIGH };
}

// Draw one student profile card
// Counts how many flagged pairs a student appears in, and their
// highest score. This is the signal the old card was missing — you
// previously had to open a student to discover they appeared in
// eight flagged pairs.
struct StudentFlagSummary { int count; double maxScore; };

// ── Per-run Classes cache (scale perf fix A+B) ───────────────────
// Everything the Classes grid needs per student that depends only on
// the analysis results + sync manifest, computed ONCE per run instead
// of on every paint. At 160 students / 6,481 flagged pairs the old
// per-paint versions (a flagged-pairs scan per card plus again for
// the sort counts, an allPairs scan per student ×3, and two manifest
// disk reads) cost ~800ms per frame — see the 2026-09-29 scale
// profiling. Parallel to g_analysisResults.profiles by index.
//
// Students are matched EXACTLY by the pipeline's own student identity
// (AuthorshipDisplay::studentName for flagged pairs, studentNameForFile
// for allPairs filenames — both extractStudentName in main.cpp), not
// by case-insensitive substring as before, which counted "LuisDizon"'s
// pairs toward a student named "Luis" too.
//
// Invalidated explicitly at WM_ANALYSIS_COMPLETE; also rebuilds if the
// data folder or result sizes no longer match (e.g. a Classroom sync
// switched g_state.dataFolder before its re-run finished) — same
// "reflects the current folder" behavior the per-paint reads had.
struct StudentsTabCache {
    bool valid = false;
    std::wstring folder;
    size_t profilesN = 0, flaggedN = 0, allPairsN = 0;
    std::map<std::string,SyncManifestEntry> manifest;
    std::vector<StudentFlagSummary> flags;
    std::vector<std::string> blocks;
};
static StudentsTabCache g_studentsCache;

void invalidateStudentsTabCache() { g_studentsCache.valid = false; }

static const StudentsTabCache& studentsTabCache() {
    StudentsTabCache& c = g_studentsCache;
    const AnalysisResults& r = g_analysisResults;
    if (c.valid && c.folder == g_state.dataFolder && c.profilesN == r.profiles.size() &&
        c.flaggedN == r.flaggedPairs.size() && c.allPairsN == r.allPairs.size())
        return c;

    c.valid     = true;
    c.folder    = g_state.dataFolder;
    c.profilesN = r.profiles.size();
    c.flaggedN  = r.flaggedPairs.size();
    c.allPairsN = r.allPairs.size();
    c.manifest  = gc_loadSyncManifest(toNarrowGC(g_state.dataFolder));
    c.flags.assign(r.profiles.size(), StudentFlagSummary{0, 0.0});
    c.blocks.assign(r.profiles.size(), "General");

    std::unordered_map<std::string, int> profileIdx;
    for (size_t i = 0; i < r.profiles.size(); ++i) profileIdx[r.profiles[i].name] = (int)i;

    // Flagged-appearance count + highest score per student. The old
    // card footer's signal — a student previously had to be opened to
    // discover they appeared in eight flagged pairs.
    for (const auto& fp : r.flaggedPairs) {
        for (const std::string* who : {&fp.studentA.studentName, &fp.studentB.studentName}) {
            if (who == &fp.studentB.studentName && *who == fp.studentA.studentName) break;
            auto it = profileIdx.find(*who);
            if (it == profileIdx.end()) continue;
            StudentFlagSummary& s = c.flags[it->second];
            ++s.count;
            if (fp.combinedScore > s.maxScore) s.maxScore = fp.combinedScore;
        }
    }

    // Block/section per student: the block of the first of their
    // compared files (allPairs order, A before B) that has a non-empty
    // manifest block — the same first-hit order the old per-paint
    // determineStudentBlock scan used. Students with no manifest entry
    // (e.g. manually imported files never went through Classroom sync)
    // stay "General".
    std::vector<char> assigned(r.profiles.size(), 0);
    std::unordered_map<std::string, int> fileOwner; // filename -> profile idx, -1 = none
    for (const auto& row : r.allPairs) {
        for (const std::string* fname : {&row.filenameA, &row.filenameB}) {
            auto fo = fileOwner.find(*fname);
            if (fo == fileOwner.end()) {
                auto pi = profileIdx.find(studentNameForFile(*fname));
                fo = fileOwner.emplace(*fname, pi == profileIdx.end() ? -1 : pi->second).first;
            }
            int idx = fo->second;
            if (idx < 0 || assigned[idx]) continue;
            auto hit = c.manifest.find(*fname + ".c");
            if (hit != c.manifest.end() && !hit->second.block.empty()) {
                c.blocks[idx] = hit->second.block;
                assigned[idx] = 1;
            }
        }
    }
    return c;
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

// ── Roster layout (Classes grid) ─────────────────────────────────
// One full-width row per student, like a class roster, instead of a
// two-column grid of card boxes: name + submission count, Style DNA
// strip, the two condensed habit lines, and the flagged-appearance
// count/max on the right. Rows are separated by hairlines; the hovered
// row gets a gray-850 fill and a 3px gold bar, so each student still
// reads as its own unit without a box around it.
static const int ROSTER_ROW_HEIGHT    = 76;
static const int ROSTER_HEADER_HEIGHT = 34;

static int g_rosterHoverIdx = -1;  // profile index under the cursor, -1 = none

static DnaStripStyle dnaStyleRoster() {
    return { S(7), S(2), S(8), S(24), GRAY_400, RISK_HIGH };
}

struct RosterCols { int left, right, nameX, nameW, dnaX, dnaW, habitsX, habitsW, flagX; };

// Column positions for a given content width. The habits column is the
// one that gives way on narrow windows (dropped below ~150px) so the
// name, DNA and flagged columns never clip.
static RosterCols rosterColumns(int x, int width) {
    RosterCols c;
    c.left  = x + S(30);
    c.right = x + width - S(30);
    int W = c.right - c.left;
    c.nameX = c.left + S(18);
    c.nameW = W * 26 / 100;
    if (c.nameW < S(170)) c.nameW = S(170);
    if (c.nameW > S(300)) c.nameW = S(300);
    c.dnaX  = c.nameX + c.nameW + S(16);
    c.dnaW  = S(150);
    c.flagX = c.right - S(16) - S(130);
    c.habitsX = c.dnaX + c.dnaW + S(20);
    c.habitsW = c.flagX - S(20) - c.habitsX;
    if (c.habitsW < S(150)) c.habitsW = 0;
    return c;
}

static void drawRosterHeader(HDC hdc, int y, const RosterCols& c) {
    SetBkMode(hdc, TRANSPARENT);
    HFONT of = (HFONT)SelectObject(hdc, g_hFontLabel);
    SetTextColor(hdc, GRAY_400);
    int h = S(ROSTER_HEADER_HEIGHT);
    RECT n = {c.nameX, y, c.nameX + c.nameW, y + h};
    DrawTextW(hdc, L"STUDENT", -1, &n, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT d = {c.dnaX, y, c.dnaX + c.dnaW, y + h};
    DrawTextW(hdc, L"STYLE DNA", -1, &d, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    if (c.habitsW > 0) {
        RECT hb = {c.habitsX, y, c.habitsX + c.habitsW, y + h};
        DrawTextW(hdc, L"HABITS", -1, &hb, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    RECT f = {c.flagX, y, c.right - S(16), y + h};
    DrawTextW(hdc, L"FLAGGED", -1, &f, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    RECT line = {c.left, y + h - 1, c.right, y + h};
    HBRUSH lb = CreateSolidBrush(GRAY_700);
    FillRect(hdc, &line, lb);
    DeleteObject(lb);
    SelectObject(hdc, of);
}

static void drawRosterRow(HDC hdc, int y, const RosterCols& c,
                          const StudentProfileSummary& profile,
                          const StudentFlagSummary& flags, bool hovered)
{
    SetBkMode(hdc, TRANSPARENT);
    int rowH = S(ROSTER_ROW_HEIGHT);
    if (hovered) {
        RECT bg = {c.left, y, c.right, y + rowH - 1};
        HBRUSH hb = CreateSolidBrush(GRAY_850);
        FillRect(hdc, &bg, hb);
        DeleteObject(hb);
        RECT bar = {c.left, y, c.left + S(3), y + rowH - 1};
        HBRUSH gb = CreateSolidBrush(GOLD_500);
        FillRect(hdc, &bar, gb);
        DeleteObject(gb);
    }

    // Name + submission count
    HFONT of = (HFONT)SelectObject(hdc, g_hFontButton);
    SetTextColor(hdc, WHITE_);
    RECT nameR = {c.nameX, y + S(14), c.nameX + c.nameW, y + S(38)};
    DrawTextW(hdc, s2w(profile.name).c_str(), -1, &nameR,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(hdc, g_hFontMonoSm);
    SetTextColor(hdc, GRAY_400);
    std::wstring cnt = std::to_wstring(profile.submissionCount) +
                       (profile.submissionCount == 1 ? L" submission" : L" submissions");
    RECT cntR = {c.nameX, y + S(40), c.nameX + c.nameW, y + S(60)};
    DrawTextW(hdc, cnt.c_str(), -1, &cntR, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Style DNA strip
    DnaStripStyle ds = dnaStyleRoster();
    drawDnaStrip(hdc, c.dnaX, y + (rowH - ds.height) / 2, profile.featureVector, nullptr, ds);

    // Habits — the two condensed note lines
    if (c.habitsW > 0) {
        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_200);
        auto lines = condenseStyleNotes(profile.styleNotes, 2);
        int ly = y + S(14);
        for (const auto& line : lines) {
            RECT lr = {c.habitsX, ly, c.habitsX + c.habitsW, ly + S(24)};
            DrawTextW(hdc, line.c_str(), -1, &lr,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            ly += S(24);
        }
    }

    // Flagged appearances — count in the severity color, max below
    RECT fR = {c.flagX, y + S(12), c.right - S(16), y + S(40)};
    RECT mR = {c.flagX, y + S(40), c.right - S(16), y + S(60)};
    if (flags.count > 0) {
        COLORREF sev = (flags.maxScore >= 85.0) ? RISK_HIGH
                     : (flags.maxScore >= 70.0) ? RISK_MOD : RISK_LOW;
        SelectObject(hdc, g_hFontH2);
        SetTextColor(hdc, sev);
        std::wstring n = std::to_wstring(flags.count);
        DrawTextW(hdc, n.c_str(), -1, &fR, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, g_hFontMonoSm);
        SetTextColor(hdc, GRAY_400);
        wchar_t maxBuf[32];
        swprintf(maxBuf, 32, L"max %.1f%%", flags.maxScore);
        DrawTextW(hdc, maxBuf, -1, &mR, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    } else {
        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_500);
        DrawTextW(hdc, L"None", -1, &fR, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

    // Separator
    RECT line = {c.left, y + rowH - 1, c.right, y + rowH};
    HBRUSH lb = CreateSolidBrush(GRAY_700);
    FillRect(hdc, &line, lb);
    DeleteObject(lb);

    SelectObject(hdc, of);
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
    const StudentsTabCache& cache = studentsTabCache();

    std::set<std::string> uniqueBlocks(cache.blocks.begin(), cache.blocks.end());
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

    // Sort ▾ dropdown, right-aligned in this fixed bar (it used to be a
    // click-to-cycle button in the scrolling header below).
    int sortW = dropdownAnchorWidth(hdc, DD_CLASSES_SORT, L"Sort: ");
    int sortH = btbH - S(12);
    RECT sortR = {x + width - S(20) - sortW, y + (btbH - sortH) / 2,
                  x + width - S(20), y + (btbH - sortH) / 2 + sortH};
    // Search box, left of Sort — shrinks on narrow windows first.
    int searchW = (sortR.left - S(10)) - (x + S(20)) - S(260);  // keep room for chips
    if (searchW > S(280)) searchW = S(280);
    if (searchW < S(170)) searchW = S(170);
    RECT searchR = {sortR.left - S(10) - searchW, sortR.top, sortR.left - S(10), sortR.bottom};
    int chipsRight = searchR.left - S(16);

    int tabX = x + S(20);
    int tabPad = S(10);
    int tabH = btbH - S(10);
    int tabY = y + S(5);

    // Build the full chip list: "All" first (count = every student),
    // then each block name with its own student count.
    std::map<std::string,int> blockCounts;
    for (const auto& b : cache.blocks) blockCounts[b]++;

    std::vector<std::pair<int,std::wstring>> chipItems; // {tabIdx, label}
    chipItems.push_back({-1, L"All"});
    for (size_t i = 0; i < g_blockTabNames.size(); ++i)
        chipItems.push_back({(int)i, s2w(g_blockTabNames[i])});

    // Measure every chip first, then show as many as fit beside the
    // search box; the rest go behind a final "+N more ▾" chip that opens
    // a dropdown of the hidden sections (so on a small window nothing is
    // unreachable). If the selected section is one of the hidden ones,
    // that chip shows its name instead, in the active style.
    struct Chip { int tabIdx; std::wstring label; int w; };
    std::vector<Chip> chips;
    for (const auto& item : chipItems) {
        int count = (item.first == -1)
            ? (int)r.profiles.size()
            : blockCounts[g_blockTabNames[item.first]];
        std::wstring fullLabel = item.second + L"  (" + std::to_wstring(count) + L")";
        RECT measureR = {0, 0, 0, 0};
        DrawTextW(hdc, fullLabel.c_str(), -1, &measureR, DT_CALCRECT | DT_SINGLELINE);
        chips.push_back({item.first, fullLabel, measureR.right - measureR.left + S(32)});
    }

    int avail = chipsRight - tabX;
    auto widthOf = [&](int k) {           // first k chips, with gaps after each
        int w = 0;
        for (int i = 0; i < k; ++i) w += chips[i].w + tabPad;
        return w;
    };
    auto overflowLabel = [&](int visible) {
        for (int i = visible; i < (int)chips.size(); ++i)
            if (chips[i].tabIdx == g_currentBlockTab) return chips[i].label;
        return L"+" + std::to_wstring((int)chips.size() - visible) + L" more";
    };
    auto overflowWidth = [&](const std::wstring& lbl) {
        RECT m = {0, 0, 0, 0};
        DrawTextW(hdc, (lbl + L"  \u25BE").c_str(), -1, &m, DT_CALCRECT | DT_SINGLELINE);
        return m.right - m.left + S(32);
    };

    int visible = (int)chips.size();
    if (widthOf(visible) - tabPad > avail) {
        for (visible = (int)chips.size() - 1; visible > 0; --visible)
            if (widthOf(visible) + overflowWidth(overflowLabel(visible)) <= avail) break;
    }

    for (int i = 0; i < visible; ++i) {
        const Chip& ch = chips[i];
        bool active = (g_currentBlockTab == ch.tabIdx);
        RECT pillR = {tabX, tabY, tabX + ch.w, tabY + tabH};

        // Pill chip: gray-850 rest, maroon-700 active fill, per spec
        // §5.8 ("pill, gray-850 rest, maroon-700 fill when active").
        drawCard(hdc, pillR, active ? MAROON_700 : GRAY_850,
                 active ? GOLD_500 : GRAY_700, tabH / 2);
        SetTextColor(hdc, active ? WHITE_ : GRAY_400);
        DrawTextW(hdc, ch.label.c_str(), -1, &pillR,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        BlockTabRect hit;
        hit.x = tabX; hit.y = tabY; hit.w = ch.w; hit.h = tabH;
        hit.tabIdx = ch.tabIdx;
        g_blockTabRects.push_back(hit);
        addFocusable(g_contentFocusables, FocusKind::BlockChip,
                     RECT{hit.x, hit.y, hit.x + hit.w, hit.y + hit.h}, hit.tabIdx, false);

        tabX += ch.w + tabPad;
    }

    g_sectionOverflowLabels.clear();
    g_sectionOverflowTabs.clear();
    if (visible < (int)chips.size()) {
        for (int i = visible; i < (int)chips.size(); ++i) {
            g_sectionOverflowLabels.push_back(chips[i].label);
            g_sectionOverflowTabs.push_back(chips[i].tabIdx);
        }
        std::wstring lbl = overflowLabel(visible);
        bool activeHidden = (lbl.empty() || lbl[0] != L'+');
        bool open = dropdownIsOpen(DD_CLASSES_SECTIONS);
        int ovW = overflowWidth(lbl);
        if (ovW > chipsRight - tabX) ovW = chipsRight - tabX;
        RECT pillR = {tabX, tabY, tabX + ovW, tabY + tabH};
        drawCard(hdc, pillR, activeHidden ? MAROON_700 : GRAY_850,
                 (activeHidden || open) ? GOLD_500 : GRAY_700, tabH / 2);
        SetTextColor(hdc, activeHidden ? WHITE_ : GRAY_200);
        std::wstring text = lbl + L"  \u25BE";
        RECT tr = {pillR.left + S(14), pillR.top, pillR.right - S(14), pillR.bottom};
        DrawTextW(hdc, text.c_str(), -1, &tr,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        registerDropdownAnchor(DD_CLASSES_SECTIONS, pillR);
    }

    drawSearchBox(hdc, searchR);
    drawDropdownAnchor(hdc, sortR, DD_CLASSES_SORT, L"Sort: ");

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

    // Title, with the click hint on its own line beneath it (it used
    // to be right-aligned on the title's line and overlapped the title
    // on narrow windows).
    RECT hdrRect = {x + S(28), y + S(12), x + width - S(28), y + S(52)};
    std::wstring fullHdr = L"Student Style Profiles  ("
                         + std::to_wstring(r.profiles.size()) + L")";
    DrawTextW(hdc, fullHdr.c_str(), -1, &hdrRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_400);
    RECT hintRect = {x + S(28), y + S(52), x + width - S(28), y + S(74)};
    DrawTextW(hdc, L"Click a student to view their full profile",
              -1, &hintRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    y += S(88);

    // Group students by block. Students are only grouped for DISPLAY
    // purposes here — this has no effect on scoring or pairwise
    // comparison, which still spans all blocks (a same-code submission
    // across two different sections is still real evidence worth
    // flagging, not noise to isolate away).
    const StudentsTabCache& cache = studentsTabCache();

    // Flagged-appearance count per student, needed for the "Flagged"
    // sort mode — from the per-run cache.
    auto flagCount = [&](int i) { return cache.flags[i].count; };

    std::map<std::string, std::vector<int>> byBlock; // block name -> profile indices
    // Search box (block bar): case-insensitive substring of the name.
    auto passesSearch = [&](const std::string& name) {
        if (g_searchFilterLower.empty()) return true;
        std::wstring nameLower = s2w(name);
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::towlower);
        return nameLower.find(g_searchFilterLower) != std::wstring::npos;
    };
    for (size_t i = 0; i < r.profiles.size(); ++i)
        if (passesSearch(r.profiles[i].name))
            byBlock[cache.blocks[i]].push_back((int)i);

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
                    return flagCount(a) > flagCount(b);
                });
                break;
        }
    }

    RosterCols cols = rosterColumns(x, width);

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
        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_400);
        RECT emR = {x + S(30), y, x + width - S(30), y + S(40)};
        DrawTextW(hdc, L"No students match your search.", -1, &emR,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        y += S(50);
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

        drawRosterHeader(hdc, y, cols);
        y += S(ROSTER_HEADER_HEIGHT);

        int rowH = S(ROSTER_ROW_HEIGHT);
        for (int idx : indices) {
            const auto& profile = r.profiles[idx];
            // Viewport culling (scale perf fix C) — rows are fixed-height,
            // so layout, hit rect and focusable are identical whether or
            // not the row is painted this frame.
            bool visible = (y + rowH >= -200) && (y <= g_viewportH + 200);
            if (visible)
                drawRosterRow(hdc, y, cols, profile, cache.flags[idx],
                              idx == g_rosterHoverIdx);

            StudentCardRect hit;
            hit.x = cols.left;
            hit.y = y + g_scrollY;  // content-space, like every other hit rect
            hit.w = cols.right - cols.left;
            hit.h = rowH;
            hit.studentIdx = idx;
            g_studentCardRects.push_back(hit);
            addFocusable(g_contentFocusables, FocusKind::StudentCard,
                         RECT{hit.x, hit.y, hit.x + hit.w, hit.y + hit.h},
                         idx, true);
            y += rowH;
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
    // Click rect straight from the actually-drawn backRect, with the
    // same +g_scrollY convention as every other content hit-rect (it
    // used to be set separately from a fixed, unscaled reference point
    // in drawStudentsPage and drifted once the page was scrolled).
    g_backButtonRect = RECT{backRect.left, backRect.top + g_scrollY,
                            backRect.right, backRect.bottom + g_scrollY};
    addFocusable(g_contentFocusables, FocusKind::BackButton,
                 g_backButtonRect, 0, true);

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
    // Exact student-identity matching, same as studentsTabCache() —
    // the old case-insensitive substring match put "LuisDizon"'s files
    // and pairs on "Luis"'s page, disagreeing with the card's count.
    std::vector<std::string> files;
    std::set<std::string> seenFiles;
    for (const auto& pair : g_analysisResults.allPairs) {
        if (studentNameForFile(pair.filenameA) == profile.name)
            if (seenFiles.insert(pair.filenameA).second) files.push_back(pair.filenameA);
        if (studentNameForFile(pair.filenameB) == profile.name)
            if (seenFiles.insert(pair.filenameB).second) files.push_back(pair.filenameB);
    }

    // Load the Classroom sync manifest (if any) so we can show the
    // instructor-facing assignment title ("Midterm Exam") instead of
    // the internally-encoded filename used for parsing. Files that
    // weren't synced via Classroom (manually imported) simply won't
    // have an entry and fall back to showing the raw filename.
    const std::map<std::string,SyncManifestEntry>& syncManifest =
        studentsTabCache().manifest;

    std::vector<std::pair<int,const PairAnalysisDisplay*>> related; // {flaggedPairs index, ptr}
    for (int fpi = 0; fpi < (int)g_analysisResults.flaggedPairs.size(); ++fpi) {
        const auto& fp = g_analysisResults.flaggedPairs[fpi];
        if (fp.studentA.studentName == profile.name || fp.studentB.studentName == profile.name)
            related.push_back({fpi, &fp});
    }

    // ── 3-COLUMN CARD LAYOUT ─────────────────────────────────
    // On narrow content widths (small window, or a wide sidebar) the
    // third column left too little room for a row's pill + name +
    // score, so Flagged Appearances drops below the first two columns
    // at full width instead.
    int gap     = S(14);
    int colBase = x + S(28);
    int totalW  = width - S(56);
    bool stacked = totalW < S(900);
    int c1W     = stacked ? (totalW - gap) * 60 / 100 : (int)(totalW * 0.38);
    int c2W     = stacked ? totalW - gap - c1W        : (int)(totalW * 0.26);
    int c3W     = stacked ? totalW : totalW - c1W - c2W - gap * 2;
    int c1X     = colBase;
    int c2X     = colBase + c1W + gap;
    int c3X     = stacked ? colBase : colBase + c1W + c2W + gap * 2;

    // Calculate max column height
    int notesH  = (int)profile.styleNotes.size() * S(26) + S(50);
    if (notesH < S(100)) notesH = S(100);
    int filesH  = (int)files.size() * S(24) + S(50);
    if (filesH < S(80)) filesH = S(80);
    // Flagged Appearances shows at most FA_MAX_VISIBLE_ROWS rows; the
    // rest scroll inside the panel (see g_faScrollY).
    int faRowH    = S(34);
    int faVisible = (int)related.size() < FA_MAX_VISIBLE_ROWS
                  ? (int)related.size() : FA_MAX_VISIBLE_ROWS;
    int faListH   = faVisible * faRowH;
    int flagsH  = faListH + S(50) +
                  ((int)related.size() > FA_MAX_VISIBLE_ROWS ? S(20) : 0);
    if (flagsH < S(80)) flagsH = S(80);
    if (g_faStudent != studentIdx) { g_faStudent = studentIdx; g_faScrollY = 0; }
    g_faScrollMax = (int)related.size() * faRowH - faListH;
    if (g_faScrollMax < 0) g_faScrollMax = 0;
    if (g_faScrollY > g_faScrollMax) g_faScrollY = g_faScrollMax;
    int row1H   = notesH > filesH ? notesH : filesH;
    int colH    = stacked ? row1H : (row1H > flagsH ? row1H : flagsH);
    int c3Y     = stacked ? y + colH + gap : y;
    int c3H     = stacked ? flagsH : colH;

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
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
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
    RECT c3Card = {c3X, c3Y, c3X + c3W, c3Y + c3H};
    drawCard(hdc, c3Card, GRAY_850, GRAY_700, 10);

    drawEyebrow(hdc, c3X + S(14), c3Y + S(12), c3W - S(28), L"FLAGGED APPEARANCES");
    HPEN c3Sep = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldC3 = (HPEN)SelectObject(hdc, c3Sep);
    MoveToEx(hdc, c3X + S(14), c3Y + S(33), nullptr);
    LineTo(hdc, c3X + c3W - S(14), c3Y + S(33));
    SelectObject(hdc, oldC3); DeleteObject(c3Sep);

    int ry = c3Y + S(40);
    bool faScrolls = g_faScrollMax > 0;
    g_faListRect  = RECT{c3X + S(12), ry, c3X + c3W - S(12), ry + faListH};
    g_faTrackRect = RECT{};
    g_faThumbRect = RECT{};
    int faSaved = 0;
    if (!related.empty()) {
        // Clip rows to the panel's own viewport (on top of whatever
        // clip the page already has), and shift them by its scroll.
        faSaved = SaveDC(hdc);
        IntersectClipRect(hdc, g_faListRect.left, g_faListRect.top,
                          g_faListRect.right, g_faListRect.bottom);
        ry -= g_faScrollY;
    }
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

            int rowH = faRowH;
            if (ry + rowH <= g_faListRect.top || ry >= g_faListRect.bottom) {
                ry += rowH;  // outside the panel viewport: no draw, no hit rect
                continue;
            }
            RECT rowR = {c3X + S(12), ry,
                         c3X + c3W - S(12) - (faScrolls ? S(12) : 0), ry + rowH - S(2)};

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
            RECT scR = {rowR.right - S(60), rowR.top, rowR.right - S(2), rowR.bottom};
            DrawTextW(hdc, fmtPct(pair->combinedScore).c_str(), -1, &scR,
                      DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

            // Who this student was paired with, and on which exam — the
            // two raw file names used to sit here, middle-truncated into
            // unreadable fragments at every width (the pair page still
            // shows the full file names one click away).
            const std::string& other = (pair->studentA.studentName == profile.name)
                ? pair->studentB.studentName : pair->studentA.studentName;
            std::wstring vs = L"vs " + s2w(other) + L"  ·  " +
                periodDisplayLabelGC(extractPeriodGC(pair->filenameA));
            SelectObject(hdc, g_hFontBodyNew);
            SetTextColor(hdc, GRAY_200);
            RECT vsR = {pillR.right + S(10), rowR.top, scR.left - S(8), rowR.bottom};
            DrawTextW(hdc, vs.c_str(), -1, &vsR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

            // Hit rect clipped to the panel viewport (a half-scrolled
            // row is only clickable where it's visible).
            RECT vis;
            IntersectRect(&vis, &rowR, &g_faListRect);
            FlaggedAppearanceRowRect hit;
            hit.r = { vis.left, vis.top + g_scrollY, vis.right, vis.bottom + g_scrollY };
            hit.flaggedPairIndex = flaggedIdx;
            g_flaggedAppearanceRowRects.push_back(hit);
            addFocusable(g_contentFocusables, FocusKind::FlaggedAppearanceRow,
                         hit.r, hit.flaggedPairIndex, true);

            ry += rowH;
        }
    }
    if (faSaved) RestoreDC(hdc, faSaved);

    // The panel's own scrollbar — same thin-thumb look as the source
    // view's, on the panel's right edge.
    if (faScrolls) {
        int total = (int)related.size() * faRowH;
        g_faTrackRect = RECT{c3X + c3W - S(18), g_faListRect.top,
                             c3X + c3W - S(10), g_faListRect.bottom};
        HBRUSH tb = CreateSolidBrush(GRAY_800);
        RECT trackFill = {g_faTrackRect.left + S(2), g_faTrackRect.top,
                          g_faTrackRect.right - S(2), g_faTrackRect.bottom};
        FillRect(hdc, &trackFill, tb);
        DeleteObject(tb);
        int thumbH = faListH * faListH / total;
        if (thumbH < S(28)) thumbH = S(28);
        int thumbY = g_faListRect.top +
                     (int)((long long)(faListH - thumbH) * g_faScrollY / g_faScrollMax);
        g_faThumbRect = RECT{g_faTrackRect.left, thumbY, g_faTrackRect.right, thumbY + thumbH};
        RECT thumbFill = {g_faThumbRect.left + S(2), g_faThumbRect.top,
                          g_faThumbRect.right - S(2), g_faThumbRect.bottom};
        HBRUSH thb = CreateSolidBrush(g_faDragging ? GOLD_500 : GRAY_500);
        FillRect(hdc, &thumbFill, thb);
        DeleteObject(thb);

        // "N of M" under the list so it's obvious there's more.
        SelectObject(hdc, g_hFontSmall);
        SetTextColor(hdc, GRAY_500);
        std::wstring more = std::to_wstring(related.size()) + L" flagged appearances · scroll for more";
        RECT mr = {c3X + S(14), g_faListRect.bottom + S(4), c3X + c3W - S(14), g_faListRect.bottom + S(24)};
        DrawTextW(hdc, more.c_str(), -1, &mr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    y = (stacked ? c3Y + c3H : y + colH) + S(30);

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
        if (searchBoxHit(mx, my)) focusSearchBox(hwnd);
        return true;  // the bar's own band: never fall through to a row under it
    }

    // Flagged Appearances panel scrollbar: drag the thumb, or click
    // the track above/below it to page.
    if (g_selectedStudent >= 0 && g_faTrackRect.right > g_faTrackRect.left &&
        mx >= g_faTrackRect.left && mx < g_faTrackRect.right &&
        my >= g_faTrackRect.top && my < g_faTrackRect.bottom)
    {
        if (my >= g_faThumbRect.top && my < g_faThumbRect.bottom) {
            g_faDragging = true;
            g_faDragStartY = my;
            g_faDragStartScroll = g_faScrollY;
            SetCapture(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
        } else {
            int page = g_faListRect.bottom - g_faListRect.top;
            faSetScroll(hwnd, g_faScrollY + (my < g_faThumbRect.top ? -page : page));
        }
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
    (void)pageContentStartY;
    if (g_selectedStudent >= 0) {
        // Back/report click rects are set inside drawStudentDetailPage
        // from the rects it actually draws (scroll-correct).
        return drawStudentDetailPage(hdc, contentX, contentY, contentW,
                                      g_selectedStudent);
    }
    return drawStudentProfiles(hdc, contentX, contentY, contentW);
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
// Roster row hover (grid view). my is window-space; hit rects are
// content-space (+g_scrollY).
void handleStudentsGridMouseMove(HWND hwnd, int mx, int my) {
    if (g_faDragging) {
        int trackH = g_faListRect.bottom - g_faListRect.top;
        int thumbH = g_faThumbRect.bottom - g_faThumbRect.top;
        int travel = trackH - thumbH;
        if (travel > 0)
            faSetScroll(hwnd, g_faDragStartScroll +
                (int)((long long)(my - g_faDragStartY) * g_faScrollMax / travel));
        return;
    }
    int found = -1;
    if (g_selectedStudent < 0) {
        int sy = my + g_scrollY;
        for (const auto& card : g_studentCardRects) {
            if (mx >= card.x && mx < card.x + card.w &&
                sy >= card.y && sy < card.y + card.h) { found = card.studentIdx; break; }
        }
    }
    if (found != g_rosterHoverIdx) {
        g_rosterHoverIdx = found;
        TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&tme);
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void clearStudentsGridHover(HWND hwnd) {
    if (g_rosterHoverIdx != -1) {
        g_rosterHoverIdx = -1;
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

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
