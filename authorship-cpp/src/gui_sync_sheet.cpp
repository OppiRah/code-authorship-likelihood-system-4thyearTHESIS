// ─────────────────────────────────────────────────────────────
// gui_sync_sheet.cpp
// Google Classroom sync sheet — the CONNECT → CHOOSE → SYNCING →
// DONE modal, its three background worker threads, its own focus
// system, and SyncSheetProc.
//
// Split out of gui.cpp unchanged. Everything here was `static` in
// gui.cpp and stays `static` except the four globals and three
// functions the rest of the app already reaches through
// gui_common.h (g_hSyncSheet, g_hSyncTree, g_syncSheetFocusables,
// g_syncFocusedIndex / syncFocusMove, activateSyncFocused,
// onGoogleClassroom).
//
// NOTE: activateSyncFocused's comment refers to activateFocused()
// "above" — that function now lives in gui.cpp. Comment left
// verbatim rather than reworded, per the no-incidental-edits rule.
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"

#ifdef _WIN32

// ═════════════════════════════════════════════════════════════
// SYNC SHEET (spec §5.4) — replaces the six-popup MessageBox chain
// ═════════════════════════════════════════════════════════════
// One in-app modal (640px, gray-900, gold-500 title, scrim behind)
// that changes STATE rather than closing and reopening. All the
// existing sync business logic (Prelim filtering, per-block manifest
// tagging, drop-zone folder creation, audit logging) is preserved
// unchanged — only the presentation layer moves.
//
// States: CONNECT → CHOOSE → SYNCING → DONE
// Network calls run on a background thread (mirroring the existing
// analysisThread pattern) so "Run in background" is genuine, not
// simulated — the sheet can minimize to a status-bar strip while the
// UI thread stays responsive.

enum SyncSheetState { SHEET_CONNECT, SHEET_CHOOSE, SHEET_SYNCING, SHEET_DONE, SHEET_CLOSED };

// onRunAnalysis is defined later in the file; SyncSheetProc's
// SHEET_DONE state calls it directly ("Run analysis" primary button)
// so the instructor never has to close the sheet and hunt for the
// sidebar button separately.

struct SyncAssignmentRow {
    GAssignment assignment;
    std::string period;   // "Prelim" / "Midterm" / etc, derived from title
    bool checked;
};

struct SyncBlockNode {
    GCourse course;
    std::string label;    // "Programming 1 (C Language) · BSIT Block 1"
    std::vector<SyncAssignmentRow> rows;
    HTREEITEM treeItem;
};

static SyncSheetState g_syncState = SHEET_CLOSED;
HWND  g_hSyncSheet   = nullptr;
HWND  g_hSyncTree    = nullptr;
static GCCredentials g_syncCreds;
static std::vector<SyncBlockNode> g_syncBlocks;

// ── Sync sheet's own focus system (item 2) ────────────────────────
// Deliberately separate from g_sidebarFocusables/g_contentFocusables
// rather than merged into one combined index: g_syncState (just
// above) isn't visible yet at the point those two are declared much
// earlier in the file, and there's no actual need to unify them
// anyway — the main window is disabled the whole time the sheet is
// open, so the two systems never need to interleave. Handled as its
// own small parallel system: its own vector, its own index, its own
// branch in the main message loop (checked via msg.hwnd == g_hSyncSheet).
std::vector<FocusableItem> g_syncSheetFocusables;
int g_syncFocusedIndex = -1;

void syncFocusMove(int delta) {
    if (g_syncSheetFocusables.empty()) { g_syncFocusedIndex = -1; return; }
    int n = (int)g_syncSheetFocusables.size();
    int next = g_syncFocusedIndex + delta;
    if (next < 0) next = 0;
    if (next >= n) next = n - 1;
    g_syncFocusedIndex = next;
    g_focusVisible = true;
}

static bool  g_syncExcludePrelim = true;
static bool  g_syncConnecting    = false;
static bool  g_syncMinimized     = false;

// Syncing-state progress
static int          g_syncTotal = 0, g_syncDone = 0;
static std::wstring g_syncCurrentLabel;
static std::vector<std::wstring> g_syncLogLines;
static HWND  g_hSyncLogList = nullptr;

// Done-state results (mirrors the accumulators the old function used)
static int totalFilesDownloadedG = 0, totalAssignmentsSucceededG = 0, totalAssignmentsPlannedG = 0;
static std::vector<std::string> assignmentsNoSubsG, assignmentsFailedG, blocksSyncedG;

static void syncSheetSetState(SyncSheetState s);
static void syncSheetLayout();
static std::wstring periodFromTitle(const std::string& title);

// Derives a period label purely for the tree's display text — the
// real categorization (which drives scoring) still happens in
// google_classroom.cpp / main.cpp exactly as before. This is
// presentation-only, matching the spec's row format:
//   ☐ Midterm Exam            Midterm
static std::wstring periodFromTitle(const std::string& title) {
    std::string lower = title;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("prelim") != std::string::npos)    return L"Prelim";
    if (lower.find("semifinal") != std::string::npos ||
        lower.find("semi-final") != std::string::npos) return L"Semifinal";
    if (lower.find("midterm") != std::string::npos)   return L"Midterm";
    if (lower.find("final") != std::string::npos)     return L"Final";
    return L"";
}

// ── Background worker: authenticate ───────────────────────────
static void syncAuthThread(HWND sheet) {
    gc_loadSettings(g_syncCreds);
    bool ok = gc_hasValidToken();
    if (!ok) ok = gc_authenticate(sheet, g_syncCreds, true);
    PostMessage(sheet, WM_SYNC_AUTHDONE, ok ? 1 : 0, 0);
}

// ── Background worker: fetch courses + assignments ────────────
static void syncFetchThread(HWND sheet) {
    auto courses = gc_getCourses(g_syncCreds);
    auto* blocks = new std::vector<SyncBlockNode>();

    for (const auto& c : courses) {
        SyncBlockNode node;
        node.course = c;
        node.label = c.name;
        if (!c.section.empty()) node.label += " \u00B7 " + c.section;
        node.treeItem = nullptr;

        auto assignments = gc_getAssignments(g_syncCreds, c.id);
        for (const auto& a : assignments) {
            SyncAssignmentRow row;
            row.assignment = a;
            row.period = wideToUtf8(periodFromTitle(a.title));
            row.checked = (row.period != "Prelim"); // exclude-prelims default ON
            node.rows.push_back(row);
        }
        blocks->push_back(node);
    }

    PostMessage(sheet, WM_SYNC_COURSESDONE, 0, (LPARAM)blocks);
}

// ── Background worker: run the actual sync ────────────────────
// This is the EXACT business logic from the old onGoogleClassroom(),
// unchanged — Prelim exclusion, per-block manifest tagging via
// blockLabel, drop-zone subfolder creation, audit logging. Only the
// presentation moved: MessageBoxW confirms became the checkbox tree
// the user already committed to in SHEET_CHOOSE.
static void syncRunThread(HWND sheet, std::vector<SyncBlockNode>* blocks,
                            std::wstring destFolder) {
    std::string outputFolder = toNarrowGC(destFolder);
    CreateDirectoryA(outputFolder.c_str(), nullptr);

    int totalFilesDownloaded = 0, totalAssignmentsSucceeded = 0, totalAssignmentsPlanned = 0;
    std::vector<std::string> assignmentsWithNoSubmissions, assignmentsFailed, blocksSynced;

    // Count planned items for the progress bar
    int totalPlanned = 0;
    for (auto& b : *blocks)
        for (auto& r : b.rows)
            if (r.checked) ++totalPlanned;

    g_syncTotal = totalPlanned;
    g_syncDone  = 0;

    for (auto& block : *blocks) {
        std::vector<SyncAssignmentRow*> checkedRows;
        for (auto& r : block.rows) if (r.checked) checkedRows.push_back(&r);
        if (checkedRows.empty()) continue;

        std::string blockLabel = block.course.name;
        if (!block.course.section.empty()) blockLabel += " - " + block.course.section;

        blocksSynced.push_back(blockLabel);
        totalAssignmentsPlanned += (int)checkedRows.size();

        // Drop-zone subfolder for manual imports — unchanged from before
        std::string sanitizedBlockName;
        for (char c : blockLabel) {
            if (isalnum((unsigned char)c) || c == '_' || c == '-') sanitizedBlockName += c;
            else if (c == ' ') sanitizedBlockName += '_';
        }
        if (!sanitizedBlockName.empty())
            CreateDirectoryA((outputFolder + "\\" + sanitizedBlockName).c_str(), nullptr);

        for (auto* row : checkedRows) {
            g_syncCurrentLabel = L"[" + s2w(blockLabel) + L"] " + s2w(row->assignment.title);
            PostMessage(sheet, WM_SYNC_PROGRESS, 0, 0);

            GCResult r = gc_downloadSubmissions(sheet, g_syncCreds,
                block.course.id, row->assignment.id, row->assignment.title,
                blockLabel, outputFolder);

            audit_log("GOOGLE CLASSROOM SYNC",
                "Block: " + blockLabel + "\n" +
                "Assignment: " + row->assignment.title + "\n" +
                "Files downloaded: " + std::to_string(r.filesDownloaded) + "\n" +
                "Result: " + std::string(r.success ? "Success" : "No files") +
                (r.error.empty() ? "" : ("\nNote: " + r.error)));

            if (r.success && r.filesDownloaded > 0) {
                totalFilesDownloaded += r.filesDownloaded;
                ++totalAssignmentsSucceeded;
                g_syncLogLines.push_back(L"\u2713  " + s2w(row->assignment.title) +
                    L"  (" + std::to_wstring(r.filesDownloaded) + L" files)");
            } else if (r.filesDownloaded == 0) {
                assignmentsWithNoSubmissions.push_back(blockLabel + " \u2014 " + row->assignment.title);
                g_syncLogLines.push_back(L"\u2014  " + s2w(row->assignment.title) + L"  (no submissions)");
            } else {
                assignmentsFailed.push_back(blockLabel + " \u2014 " + row->assignment.title);
                g_syncLogLines.push_back(L"\u2717  " + s2w(row->assignment.title) + L"  (failed)");
            }

            ++g_syncDone;
            PostMessage(sheet, WM_SYNC_PROGRESS, 0, 0);
        }
    }

    int cFileCount = countCFiles(destFolder);
    g_state.dataFolder = destFolder;
    g_state.fileCount  = cFileCount;
    g_state.hasData    = (cFileCount > 0);

    totalFilesDownloadedG      = totalFilesDownloaded;
    totalAssignmentsSucceededG = totalAssignmentsSucceeded;
    totalAssignmentsPlannedG   = totalAssignmentsPlanned;
    assignmentsNoSubsG         = assignmentsWithNoSubmissions;
    assignmentsFailedG         = assignmentsFailed;
    blocksSyncedG              = blocksSynced;

    delete blocks;
    PostMessage(sheet, WM_SYNC_DONE, 0, 0);
}

// ── Tree population (SHEET_CHOOSE) ─────────────────────────────
static void populateSyncTree() {
    if (!g_hSyncTree) return;
    TreeView_DeleteAllItems(g_hSyncTree);

    for (auto& block : g_syncBlocks) {
        int checkedCount = 0;
        for (auto& r : block.rows) if (r.checked) ++checkedCount;

        std::wstring parentLabel = s2w(block.label) + L"    " +
            std::to_wstring(checkedCount) + L" of " +
            std::to_wstring(block.rows.size()) + L" selected";

        TVINSERTSTRUCTW tvi = {};
        tvi.hParent = TVI_ROOT;
        tvi.hInsertAfter = TVI_LAST;
        tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
        tvi.item.pszText = const_cast<wchar_t*>(parentLabel.c_str());
        tvi.item.lParam = -1; // -1 marks a block header, not an assignment row
        HTREEITEM parent = TreeView_InsertItem(g_hSyncTree, &tvi);
        block.treeItem = parent;
        TreeView_SetCheckState(g_hSyncTree, parent, checkedCount == (int)block.rows.size());

        for (size_t i = 0; i < block.rows.size(); ++i) {
            const auto& row = block.rows[i];
            std::wstring rowLabel = s2w(row.assignment.title);
            if (!row.period.empty()) rowLabel += L"    " + s2w(row.period);

            TVINSERTSTRUCTW rvi = {};
            rvi.hParent = parent;
            rvi.hInsertAfter = TVI_LAST;
            rvi.item.mask = TVIF_TEXT | TVIF_PARAM;
            rvi.item.pszText = const_cast<wchar_t*>(rowLabel.c_str());
            // Encode (blockIdx, rowIdx) into lParam
            rvi.item.lParam = (LPARAM)((&block - &g_syncBlocks[0]) * 10000 + i);
            HTREEITEM child = TreeView_InsertItem(g_hSyncTree, &rvi);
            TreeView_SetCheckState(g_hSyncTree, child, row.checked);
        }
        TreeView_Expand(g_hSyncTree, parent, TVE_EXPAND);
    }
}

// Recomputes the footer button label — "Sync N assignments from M
// classes" — live, so the user never answers the same question twice.
static std::wstring syncFooterLabel() {
    int totalAssign = 0, totalBlocks = 0;
    for (auto& b : g_syncBlocks) {
        int c = 0;
        for (auto& r : b.rows) if (r.checked) ++c;
        if (c > 0) { totalAssign += c; ++totalBlocks; }
    }
    if (totalAssign == 0) return L"Select at least one assignment";
    return L"Sync " + std::to_wstring(totalAssign) + L" assignment" +
           (totalAssign == 1 ? L"" : L"s") + L" from " +
           std::to_wstring(totalBlocks) + L" class" + (totalBlocks == 1 ? L"" : L"es");
}

static RECT g_syncPrimaryBtnRect = {};
static RECT g_syncSecondaryBtnRect = {};
static RECT g_syncSelectAllRect = {}, g_syncSelectNoneRect = {}, g_syncExcludeChipRect = {};
static RECT g_syncMinimizeRect = {};
static bool g_syncHoverPrimary = false;

static LRESULT CALLBACK SyncSheetProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void registerSyncSheetClass(HINSTANCE hInst) {
    static bool done = false;
    if (done) return;
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = SyncSheetProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"CALSSSyncSheet";
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);
    done = true;
}

static const int SHEET_W_DESIGN = 640;

static int sheetHeightForState() {
    switch (g_syncState) {
        case SHEET_CONNECT: return 320;
        case SHEET_CHOOSE:  return 560;
        case SHEET_SYNCING: return 420;
        case SHEET_DONE:    return 420;
        default: return 320;
    }
}

static void syncSheetLayout() {
    if (!g_hSyncSheet) return;
    int h = S(sheetHeightForState());
    int sheetW = S(SHEET_W_DESIGN);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(g_hSyncSheet, HWND_TOP,
                 (screenW - sheetW) / 2, (screenH - h) / 2, sheetW, h,
                 SWP_NOACTIVATE);

    if (g_hSyncTree) {
        bool show = (g_syncState == SHEET_CHOOSE);
        ShowWindow(g_hSyncTree, show ? SW_SHOW : SW_HIDE);
        if (show) MoveWindow(g_hSyncTree, S(20), S(130), sheetW - S(40), S(300), TRUE);
    }
    if (g_hSyncLogList) {
        bool show = (g_syncState == SHEET_SYNCING);
        ShowWindow(g_hSyncLogList, show ? SW_SHOW : SW_HIDE);
        if (show) MoveWindow(g_hSyncLogList, S(20), S(180), sheetW - S(40), S(170), TRUE);
    }
    InvalidateRect(g_hSyncSheet, nullptr, TRUE);
}

static void syncSheetSetState(SyncSheetState s) {
    g_syncState = s;
    syncSheetLayout();
}

// Starts the whole flow. Called from onGoogleClassroom() — the sheet
// owns everything from here; the caller just opens it.
static void openSyncSheet(HWND parent) {
    HINSTANCE hInst = GetModuleHandle(nullptr);
    registerSyncSheetClass(hInst);

    g_syncBlocks.clear();
    g_syncLogLines.clear();
    g_syncConnecting = false;
    g_syncMinimized  = false;
    g_syncExcludePrelim = true;
    g_syncFocusedIndex = -1; // fresh keyboard-focus state per open (item 2)

    g_hSyncSheet = CreateWindowExW(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
        L"CALSSSyncSheet", L"Sync Google Classroom",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, S(SHEET_W_DESIGN), S(320), parent, nullptr, hInst, nullptr);

    // Tree control for SHEET_CHOOSE — real SysTreeView32 with
    // checkboxes, since a hand-rolled collapsible checkbox tree in
    // raw GDI would be high-risk for comparatively little benefit
    // over the native control.
    g_hSyncTree = CreateWindowExW(0, WC_TREEVIEWW, L"",
        WS_CHILD | WS_BORDER | WS_VISIBLE | WS_VSCROLL |
        TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_CHECKBOXES,
        S(20), S(130), S(SHEET_W_DESIGN) - S(40), S(300),
        g_hSyncSheet, nullptr, hInst, nullptr);
    SendMessage(g_hSyncTree, WM_SETFONT, (WPARAM)g_hFontBodyNew, TRUE);

    // Native tree-view controls default to a white background — jarring
    // against the dark theme everywhere else in the sheet. Restyle it
    // to match: dark surface, light text, muted connector lines.
    TreeView_SetBkColor(g_hSyncTree, GRAY_850);
    TreeView_SetTextColor(g_hSyncTree, GRAY_200);
    TreeView_SetLineColor(g_hSyncTree, GRAY_700);
    ShowWindow(g_hSyncTree, SW_HIDE);

    g_hSyncLogList = CreateWindowW(L"LISTBOX", L"",
        WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOSEL | LBS_NOINTEGRALHEIGHT,
        S(20), S(180), S(SHEET_W_DESIGN) - S(40), S(170),
        g_hSyncSheet, nullptr, hInst, nullptr);
    SendMessage(g_hSyncLogList, WM_SETFONT, (WPARAM)g_hFontMonoSm, TRUE);
    ShowWindow(g_hSyncLogList, SW_HIDE);

    EnableWindow(g_hMainWindow, FALSE);
    syncSheetSetState(SHEET_CONNECT);
    ShowWindow(g_hSyncSheet, SW_SHOW);
    // Needed for the Esc-closes-sheet fix (WM_KEYDOWN) to actually
    // reach this window rather than the disabled parent.
    SetFocus(g_hSyncSheet);
}

static void closeSyncSheet() {
    g_syncState = SHEET_CLOSED;
    if (g_hSyncSheet) { DestroyWindow(g_hSyncSheet); g_hSyncSheet = nullptr; }
    g_hSyncTree = nullptr;
    g_hSyncLogList = nullptr;
    g_syncFocusedIndex = -1; // item 2
    EnableWindow(g_hMainWindow, TRUE);
    SetForegroundWindow(g_hMainWindow);
    updateUI();
}

// ── Sync sheet shared handlers (item 2) ───────────────────────────
// Same reasoning as every other screen in this pass: each function
// here is called by both the mouse click handler and
// activateFocused(), so the two input paths can't drift apart. Pulled
// out of what was previously only reachable from WM_LBUTTONDOWN.

static void syncConnectStart(HWND hwnd) {
    if (g_syncConnecting) return;
    g_syncConnecting = true;
    InvalidateRect(hwnd, nullptr, FALSE);
    std::thread t(syncAuthThread, hwnd);
    t.detach();
}

static void syncSelectAllRows(HWND hwnd) {
    for (auto& b : g_syncBlocks)
        for (auto& r : b.rows)
            if (!g_syncExcludePrelim || r.period != "Prelim") r.checked = true;
    populateSyncTree();
    InvalidateRect(hwnd, nullptr, FALSE);
}

static void syncSelectNoneRows(HWND hwnd) {
    for (auto& b : g_syncBlocks)
        for (auto& r : b.rows) r.checked = false;
    populateSyncTree();
    InvalidateRect(hwnd, nullptr, FALSE);
}

static void syncToggleExcludePrelim(HWND hwnd) {
    g_syncExcludePrelim = !g_syncExcludePrelim;
    if (g_syncExcludePrelim)
        for (auto& b : g_syncBlocks)
            for (auto& r : b.rows)
                if (r.period == "Prelim") r.checked = false;
    populateSyncTree();
    InvalidateRect(hwnd, nullptr, FALSE);
}

// The CHOOSE state's primary button: starts the actual sync run.
// No-op if nothing is checked, exactly like the mouse click did.
static void syncChooseConfirm(HWND hwnd) {
    int totalChecked = 0;
    for (auto& b : g_syncBlocks)
        for (auto& r : b.rows) if (r.checked) ++totalChecked;
    if (totalChecked == 0) return;

    if (g_state.dataFolder.empty()) {
        char docsPath[MAX_PATH] = {};
        if (SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr,
                              SHGFP_TYPE_CURRENT, docsPath) == S_OK)
            g_state.dataFolder = s2w(std::string(docsPath) + "\\CALSS_Classroom_Sync");
        else
            g_state.dataFolder = L"CALSS_Classroom_Sync";
    }

    g_syncLogLines.clear();
    syncSheetSetState(SHEET_SYNCING);

    auto* blocksCopy = new std::vector<SyncBlockNode>(g_syncBlocks);
    std::thread t(syncRunThread, hwnd, blocksCopy, g_state.dataFolder);
    t.detach();
}

static void syncMinimizeSheet(HWND hwnd) {
    g_syncMinimized = true;
    ShowWindow(hwnd, SW_HIDE);
    EnableWindow(g_hMainWindow, TRUE);
}

static void syncDoneRunAnalysis() {
    closeSyncSheet();
    onRunAnalysis(g_hMainWindow);
}

static LRESULT CALLBACK SyncSheetProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // Dark-theme the log listbox — LISTBOX controls need explicit
        // WM_CTLCOLORLISTBOX handling (unlike TreeView, which has its
        // own dedicated TVM_SETBKCOLOR/SETTEXTCOLOR messages).
        case WM_CTLCOLORLISTBOX: {
            HDC hdcCtl = (HDC)wParam;
            SetBkColor(hdcCtl, GRAY_850);
            SetTextColor(hdcCtl, GRAY_200);
            static HBRUSH listBg = nullptr;
            if (listBg) DeleteObject(listBg);
            listBg = CreateSolidBrush(GRAY_850);
            return (LRESULT)listBg;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdcScreen = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);

            HDC hdc = CreateCompatibleDC(hdcScreen);
            HBITMAP bmp = CreateCompatibleBitmap(hdcScreen, rc.right, rc.bottom);
            HBITMAP oldBmp = (HBITMAP)SelectObject(hdc, bmp);

            HBRUSH bg = CreateSolidBrush(GRAY_900);
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            SetBkMode(hdc, TRANSPARENT);
            HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontH2);

            // Keyboard focus (item 2): rebuilt fresh each paint, same
            // pattern as g_sidebarFocusables/g_contentFocusables.
            g_syncSheetFocusables.clear();

            // Title, gold, per spec
            SetTextColor(hdc, GOLD_500);
            RECT titleR = {S(20), S(16), rc.right - S(20), S(44)};
            const wchar_t* titleText =
                g_syncState == SHEET_CONNECT ? L"Connect Google Classroom" :
                g_syncState == SHEET_CHOOSE  ? L"Choose what to sync" :
                g_syncState == SHEET_SYNCING ? L"Syncing\u2026" : L"Sync complete";
            DrawTextW(hdc, titleText, -1, &titleR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, g_hFontBodyNew);

            // ── STATE A: Connect ──────────────────────────────
            if (g_syncState == SHEET_CONNECT) {
                SetTextColor(hdc, GRAY_200);
                RECT body = {S(20), S(60), rc.right - S(20), S(180)};
                DrawTextW(hdc,
                    L"1.  Sign in with the Google account used for Classroom\n"
                    L"2.  Grant CALSS permission to read your courses and rosters\n"
                    L"3.  Return here \u2014 the sheet advances automatically",
                    -1, &body, DT_LEFT | DT_WORDBREAK);

                RECT btn = {S(20), rc.bottom - S(60), S(260), rc.bottom - S(20)};
                g_syncPrimaryBtnRect = btn;
                bool disabled = g_syncConnecting;
                if (!disabled) {
                    addFocusable(g_syncSheetFocusables, FocusKind::SyncPrimaryBtn, btn, 0, false);
                }
                COLORREF bgc = disabled ? GRAY_800 : (g_syncHoverPrimary ? MAROON_600 : MAROON_700);
                HBRUSH pb = CreateSolidBrush(bgc);
                HPEN pp = CreatePen(PS_SOLID, 1, bgc);
                HBRUSH ob = (HBRUSH)SelectObject(hdc, pb);
                HPEN op = (HPEN)SelectObject(hdc, pp);
                RoundRect(hdc, btn.left, btn.top, btn.right, btn.bottom, S(6), S(6));
                SelectObject(hdc, ob); SelectObject(hdc, op);
                DeleteObject(pb); DeleteObject(pp);
                SetTextColor(hdc, disabled ? GRAY_500 : WHITE_);
                DrawTextW(hdc, g_syncConnecting ? L"Waiting for browser\u2026" : L"Connect Google account",
                          -1, &btn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                // Cancel — visible in both sub-states now (previously
                // only appeared once g_syncConnecting was true, leaving
                // the initial screen with no visible way out at all).
                // Same drawCard() secondary-button treatment used
                // everywhere else in the app (matches AppModal's own
                // secondary button exactly), replacing what used to be
                // bare, unstyled text here.
                RECT cancelR = {S(280), rc.bottom - S(60), S(420), rc.bottom - S(20)};
                g_syncSecondaryBtnRect = cancelR;
                addFocusable(g_syncSheetFocusables, FocusKind::SyncSecondaryBtn, cancelR, 0, false);
                drawCard(hdc, cancelR, GRAY_800, GRAY_700, 8);
                SetTextColor(hdc, GRAY_200);
                DrawTextW(hdc, L"Cancel", -1, &cancelR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            // ── STATE B: Choose (tree is a real child control) ─
            else if (g_syncState == SHEET_CHOOSE) {
                // Header controls — same ghost-chip treatment as
                // "Exclude prelims" so all three read as buttons,
                // not plain clickable text with no visible affordance.
                SelectObject(hdc, g_hFontBodyNew);

                SIZE saSz; GetTextExtentPoint32W(hdc, L"Select all", 10, &saSz);
                RECT saR = {S(20), S(54), S(20) + saSz.cx + S(28), S(78)};
                g_syncSelectAllRect = saR;
                addFocusable(g_syncSheetFocusables, FocusKind::SyncSelectAll, saR, 0, false);
                drawCard(hdc, saR, GRAY_800, GRAY_700, 12);
                SetTextColor(hdc, GRAY_200);
                DrawTextW(hdc, L"Select all", -1, &saR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SIZE snSz; GetTextExtentPoint32W(hdc, L"Select none", 11, &snSz);
                RECT snR = {saR.right + S(8), S(54), saR.right + S(8) + snSz.cx + S(28), S(78)};
                g_syncSelectNoneRect = snR;
                addFocusable(g_syncSheetFocusables, FocusKind::SyncSelectNone, snR, 0, false);
                drawCard(hdc, snR, GRAY_800, GRAY_700, 12);
                SetTextColor(hdc, GRAY_200);
                DrawTextW(hdc, L"Select none", -1, &snR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                // Exclude-prelims toggle chip
                RECT chip = {rc.right - S(200), S(54), rc.right - S(20), S(78)};
                g_syncExcludeChipRect = chip;
                addFocusable(g_syncSheetFocusables, FocusKind::SyncExcludePrelim, chip, 0, false);
                COLORREF chipBg = g_syncExcludePrelim ? MAROON_700 : GRAY_800;
                drawCard(hdc, chip, chipBg, g_syncExcludePrelim ? GOLD_500 : GRAY_700, 12);
                SetTextColor(hdc, g_syncExcludePrelim ? WHITE_ : GRAY_400);
                DrawTextW(hdc, L"Exclude prelims", -1, &chip, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                // The tree itself — a real native SysTreeView32 control,
                // not a virtual hit-rect like everything else here. This
                // stop hands real Win32 focus to it (see the main
                // message loop) so the tree's own built-in keyboard
                // handling (arrow keys navigate, Space toggles a check)
                // takes over — not reimplemented here. Known limitation:
                // once real focus is on the tree, further Tab presses
                // aren't intercepted by this system (plain SysTreeView32
                // has no built-in "Tab moves to next control" behavior
                // without a dialog manager), so a keyboard user has to
                // click elsewhere to return to this Tab order. Flagged,
                // not fixed, per this pass's scope.
                if (g_hSyncTree) {
                    // childRectInParent reads the REAL window rect,
                    // which syncSheetLayout() already positions using
                    // S()-scaled MoveWindow args — nothing further
                    // needed here for this one.
                    RECT treeR = childRectInParent(g_hSyncTree, hwnd);
                    addFocusable(g_syncSheetFocusables, FocusKind::SyncTree, treeR, 0, false);
                }

                // Destination path (below the tree — tree occupies 130..430)
                SetTextColor(hdc, GRAY_400);
                RECT destLbl = {S(20), S(438), S(120), S(458)};
                DrawTextW(hdc, L"Save to:", -1, &destLbl, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                SelectObject(hdc, g_hFontMonoSm);
                SetTextColor(hdc, GRAY_200);
                RECT destPath = {S(120), S(438), rc.right - S(20), S(458)};
                std::wstring shown = truncateMiddle(hdc, g_state.dataFolder.empty()
                    ? L"Documents\\CALSS_Classroom_Sync" : g_state.dataFolder,
                    (rc.right - S(20)) - S(120));
                DrawTextW(hdc, shown.c_str(), -1, &destPath, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // Footer — live-recomputed label
                SelectObject(hdc, g_hFontButton);
                RECT btn = {S(20), rc.bottom - S(60), rc.right - S(140), rc.bottom - S(20)};
                g_syncPrimaryBtnRect = btn;
                std::wstring label = syncFooterLabel();
                bool enabled = (label != L"Select at least one assignment");
                if (enabled) {
                    addFocusable(g_syncSheetFocusables, FocusKind::SyncPrimaryBtn, btn, 0, false);
                }
                COLORREF bgc = !enabled ? GRAY_800 : (g_syncHoverPrimary ? MAROON_600 : MAROON_700);
                HBRUSH pb = CreateSolidBrush(bgc);
                HPEN pp = CreatePen(PS_SOLID, 1, bgc);
                HBRUSH ob = (HBRUSH)SelectObject(hdc, pb);
                HPEN op = (HPEN)SelectObject(hdc, pp);
                RoundRect(hdc, btn.left, btn.top, btn.right, btn.bottom, S(6), S(6));
                SelectObject(hdc, ob); SelectObject(hdc, op);
                DeleteObject(pb); DeleteObject(pp);
                SetTextColor(hdc, !enabled ? GRAY_500 : WHITE_);
                DrawTextW(hdc, label.c_str(), -1, &btn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                RECT cancelR = {rc.right - S(120), rc.bottom - S(60), rc.right - S(20), rc.bottom - S(20)};
                g_syncSecondaryBtnRect = cancelR;
                addFocusable(g_syncSheetFocusables, FocusKind::SyncSecondaryBtn, cancelR, 0, false);
                drawCard(hdc, cancelR, GRAY_800, GRAY_700, 8);
                SetTextColor(hdc, GRAY_200);
                DrawTextW(hdc, L"Cancel", -1, &cancelR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            // ── STATE C: Syncing (log list is a real child control) ─
            else if (g_syncState == SHEET_SYNCING) {
                SetTextColor(hdc, GRAY_200);
                RECT prog = {S(20), S(56), rc.right - S(20), S(76)};
                std::wstring progTxt = std::to_wstring(g_syncDone) + L" / " +
                                       std::to_wstring(g_syncTotal);
                SelectObject(hdc, g_hFontMono);
                DrawTextW(hdc, progTxt.c_str(), -1, &prog, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // Determinate bar
                int barY = S(82), barH = S(6);
                RECT track = {S(20), barY, rc.right - S(20), barY + barH};
                HBRUSH tb = CreateSolidBrush(GRAY_800);
                FillRect(hdc, &track, tb); DeleteObject(tb);
                int pct = g_syncTotal > 0 ? (g_syncDone * 100 / g_syncTotal) : 0;
                int fillW = ((rc.right - S(40)) * pct) / 100;
                if (fillW > 0) {
                    RECT fill = {S(20), barY, S(20) + fillW, barY + barH};
                    HBRUSH fb = CreateSolidBrush(GOLD_500);
                    FillRect(hdc, &fill, fb); DeleteObject(fb);
                }

                SelectObject(hdc, g_hFontBodyNew);
                SetTextColor(hdc, GRAY_400);
                RECT cur = {S(20), S(100), rc.right - S(20), S(122)};
                std::wstring curTxt = truncateMiddle(hdc, g_syncCurrentLabel, rc.right - S(40));
                DrawTextW(hdc, curTxt.c_str(), -1, &cur, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // "Run in background" — minimizes the sheet to a slim
                // strip; the worker thread keeps running regardless,
                // since sync is genuinely threaded (mirrors
                // analysisThread), not just visually hidden.
                RECT bgBtn = {S(20), rc.bottom - S(44), S(200), rc.bottom - S(16)};
                g_syncMinimizeRect = bgBtn;
                addFocusable(g_syncSheetFocusables, FocusKind::SyncMinimize, bgBtn, 0, false);
                SetTextColor(hdc, GRAY_400);
                SelectObject(hdc, g_hFontBodyNew);
                DrawTextW(hdc, L"Run in background", -1, &bgBtn,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

            // ── STATE D: Done ──────────────────────────────────
            else if (g_syncState == SHEET_DONE) {
                SelectObject(hdc, g_hFontBodyNew);
                SetTextColor(hdc, GRAY_200);

                std::wstring summary =
                    L"Blocks synced: " + std::to_wstring(blocksSyncedG.size()) + L"\n" +
                    L"Assignments: " + std::to_wstring(totalAssignmentsSucceededG) +
                        L" / " + std::to_wstring(totalAssignmentsPlannedG) + L"\n" +
                    L"Files downloaded: " + std::to_wstring(totalFilesDownloadedG) + L"\n" +
                    L"Files now in sync folder: " + std::to_wstring(g_state.fileCount);
                RECT sumR = {S(20), S(60), rc.right - S(20), S(160)};
                DrawTextW(hdc, summary.c_str(), -1, &sumR, DT_LEFT | DT_WORDBREAK);

                if (!assignmentsFailedG.empty()) {
                    SetTextColor(hdc, RISK_HIGH);
                    RECT failR = {S(20), S(168), rc.right - S(20), S(190)};
                    std::wstring t = std::to_wstring(assignmentsFailedG.size()) + L" assignment(s) failed to sync";
                    DrawTextW(hdc, t.c_str(), -1, &failR, DT_LEFT | DT_SINGLELINE);
                }

                SetTextColor(hdc, GRAY_500);
                RECT tip = {S(20), S(220), rc.right - S(20), S(270)};
                DrawTextW(hdc,
                    L"A subfolder was created for each synced block \u2014 drop extra "
                    L".c files into the matching folder and they'll be picked up "
                    L"on the next analysis.",
                    -1, &tip, DT_LEFT | DT_WORDBREAK);

                SelectObject(hdc, g_hFontButton);
                RECT runBtn = {S(20), rc.bottom - S(60), S(220), rc.bottom - S(20)};
                g_syncPrimaryBtnRect = runBtn;
                addFocusable(g_syncSheetFocusables, FocusKind::SyncPrimaryBtn, runBtn, 0, false);
                COLORREF bgc = g_syncHoverPrimary ? MAROON_600 : MAROON_700;
                HBRUSH pb = CreateSolidBrush(bgc);
                HPEN pp = CreatePen(PS_SOLID, 1, bgc);
                HBRUSH ob = (HBRUSH)SelectObject(hdc, pb);
                HPEN op = (HPEN)SelectObject(hdc, pp);
                RoundRect(hdc, runBtn.left, runBtn.top, runBtn.right, runBtn.bottom, S(6), S(6));
                SelectObject(hdc, ob); SelectObject(hdc, op);
                DeleteObject(pb); DeleteObject(pp);
                SetTextColor(hdc, WHITE_);
                DrawTextW(hdc, L"Run analysis", -1, &runBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                RECT closeBtn = {S(240), rc.bottom - S(60), S(340), rc.bottom - S(20)};
                g_syncSecondaryBtnRect = closeBtn;
                addFocusable(g_syncSheetFocusables, FocusKind::SyncSecondaryBtn, closeBtn, 0, false);
                drawCard(hdc, closeBtn, GRAY_800, GRAY_700, 8);
                SetTextColor(hdc, GRAY_200);
                SelectObject(hdc, g_hFontBodyNew);
                DrawTextW(hdc, L"Close", -1, &closeBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            SelectObject(hdc, oldFont);
            BitBlt(hdcScreen, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);
            SelectObject(hdc, oldBmp);
            DeleteObject(bmp);
            DeleteDC(hdc);

            // Keyboard focus ring (item 2) — same "draw on hdcScreen
            // after the blit" pattern as SidebarProc/ContentProc.
            if (g_focusVisible && g_syncFocusedIndex >= 0 &&
                g_syncFocusedIndex < (int)g_syncSheetFocusables.size())
            {
                drawFocusRing(hdcScreen, g_syncSheetFocusables[g_syncFocusedIndex].rect);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND: return 1;

        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            bool hov = PtInRect(&g_syncPrimaryBtnRect, pt);
            if (hov != g_syncHoverPrimary) {
                g_syncHoverPrimary = hov;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            if (g_syncState == SHEET_CONNECT) {
                if (PtInRect(&g_syncPrimaryBtnRect, pt) && !g_syncConnecting) {
                    syncConnectStart(hwnd);
                } else if (PtInRect(&g_syncSecondaryBtnRect, pt)) {
                    closeSyncSheet();
                }
            }
            else if (g_syncState == SHEET_CHOOSE) {
                if (PtInRect(&g_syncSelectAllRect, pt)) {
                    syncSelectAllRows(hwnd);
                } else if (PtInRect(&g_syncSelectNoneRect, pt)) {
                    syncSelectNoneRows(hwnd);
                } else if (PtInRect(&g_syncExcludeChipRect, pt)) {
                    syncToggleExcludePrelim(hwnd);
                } else if (PtInRect(&g_syncSecondaryBtnRect, pt)) {
                    closeSyncSheet();
                } else if (PtInRect(&g_syncPrimaryBtnRect, pt)) {
                    syncChooseConfirm(hwnd);
                }
            }
            else if (g_syncState == SHEET_SYNCING) {
                if (PtInRect(&g_syncMinimizeRect, pt)) {
                    syncMinimizeSheet(hwnd);
                }
            }
            else if (g_syncState == SHEET_DONE) {
                if (PtInRect(&g_syncPrimaryBtnRect, pt)) {
                    syncDoneRunAnalysis();
                } else if (PtInRect(&g_syncSecondaryBtnRect, pt)) {
                    closeSyncSheet();
                }
            }
            return 0;
        }

        case WM_SYNC_AUTHDONE: {
            g_syncConnecting = false;
            if (wParam == 0) {
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            std::thread t(syncFetchThread, hwnd);
            t.detach();
            return 0;
        }

        case WM_SYNC_COURSESDONE: {
            auto* blocks = (std::vector<SyncBlockNode>*)lParam;
            g_syncBlocks = *blocks;
            delete blocks;
            populateSyncTree();
            syncSheetSetState(SHEET_CHOOSE);
            return 0;
        }

        case WM_SYNC_PROGRESS: {
            if (g_hSyncLogList && !g_syncLogLines.empty()) {
                SendMessageW(g_hSyncLogList, LB_RESETCONTENT, 0, 0);
                for (auto& line : g_syncLogLines)
                    SendMessageW(g_hSyncLogList, LB_ADDSTRING, 0, (LPARAM)line.c_str());
                SendMessageW(g_hSyncLogList, LB_SETTOPINDEX,
                             (WPARAM)g_syncLogLines.size() - 1, 0);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_SYNC_DONE: {
            if (g_syncMinimized) {
                g_syncMinimized = false;
                ShowWindow(hwnd, SW_SHOW);
                EnableWindow(g_hMainWindow, FALSE);
            }
            syncSheetSetState(SHEET_DONE);
            return 0;
        }

        case WM_NOTIFY: {
            NMHDR* nm = (NMHDR*)lParam;
            if (nm->hwndFrom == g_hSyncTree && nm->code == TVN_ITEMCHANGEDW) {
                NMTVITEMCHANGE* ic = (NMTVITEMCHANGE*)lParam;
                LPARAM lp = 0;
                TVITEMW item = {};
                item.mask = TVIF_PARAM;
                item.hItem = ic->hItem;
                TreeView_GetItem(g_hSyncTree, &item);
                lp = item.lParam;
                bool checked = TreeView_GetCheckState(g_hSyncTree, ic->hItem) != 0;

                if (lp == -1) {
                    // Block header toggled — cascade to all children
                    HTREEITEM child = TreeView_GetChild(g_hSyncTree, ic->hItem);
                    while (child) {
                        TreeView_SetCheckState(g_hSyncTree, child, checked);
                        TVITEMW ci = {};
                        ci.mask = TVIF_PARAM;
                        ci.hItem = child;
                        TreeView_GetItem(g_hSyncTree, &ci);
                        int blockIdx = (int)(ci.lParam / 10000);
                        int rowIdx   = (int)(ci.lParam % 10000);
                        if (blockIdx >= 0 && blockIdx < (int)g_syncBlocks.size() &&
                            rowIdx  >= 0 && rowIdx  < (int)g_syncBlocks[blockIdx].rows.size())
                            g_syncBlocks[blockIdx].rows[rowIdx].checked = checked;
                        child = TreeView_GetNextSibling(g_hSyncTree, child);
                    }
                } else {
                    int blockIdx = (int)(lp / 10000);
                    int rowIdx   = (int)(lp % 10000);
                    if (blockIdx >= 0 && blockIdx < (int)g_syncBlocks.size() &&
                        rowIdx  >= 0 && rowIdx  < (int)g_syncBlocks[blockIdx].rows.size())
                        g_syncBlocks[blockIdx].rows[rowIdx].checked = checked;
                }
                InvalidateRect(hwnd, nullptr, FALSE); // refresh footer label
            }
            return 0;
        }

        case WM_KEYDOWN:
            // Compliance fix item 2: Esc must close sheets. Reuses
            // WM_CLOSE's existing guard (never close mid-sync) rather
            // than duplicating that logic.
            if (wParam == VK_ESCAPE) {
                if (g_syncState != SHEET_SYNCING) closeSyncSheet();
                return 0;
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);

        case WM_CLOSE:
            if (g_syncState != SHEET_SYNCING) closeSyncSheet();
            return 0;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void onGoogleClassroom(HWND hwnd) {
    openSyncSheet(hwnd);
}

// Activates whatever is currently focused in the sync sheet's own
// separate focus list (Enter key, from the main message loop's
// sync-sheet branch). Mirrors activateFocused() above but operates on
// g_syncSheetFocusables/g_syncFocusedIndex instead — see the comment
// at their declaration for why these are kept separate. Each case
// calls the same shared handler its mouse-click equivalent already
// calls (syncConnectStart(), etc.), so the two paths can't drift
// apart, same principle as everywhere else in this pass.
void activateSyncFocused() {
    if (g_syncFocusedIndex < 0 || g_syncFocusedIndex >= (int)g_syncSheetFocusables.size())
        return;
    const FocusableItem& item = g_syncSheetFocusables[g_syncFocusedIndex];
    if (!g_hSyncSheet) return;

    switch (item.kind) {
        case FocusKind::SyncPrimaryBtn:
            // SyncPrimaryBtn is reused across states — dispatch on
            // g_syncState at activation time, exactly like the mouse
            // handler does with the same PtInRect(&g_syncPrimaryBtnRect...)
            // check across all four states.
            if (g_syncState == SHEET_CONNECT) syncConnectStart(g_hSyncSheet);
            else if (g_syncState == SHEET_CHOOSE) syncChooseConfirm(g_hSyncSheet);
            else if (g_syncState == SHEET_DONE) syncDoneRunAnalysis();
            break;

        case FocusKind::SyncSecondaryBtn:
            // Secondary is "Cancel"/"Close" in every state it appears
            // in — always closes the sheet.
            closeSyncSheet();
            break;

        case FocusKind::SyncSelectAll:
            syncSelectAllRows(g_hSyncSheet);
            break;

        case FocusKind::SyncSelectNone:
            syncSelectNoneRows(g_hSyncSheet);
            break;

        case FocusKind::SyncExcludePrelim:
            syncToggleExcludePrelim(g_hSyncSheet);
            break;

        case FocusKind::SyncMinimize:
            syncMinimizeSheet(g_hSyncSheet);
            break;

        case FocusKind::SyncTree:
            // Enter on this stop does nothing extra — real focus was
            // already handed to the tree the moment Tab arrived here
            // (see the message loop). Its own native keyboard handling
            // takes it from here.
            break;

        default:
            break;
    }
}

#endif // _WIN32