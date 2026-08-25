// ─────────────────────────────────────────────────────────────
// gui.cpp
// Win32 GUI for Code Authorship Likelihood Scoring System
// Stage 4: Wired analysis pipeline + UL color palette
// ─────────────────────────────────────────────────────────────
#include "../include/gui_common.h"

#ifdef _WIN32


static ULONG_PTR g_gdiplusToken = 0;
static void initGdiPlus() {
    GdiplusStartupInput input;
    GdiplusStartup(&g_gdiplusToken, &input, nullptr);
}
static void shutdownGdiPlus() {
    if (g_gdiplusToken) GdiplusShutdown(g_gdiplusToken);
}

// UL/CCS seal image (ulccslogo.png, shipped next to the .exe), loaded
// once and shared by every in-app draw site that shows the seal (sidebar
// header). Returns nullptr if the file isn't found, so callers can fall
// back to the old drawn gold circle rather than showing nothing.
static Image* getULLogo() {
    static Image* s_logoImg = nullptr;
    static bool s_logoTried = false;
    if (!s_logoTried) {
        s_logoTried = true;
        s_logoImg = new Image(L"ulccslogo.png");
        if (s_logoImg->GetLastStatus() != Ok) {
            delete s_logoImg;
            s_logoImg = nullptr;
        }
    }
    return s_logoImg;
}


// ── Window class and title ────────────────────────────────────
// Kept here rather than in gui_common.h: runGUI() below is the only
// user, and as `const wchar_t*` pointers they warn as unused in every
// TU that doesn't reference them.
static const wchar_t* WINDOW_CLASS_NAME = L"AuthorshipScorerMainWindow";
static const wchar_t* WINDOW_TITLE      = L"Code Authorship Likelihood System";


// ── Animation / hover state (must be before SidebarProc) ─────
static int  g_progressAnim  = 0;
static bool g_animRunning   = false;
static HWND g_hoveredBtn    = nullptr;

// Content fade-in on tab switch
int  g_contentFadeAlpha = 255;  // 0-255, 255 = fully opaque

// ── Menu and control IDs ──────────────────────────────────────


AppState g_state = {L"", 0, false, false, false, L"", L""};

// ── Window handles ────────────────────────────────────────────
HWND g_hMainWindow = nullptr;
static HWND g_hSidebar    = nullptr;
HWND g_hContent    = nullptr;
static HWND g_hStatus     = nullptr;
static HWND g_hBtnSelectFolder = nullptr;
static HWND g_hBtnImportFiles  = nullptr;
static HWND g_hBtnGClassroom   = nullptr;
static HWND g_hBtnRunAnalysis  = nullptr;
static HWND g_hChkAiSummary    = nullptr;

static std::vector<AppToast> g_toasts;

static std::vector<ToastRect> g_toastRects;

void showToast(const std::wstring& text, ToastKind kind) {
    AppToast t;
    t.text = text;
    t.kind = kind;
    t.showUntilTick = GetTickCount() + TOAST_DURATION_MS;
    g_toasts.push_back(t);
    if (g_toasts.size() > TOAST_MAX_STACK) g_toasts.erase(g_toasts.begin());
    if (g_hContent) {
        SetTimer(g_hContent, TIMER_TOAST, 200, nullptr);
        InvalidateRect(g_hContent, nullptr, FALSE);
    }
}

AppModal g_appModal;
static RECT g_modalPrimaryBtnRect = {};
static RECT g_modalSecondaryBtnRect = {};
// Keyboard focus (item 2): which of the modal's own buttons is
// currently focused. Reset to the primary button whenever a modal
// opens; Tab toggles it (see the main message loop); Enter activates
// whichever one is currently focused, not always primary.
static bool g_modalFocusOnPrimary = true;

void showAppModal(const std::wstring& title, const std::wstring& body,
                          ModalKind kind, COLORREF accent,
                          const std::wstring& primaryLabel,
                          const std::wstring& secondaryLabel,
                          std::function<void(bool)> onResult)
{
    g_appModal.kind = kind;
    g_appModal.title = title;
    g_appModal.body = body;
    g_appModal.accent = accent;
    g_appModal.primaryLabel = primaryLabel;
    g_appModal.secondaryLabel = secondaryLabel;
    g_appModal.onResult = std::move(onResult);
    g_modalFocusOnPrimary = true;
    if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
}

// Resolves the modal, fires its callback (if any), and clears it.
static void dismissAppModal(bool primaryPressed) {
    if (g_appModal.kind == MODAL_NONE) return;
    auto cb = g_appModal.onResult;
    g_appModal = AppModal{};
    if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
    if (cb) cb(primaryPressed);
}


// ── Unified keyboard focus system (compliance sweep item 2) ──────
// Treats real sidebar HWND controls and virtual canvas hit-rects as
// the same kind of thing: an ordered list of rects with an activation
// function, rebuilt fresh each paint (same pattern as every other
// hit-rect vector in this file — g_studentCardRects, g_blockTabRects,
// etc.). Tab/Shift+Tab is intercepted centrally in the main message
// loop rather than relying on WS_TABSTOP/IsDialogMessage, so one
// mental model covers the whole app instead of native dialog
// navigation for some elements and custom hit-testing for the rest —
// see the Item 2 planning discussion for why (IsDialogMessage already

// Two separate vectors, not one shared list: SidebarProc and
// ContentProc are separate windows that repaint independently and
// asynchronously (on their own hover/state changes), so if either one
// cleared+rebuilt a SHARED vector on its own WM_PAINT, it would wipe
// out whatever the other window had just contributed. Each owns and
// rebuilds only its own region; g_focusedIndex is a combined index
// (sidebar first, matching its physical left position) resolved via
// focusableAt() below.
static std::vector<FocusableItem> g_sidebarFocusables;  // rebuilt in SidebarProc's WM_PAINT
std::vector<FocusableItem> g_contentFocusables;  // rebuilt in ContentProc's WM_PAINT
static std::vector<FocusableItem> g_titleBarFocusables; // rebuilt in TitleBarProc's WM_PAINT —
                                                          // declared here (not near the rest of
                                                          // the title bar's own state further
                                                          // down) so totalFocusableCount()/
                                                          // focusableAt() below can see it; a
                                                          // third real top-level-ish window,
                                                          // folded into the SAME combined index
                                                          // as sidebar+content rather than a
                                                          // separate system like the sync sheet's
                                                          // — the title bar never receives real
                                                          // Win32 focus (nothing calls
                                                          // SetFocus(g_hTitleBar) anywhere), so a
                                                          // branch gated on msg.hwnd would never
                                                          // fire; Tab events instead arrive while
                                                          // g_hContent holds real focus, exactly
                                                          // like the sidebar buttons already rely
                                                          // on, so it has to share that mechanism.
                                                          // Ordered LAST in the combined index
                                                          // (after sidebar, after content) purely
                                                          // to minimize changes to the existing
                                                          // sidebar/content range checks below.
static int  g_focusedIndex = -1;
bool g_focusVisible = false; // only true after keyboard nav;
                                     // any mouse click clears it
                                     // (focus-visible semantics)

static int totalFocusableCount() {
    return (int)(g_sidebarFocusables.size() + g_contentFocusables.size() +
                 g_titleBarFocusables.size());
}

// Resolves a combined index into whichever region owns it. Returns
// nullptr if out of range (list shrank since g_focusedIndex was set —
// callers must check).
static FocusableItem* focusableAt(int idx) {
    if (idx < 0) return nullptr;
    if (idx < (int)g_sidebarFocusables.size()) return &g_sidebarFocusables[idx];
    int ci = idx - (int)g_sidebarFocusables.size();
    if (ci < (int)g_contentFocusables.size()) return &g_contentFocusables[ci];
    int ti = ci - (int)g_contentFocusables.size();
    if (ti >= 0 && ti < (int)g_titleBarFocusables.size()) return &g_titleBarFocusables[ti];
    return nullptr;
}

// Called once per paint, at the point each screen's rect vectors are
// already fully populated, to append that screen's items in visual
// order into whichever region the caller owns.
void addFocusable(std::vector<FocusableItem>& region, FocusKind kind,
                          const RECT& r, int dataIndex, bool inContent) {
    FocusableItem item;
    item.kind = kind;
    item.rect = r;
    item.dataIndex = dataIndex;
    item.inContent = inContent;
    region.push_back(item);
}

// Clamped (not wrapped) advance — consistent with the arrow-key tab
// switching added earlier in Item 2's contained slice.
static void focusMove(int delta) {
    int n = totalFocusableCount();
    if (n == 0) { g_focusedIndex = -1; return; }
    int next = g_focusedIndex + delta;
    if (next < 0) next = 0;
    if (next >= n) next = n - 1;
    g_focusedIndex = next;
    g_focusVisible = true;
}

// activateFocused's body (a switch over FocusKind) is filled in
// screen by screen below, since each case calls a handler defined
// later in the file; forward-declared here so focusMove()'s callers
// in WM_KEYDOWN can reference it regardless of definition order.
static void activateFocused();
static void scrollFocusedIntoView(); // defined below, once g_scrollY/g_viewportH exist

// Draws the 2px gold-400 focus ring, 2px outset from the element's
// own rect, per spec §9. Called from whichever window's WM_PAINT owns
// the focused item's screen area (ContentProc for canvas items,
// SidebarProc for sidebar items) — that's the "inContent" split above.
void drawFocusRing(HDC hdcScreen, const RECT& r) {
    Graphics g(hdcScreen);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    Pen ringPen(Color(255, GetRValue(GOLD_400), GetGValue(GOLD_400), GetBValue(GOLD_400)), 2.0f);
    g.DrawRectangle(&ringPen, (float)(r.left - 2), (float)(r.top - 2),
                     (float)(r.right - r.left + 4), (float)(r.bottom - r.top + 4));
}

// ── DPI awareness + baseline readability (item 4) ─────────────────
// Every pixel dimension in this file — font heights and layout boxes
// alike — is a hardcoded design-space value. g_dpiScale is the single
// live multiplier both halves of that read from, so a 150%-scaled
// display and the new larger baseline sizes (see createFonts()) each
// apply exactly once rather than compounding. Set once at startup
// (see runGUI()) — startup-time detection only, not live per-monitor
// tracking (WM_DPICHANGED explicitly descoped for this pass).
//
// SetProcessDpiAwarenessContext/GetDpiForSystem are Windows 10 (1607+)
// APIs; this project pins WINVER/_WIN32_WINNT to 0x0601 (Windows 7)
// above, so the SDK headers under that target don't declare either
// the functions or DPI_AWARENESS_CONTEXT at all — not just the -4
// constant. Loaded dynamically via GetProcAddress instead of called
// directly, which needs no header declarations for either and
// degrades safely to g_dpiScale staying 1.0 (today's exact unscaled
// behavior) if they're genuinely absent, rather than failing to
// compile or link.
typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);
typedef UINT (WINAPI *PFN_GetDpiForSystem)(void);

double g_dpiScale = 1.0; // 1.0 = 96 DPI / 100%

// Scales a design-space pixel value (authored at 96 DPI) by the live
// DPI scale factor. Applied at layout call sites to wrap existing
// pixel constants — e.g. RECT r = {x+S(15), S(12), ...} — rather than
// re-deriving each box from font metrics; see the item 4 planning
// discussion for why (mechanical wrap-and-move-on across every
// screen, versus a full layout rewrite that can't be verified without
// a compiler/renderer here).

// Fonts
// Type scale — new named tiers
HFONT g_hFontDisplay  = nullptr;  // 34 semibold
HFONT g_hFontH1       = nullptr;  // 24 semibold
HFONT g_hFontH2       = nullptr;  // 18 semibold
HFONT g_hFontBodyNew  = nullptr;  // 14 regular
HFONT g_hFontLabel    = nullptr;  // 11 semibold uppercase eyebrow
HFONT g_hFontMono     = nullptr;  // 13 tabular mono
HFONT g_hFontMonoSm   = nullptr;  // 11 tabular mono
HFONT g_hFontStat     = nullptr;  // 44 semibold tabular

// Legacy handles — aliases into the above, kept so unmigrated
// screens keep compiling during the incremental redesign.
HFONT g_hFontTitle    = nullptr;
HFONT g_hFontHeading  = nullptr;
HFONT g_hFontBody     = nullptr;
HFONT g_hFontSmall    = nullptr;
HFONT g_hFontButton   = nullptr;

// Cached brushes
static HBRUSH g_hbrSidebar = nullptr;
static HBRUSH g_hbrMain    = nullptr;

// ═════════════════════════════════════════════════════════════
// UTILITY: wide string conversion
// ═════════════════════════════════════════════════════════════
std::string wideToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int sz = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1,
                                  nullptr, 0, nullptr, nullptr);
    std::string s(sz - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1,
                         &s[0], sz, nullptr, nullptr);
    return s;
}


// ═════════════════════════════════════════════════════════════
// COUNT .C FILES IN A FOLDER
// ═════════════════════════════════════════════════════════════
int countCFiles(const std::wstring& folder) {
    int count = 0;
    std::wstring pattern = folder + L"\\*.c";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return 0;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            ++count;
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
    return count;
}

// ═════════════════════════════════════════════════════════════
// FONT MANAGEMENT — 7-step type scale
// ═════════════════════════════════════════════════════════════
// display 34 | h1 24 | h2 18 | body 14 | label 11 | mono 13 | stat 44
//
// Two additions the previous build lacked entirely:
//   · mono  — tabular-numeral monospace. Non-negotiable: the pair
//             comparison table is unreadable without decimal
//             alignment down a column.
//   · label — 11px uppercase +tracking, the gold eyebrow that is
//             the connective tissue of the whole design.
//
// Sizes here are smaller than the previous build (42/24/20/17/18),
// which had been inflated for TV readability. The spec scale
// restores hierarchy — everything was near-equal weight before, so
// nothing read as primary.
static void createFonts() {
    auto mkFont = [](int height, int weight, const wchar_t* face,
                      bool mono = false, int trackingTenths = 0) -> HFONT {
        return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            (mono ? FIXED_PITCH | FF_MODERN : DEFAULT_PITCH | FF_SWISS),
            face);
    };

    // Segoe UI Variable ships on Win11; Segoe UI is the Win10 fallback
    // and GDI resolves it automatically when the variable face is absent.
    const wchar_t* UI_FACE   = L"Segoe UI Variable Text";
    const wchar_t* DISP_FACE = L"Segoe UI Variable Display";
    // Cascadia Mono ships on Win11 / with Terminal; Consolas is the
    // universal fallback and has true tabular numerals.
    const wchar_t* MONO_FACE = L"Cascadia Mono";

    // Item 4 — baseline sizes bumped ~10-15% for readability (34/24/
    // 18/14/11/13/44/11 -> 38/28/20/16/12/14/48/12), independent of
    // and composed with g_dpiScale below (single multiplication: each
    // CreateFontW height is the new baseline design value times the
    // live DPI scale, computed once here — not also scaled again by
    // anything else, so 150% doesn't compound with the larger
    // baseline into oversized text).
    g_hFontDisplay = mkFont(S(38), FW_SEMIBOLD, DISP_FACE);
    g_hFontH1      = mkFont(S(28), FW_SEMIBOLD, DISP_FACE);
    g_hFontH2      = mkFont(S(20), FW_SEMIBOLD, UI_FACE);
    g_hFontBodyNew = mkFont(S(16), FW_NORMAL,   UI_FACE);
    g_hFontLabel   = mkFont(S(12), FW_SEMIBOLD, UI_FACE);
    g_hFontMono    = mkFont(S(14), FW_NORMAL,   MONO_FACE, true);
    g_hFontStat    = mkFont(S(48), FW_SEMIBOLD, DISP_FACE);
    g_hFontMonoSm  = mkFont(S(12), FW_NORMAL,   MONO_FACE, true);

    // Legacy handles — existing screens still bind these. Mapped onto
    // the new scale so every unmigrated screen inherits correct sizing
    // immediately, and gets migrated to the named tiers as it's rebuilt.
    g_hFontTitle   = g_hFontH1;       // 28
    g_hFontHeading = g_hFontH2;       // 20
    g_hFontBody    = g_hFontBodyNew;  // 16
    g_hFontSmall   = g_hFontBodyNew;  // 16
    g_hFontButton  = mkFont(S(16), FW_SEMIBOLD, UI_FACE);

    g_hbrSidebar = CreateSolidBrush(BG_SIDEBAR);
    g_hbrMain    = CreateSolidBrush(BG_MAIN);
}

static void destroyFonts() {
    // Only delete objects this function actually created — the legacy
    // handles are aliases into the same objects, so deleting both
    // would double-free.
    if (g_hFontDisplay) DeleteObject(g_hFontDisplay);
    if (g_hFontH1)      DeleteObject(g_hFontH1);
    if (g_hFontH2)      DeleteObject(g_hFontH2);
    if (g_hFontBodyNew) DeleteObject(g_hFontBodyNew);
    if (g_hFontLabel)   DeleteObject(g_hFontLabel);
    if (g_hFontMono)    DeleteObject(g_hFontMono);
    if (g_hFontMonoSm)  DeleteObject(g_hFontMonoSm);
    if (g_hFontStat)    DeleteObject(g_hFontStat);
    if (g_hFontButton)  DeleteObject(g_hFontButton);
    if (g_hbrSidebar)   DeleteObject(g_hbrSidebar);
    if (g_hbrMain)      DeleteObject(g_hbrMain);
}

// ═════════════════════════════════════════════════════════════
// UPDATE UI BASED ON STATE
// ═════════════════════════════════════════════════════════════
void updateUI() {
    bool canRun = g_state.hasData && !g_state.analysisRunning;

    if (g_hBtnRunAnalysis)  EnableWindow(g_hBtnRunAnalysis,  canRun);
    if (g_hBtnSelectFolder) EnableWindow(g_hBtnSelectFolder, !g_state.analysisRunning);
    if (g_hBtnImportFiles)  EnableWindow(g_hBtnImportFiles,  !g_state.analysisRunning);
    if (g_hBtnGClassroom)   EnableWindow(g_hBtnGClassroom,   !g_state.analysisRunning);
    if (g_hChkAiSummary)    EnableWindow(g_hChkAiSummary,    !g_state.analysisRunning);

    if (g_hStatus) {
        std::wstring statusText;
        if (g_state.analysisRunning) {
            statusText = L" " + g_state.progressMsg;
        } else if (g_state.analysisComplete) {
            statusText = L" Analysis complete — report ready";
        } else if (g_state.hasData) {
            statusText = L" Loaded: ";
            statusText += std::to_wstring(g_state.fileCount);
            statusText += L" file(s) — ready to analyze";
        } else {
            statusText = L" Ready — no data loaded";
        }
        SendMessageW(g_hStatus, SB_SETTEXTW, 0, (LPARAM)statusText.c_str());

        // Update profile count
        std::wstring profileText = L" Profiles: ";
        if (g_state.analysisComplete && g_analysisResults.valid) {
            profileText += std::to_wstring(g_analysisResults.profiles.size());
        } else {
            profileText += L"0";
        }
        SendMessageW(g_hStatus, SB_SETTEXTW, 1, (LPARAM)profileText.c_str());
    }

    if (g_hSidebar) InvalidateRect(g_hSidebar, nullptr, TRUE);
    if (g_hContent) InvalidateRect(g_hContent, nullptr, TRUE);
}

// ═════════════════════════════════════════════════════════════
// FOLDER PICKER
// ═════════════════════════════════════════════════════════════
static bool pickFolder(HWND hwndParent, std::wstring& outPath) {
    BROWSEINFOW bi = {};
    bi.hwndOwner = hwndParent;
    bi.lpszTitle = L"Select folder containing student .c files";
    bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return false;

    wchar_t path[MAX_PATH];
    if (!SHGetPathFromIDListW(pidl, path)) {
        CoTaskMemFree(pidl);
        return false;
    }
    CoTaskMemFree(pidl);
    outPath = path;
    return true;
}

// ═════════════════════════════════════════════════════════════
// FILE PICKER
// ═════════════════════════════════════════════════════════════
static bool pickFiles(HWND hwndParent, std::vector<std::wstring>& outFiles) {
    static wchar_t buffer[8192] = {0};
    buffer[0] = 0;

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner   = hwndParent;
    ofn.lpstrFilter = L"C source files (*.c)\0*.c\0All files (*.*)\0*.*\0\0";
    ofn.lpstrFile   = buffer;
    ofn.nMaxFile    = sizeof(buffer) / sizeof(wchar_t);
    ofn.lpstrTitle  = L"Select .c files to import";
    ofn.Flags       = OFN_EXPLORER | OFN_ALLOWMULTISELECT |
                      OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameW(&ofn)) return false;

    outFiles.clear();
    std::wstring firstPart = buffer;
    size_t pos = firstPart.length() + 1;

    if (buffer[pos] == 0) {
        outFiles.push_back(firstPart);
    } else {
        while (buffer[pos] != 0) {
            std::wstring fname = &buffer[pos];
            outFiles.push_back(firstPart + L"\\" + fname);
            pos += fname.length() + 1;
        }
    }
    return !outFiles.empty();
}

// ═════════════════════════════════════════════════════════════
// IMPORT FILES TO FOLDER
// ═════════════════════════════════════════════════════════════
static bool importFilesToFolder(const std::vector<std::wstring>& files,
                                  std::wstring& outFolder)
{
    outFolder = L"imported_data";
    CreateDirectoryW(outFolder.c_str(), nullptr);

    int copied = 0;
    for (const auto& srcPath : files) {
        size_t slash = srcPath.find_last_of(L"\\/");
        std::wstring fname = (slash == std::wstring::npos)
            ? srcPath : srcPath.substr(slash + 1);
        std::wstring destPath = outFolder + L"\\" + fname;
        if (CopyFileW(srcPath.c_str(), destPath.c_str(), FALSE))
            ++copied;
    }
    return copied > 0;
}


// ═════════════════════════════════════════════════════════════
// MENU CREATION
// ═════════════════════════════════════════════════════════════
static HMENU createMainMenu() {
    HMENU hMenu = CreateMenu();

    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_OPEN_FOLDER,
                L"&Open Data Folder...\tCtrl+O");
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_IMPORT,
                L"&Import .c Files...\tCtrl+I");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_GCLASSROOM,
                L"Sync with &Google Classroom...");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXIT,
                L"E&xit\tAlt+F4");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");

    HMENU hViewMenu = CreatePopupMenu();
    AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_REPORT,
                L"Open Latest &Report in Browser");
    AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_PROFILES,
                L"View Student &Profiles");
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_AUDIT_LOG,
                L"View &Audit Log");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hViewMenu, L"&View");

    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_DOCUMENTATION,
                L"&Documentation");
    AppendMenuW(hHelpMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hHelpMenu, MF_STRING, ID_HELP_ABOUT,
                L"&About");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");

    return hMenu;
}

// ═════════════════════════════════════════════════════════════
// CUSTOM TITLE BAR (spec §5.1)
// ═════════════════════════════════════════════════════════════
// Replaces the default Win32 caption, which broke the dark theme
// every time it appeared. The menu bar is NOT deleted — it's
// rehosted here as custom-drawn hit areas that open the real
// HMENU popups via TrackPopupMenu, so every existing menu command
// keeps working unchanged.

static HWND  g_hTitleBar   = nullptr;
static HMENU g_hAppMenu    = nullptr;   // owned; submenus opened on click
static int   g_hoverMenuIdx  = -1;      // 0=File 1=View 2=Help, -1=none
static int   g_hoverWinBtn   = -1;      // 0=min 1=max 2=close, -1=none
static int   g_openMenuIdx   = -1;      // which popup is currently showing

static const wchar_t* TB_MENU_LABELS[3] = { L"File", L"View", L"Help" };
static RECT g_tbMenuRects[3] = {};
static RECT g_tbWinBtnRects[3] = {};

static LRESULT CALLBACK TitleBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void registerTitleBarClass(HINSTANCE hInst) {
    static bool done = false;
    if (done) return;
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = TitleBarProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"CALSSTitleBar";
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);
    done = true;
}

// Opens the real popup menu beneath its title-bar label.
static void showTitleBarMenu(HWND hwnd, int idx) {
    if (!g_hAppMenu || idx < 0 || idx > 2) return;
    HMENU sub = GetSubMenu(g_hAppMenu, idx);
    if (!sub) return;

    RECT r = g_tbMenuRects[idx];
    POINT pt = { r.left, r.bottom };
    ClientToScreen(hwnd, &pt);

    g_openMenuIdx = idx;
    InvalidateRect(hwnd, nullptr, FALSE);

    // Commands route to the main window, where handleMenuCommand lives.
    TrackPopupMenu(sub, TPM_LEFTALIGN | TPM_TOPALIGN,
                   pt.x, pt.y, 0, g_hMainWindow, nullptr);

    g_openMenuIdx = -1;
    InvalidateRect(hwnd, nullptr, FALSE);
}

// Title bar window-control button action (minimize/maximize/close).
// Shared by the mouse click handler and activateFocused()'s
// FocusKind::TitleBarWinBtn case (item 2) — same principle as every
// shared handler in this pass.
static void titleBarWinBtnAction(int i) {
    if (i == 0) ShowWindow(g_hMainWindow, SW_MINIMIZE);
    else if (i == 1)
        ShowWindow(g_hMainWindow, IsZoomed(g_hMainWindow) ? SW_RESTORE : SW_MAXIMIZE);
    else if (i == 2)
        PostMessageW(g_hMainWindow, WM_CLOSE, 0, 0);
}

static LRESULT CALLBACK TitleBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdcScreen = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);

            // Item 4 — TITLEBAR_HEIGHT is also used unscaled to size
            // the real window in layoutChildren(); cached once here so
            // every RECT below matches that actual (scaled) window
            // height rather than the raw 40px design constant.
            int tbH = S(TITLEBAR_HEIGHT);

            // Double-buffer so the gold rule and hover states don't flicker
            HDC hdc = CreateCompatibleDC(hdcScreen);
            HBITMAP bmp = CreateCompatibleBitmap(hdcScreen, rc.right, rc.bottom);
            HBITMAP oldBmp = (HBITMAP)SelectObject(hdc, bmp);

            HBRUSH bg = CreateSolidBrush(GRAY_950);
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            SetBkMode(hdc, TRANSPARENT);

            // Keyboard focus (item 2): rebuilt fresh each paint, same
            // pattern as every other focusables list in this pass.
            g_titleBarFocusables.clear();

            // App name — 13px gray-400, left
            HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBodyNew);
            SetTextColor(hdc, GRAY_400);
            RECT nameR = { S(16), 0, S(220), tbH };
            DrawTextW(hdc, L"CALSS", -1, &nameR,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            SIZE nameSz;
            GetTextExtentPoint32W(hdc, L"CALSS", 5, &nameSz);
            int cx = S(16) + nameSz.cx + S(24);

            // Menu labels — 13px gray-200, 8x12 hit padding, gray-800 hover
            for (int i = 0; i < 3; ++i) {
                SIZE sz;
                GetTextExtentPoint32W(hdc, TB_MENU_LABELS[i],
                                      (int)wcslen(TB_MENU_LABELS[i]), &sz);
                RECT r = { cx, S(6), cx + sz.cx + S(24), tbH - S(6) };
                g_tbMenuRects[i] = r;
                addFocusable(g_titleBarFocusables, FocusKind::TitleBarMenu, r, i, false);

                if (i == g_openMenuIdx || i == g_hoverMenuIdx) {
                    HBRUSH hb = CreateSolidBrush(GRAY_800);
                    FillRect(hdc, &r, hb);
                    DeleteObject(hb);
                }
                SetTextColor(hdc, (i == g_openMenuIdx) ? WHITE_ : GRAY_200);
                DrawTextW(hdc, TB_MENU_LABELS[i], -1, &r,
                          DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                cx = r.right + S(4);
            }

            // Window controls — right aligned, 46px wide each
            const int btnW = S(46);
            for (int i = 0; i < 3; ++i) {
                RECT r = { rc.right - btnW * (3 - i), 0,
                           rc.right - btnW * (2 - i), tbH };
                g_tbWinBtnRects[i] = r;
                addFocusable(g_titleBarFocusables, FocusKind::TitleBarWinBtn, r, i, false);

                if (i == g_hoverWinBtn) {
                    // Close gets the risk color on hover; others a neutral lift
                    HBRUSH hb = CreateSolidBrush(i == 2 ? RISK_HIGH : GRAY_800);
                    FillRect(hdc, &r, hb);
                    DeleteObject(hb);
                }

                SetTextColor(hdc, (i == 2 && i == g_hoverWinBtn) ? WHITE_ : GRAY_200);

                // Draw glyphs as vectors — Segoe MDL2 isn't guaranteed present
                int midX = (r.left + r.right) / 2;
                int midY = tbH / 2;
                HPEN pen = CreatePen(PS_SOLID, 1,
                    (i == 2 && i == g_hoverWinBtn) ? WHITE_ : GRAY_200);
                HPEN oldPen = (HPEN)SelectObject(hdc, pen);

                if (i == 0) {                       // minimize
                    MoveToEx(hdc, midX - S(5), midY, nullptr);
                    LineTo(hdc, midX + S(6), midY);
                } else if (i == 1) {                // maximize / restore
                    bool maxed = IsZoomed(g_hMainWindow);
                    if (maxed) {
                        Rectangle(hdc, midX - S(6), midY - S(3), midX + S(3), midY + S(6));
                        MoveToEx(hdc, midX - S(3), midY - S(3), nullptr);
                        LineTo(hdc, midX - S(3), midY - S(6));
                        LineTo(hdc, midX + S(6), midY - S(6));
                        LineTo(hdc, midX + S(6), midY + S(3));
                        LineTo(hdc, midX + S(3), midY + S(3));
                    } else {
                        HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH);
                        HBRUSH ob = (HBRUSH)SelectObject(hdc, nb);
                        Rectangle(hdc, midX - S(5), midY - S(5), midX + S(6), midY + S(6));
                        SelectObject(hdc, ob);
                    }
                } else {                            // close
                    MoveToEx(hdc, midX - S(5), midY - S(5), nullptr);
                    LineTo(hdc, midX + S(6), midY + S(6));
                    MoveToEx(hdc, midX + S(5), midY - S(5), nullptr);
                    LineTo(hdc, midX - S(6), midY + S(6));
                }
                SelectObject(hdc, oldPen);
                DeleteObject(pen);
            }

            // The 2px gold rule — the one piece of existing identity
            // the brief keeps verbatim.
            HBRUSH gold = CreateSolidBrush(GOLD_500);
            RECT goldR = { 0, tbH - S(GOLD_RULE_HEIGHT),
                           rc.right, tbH };
            FillRect(hdc, &goldR, gold);
            DeleteObject(gold);

            SelectObject(hdc, oldFont);
            BitBlt(hdcScreen, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);
            SelectObject(hdc, oldBmp);
            DeleteObject(bmp);
            DeleteDC(hdc);

            // Keyboard focus ring (item 2) — title bar is the LAST
            // region in the combined index (see g_titleBarFocusables'
            // declaration for why it's folded into that index rather
            // than kept as its own separate system like the sync
            // sheet). Its range starts right after sidebar+content.
            int titleBarBase = (int)(g_sidebarFocusables.size() + g_contentFocusables.size());
            if (g_focusVisible && g_focusedIndex >= titleBarBase &&
                g_focusedIndex < titleBarBase + (int)g_titleBarFocusables.size())
            {
                drawFocusRing(hdcScreen, g_titleBarFocusables[g_focusedIndex - titleBarBase].rect);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND: return 1;

        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            int newMenu = -1, newBtn = -1;
            for (int i = 0; i < 3; ++i)
                if (PtInRect(&g_tbMenuRects[i], pt)) newMenu = i;
            for (int i = 0; i < 3; ++i)
                if (PtInRect(&g_tbWinBtnRects[i], pt)) newBtn = i;

            if (newMenu != g_hoverMenuIdx || newBtn != g_hoverWinBtn) {
                g_hoverMenuIdx = newMenu;
                g_hoverWinBtn  = newBtn;
                InvalidateRect(hwnd, nullptr, FALSE);
                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
            }

            // Hovering an open menu slides the popup across, matching
            // native menu-bar behavior.
            if (g_openMenuIdx >= 0 && newMenu >= 0 && newMenu != g_openMenuIdx) {
                EndMenu();
                showTitleBarMenu(hwnd, newMenu);
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            g_hoverMenuIdx = -1;
            g_hoverWinBtn  = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_LBUTTONDOWN: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            for (int i = 0; i < 3; ++i) {
                if (PtInRect(&g_tbMenuRects[i], pt)) {
                    showTitleBarMenu(hwnd, i);
                    return 0;
                }
            }
            for (int i = 0; i < 3; ++i) {
                if (PtInRect(&g_tbWinBtnRects[i], pt)) {
                    titleBarWinBtnAction(i);
                    return 0;
                }
            }

            // Empty title-bar area drags the window.
            ReleaseCapture();
            SendMessageW(g_hMainWindow, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return 0;
        }

        case WM_LBUTTONDBLCLK:
            ShowWindow(g_hMainWindow,
                       IsZoomed(g_hMainWindow) ? SW_RESTORE : SW_MAXIMIZE);
            return 0;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// ═════════════════════════════════════════════════════════════
// LAYOUT MANAGEMENT
// ═════════════════════════════════════════════════════════════
static void layoutChildren(HWND hwndParent) {
    RECT rect;
    GetClientRect(hwndParent, &rect);
    int totalW = rect.right;
    int totalH = rect.bottom;

    if (g_hStatus) SendMessage(g_hStatus, WM_SIZE, 0, 0);

    RECT statusRect = {0};
    if (g_hStatus) GetWindowRect(g_hStatus, &statusRect);
    int statusH = statusRect.bottom - statusRect.top;
    if (statusH < S(20)) statusH = S(STATUSBAR_HEIGHT);

    // Custom title bar spans the full width at the top
    if (g_hTitleBar)
        MoveWindow(g_hTitleBar, 0, 0, totalW, S(TITLEBAR_HEIGHT), TRUE);

    int bodyTop = S(TITLEBAR_HEIGHT);
    int contentH = totalH - statusH - bodyTop;
    if (contentH < 0) contentH = 0;

    if (g_hSidebar)
        MoveWindow(g_hSidebar, 0, bodyTop, S(SIDEBAR_WIDTH), contentH, TRUE);
    if (g_hContent)
        MoveWindow(g_hContent, S(SIDEBAR_WIDTH), bodyTop,
                    totalW - S(SIDEBAR_WIDTH), contentH, TRUE);
}

// ═════════════════════════════════════════════════════════════
// SIDEBAR PROCEDURE
// ═════════════════════════════════════════════════════════════
static void layoutSidebarControls(HWND hSidebar);  // defined near createSidebarButtons

// Forward declarations — these primitives are defined later in the
// file (near drawStatsDashboard/drawProgressBar) but SidebarProc's
// WM_PAINT now uses them for zone-based rendering.

static LRESULT CALLBACK SidebarProc(HWND hwnd, UINT uMsg,
                                      WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_COMMAND:
            // Forward button clicks to main window
            SendMessage(g_hMainWindow, WM_COMMAND, wParam, lParam);
            return 0;

        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND ctl = (HWND)lParam;
            SetBkMode(hdc, TRANSPARENT);
            bool hovered  = (ctl == g_hoveredBtn);
            bool disabled = !IsWindowEnabled(ctl);

            // PRIMARY — maroon fill, white text. Previously all four
            // buttons were near-identical white blocks and the eye
            // could not find the primary (broken item #4).
            if (ctl == g_hBtnRunAnalysis) {
                COLORREF bg = disabled ? GRAY_800
                            : (hovered ? MAROON_600 : MAROON_700);
                SetBkColor(hdc, bg);
                SetTextColor(hdc, disabled ? GRAY_500 : WHITE_);
                static HBRUSH runBr = nullptr;
                if (runBr) DeleteObject(runBr);
                runBr = CreateSolidBrush(bg);
                return (LRESULT)runBr;
            }

            // The AI checkbox reads as body text, not as a button
            if (ctl == g_hChkAiSummary) {
                SetBkColor(hdc, GRAY_950);
                SetTextColor(hdc, disabled ? GRAY_500 : GRAY_200);
                static HBRUSH chkBr = nullptr;
                if (chkBr) DeleteObject(chkBr);
                chkBr = CreateSolidBrush(GRAY_950);
                return (LRESULT)chkBr;
            }

            // SECONDARY — gray-800 fill, gray-200 text
            COLORREF bg = disabled ? GRAY_900
                        : (hovered ? GRAY_700 : GRAY_800);
            SetBkColor(hdc, bg);
            SetTextColor(hdc, disabled ? GRAY_500
                            : (hovered ? WHITE_ : GRAY_200));
            static HBRUSH ghostBr = nullptr;
            if (ghostBr) DeleteObject(ghostBr);
            ghostBr = CreateSolidBrush(bg);
            return (LRESULT)ghostBr;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdcScreen = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);

            // Double-buffer — the sidebar repaints on every hover and
            // every resize-driven re-pin, so flicker is visible without this.
            HDC hdc = CreateCompatibleDC(hdcScreen);
            HBITMAP bmp = CreateCompatibleBitmap(hdcScreen, rect.right, rect.bottom);
            HBITMAP oldBmp = (HBITMAP)SelectObject(hdc, bmp);

            HBRUSH bgBr = CreateSolidBrush(GRAY_950);
            FillRect(hdc, &rect, bgBr);
            DeleteObject(bgBr);

            // Hairline divider against the content canvas
            HPEN borderPen = CreatePen(PS_SOLID, 1, GRAY_700);
            HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
            MoveToEx(hdc, rect.right - 1, 0, nullptr);
            LineTo(hdc, rect.right - 1, rect.bottom);
            SelectObject(hdc, oldPen);
            DeleteObject(borderPen);

            SetBkMode(hdc, TRANSPARENT);
            HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBodyNew);

            const int px = S(16);
            const int pw = rect.right - px * 2;

            // ── HEADER: UL crest + wordmark ───────────────────
            int sealR  = S(30);
            int sealCx = rect.right / 2;
            int sealCy = S(20) + sealR;
            {
                Graphics g(hdc);
                g.SetSmoothingMode(SmoothingModeAntiAlias);
                Image* logo = getULLogo();
                if (logo) {
                    // Drawn ~20% larger than the old flat circle and
                    // re-centered on the same point -- a real badge photo
                    // reads smaller than a flat gold fill did at the same
                    // radius.
                    int r2 = (int)(sealR * 1.2);
                    g.DrawImage(logo, sealCx - r2, sealCy - r2, r2 * 2, r2 * 2);
                } else {
                    Pen ring(Color(255, GetRValue(GOLD_500),
                                   GetGValue(GOLD_500), GetBValue(GOLD_500)), 2.0f);
                    SolidBrush fill(Color(255, GetRValue(MAROON_800),
                                          GetGValue(MAROON_800), GetBValue(MAROON_800)));
                    g.FillEllipse(&fill, sealCx - sealR, sealCy - sealR, sealR * 2, sealR * 2);
                    g.DrawEllipse(&ring, sealCx - sealR, sealCy - sealR, sealR * 2, sealR * 2);
                    Pen inner(Color(120, GetRValue(GOLD_500),
                                    GetGValue(GOLD_500), GetBValue(GOLD_500)), 1.0f);
                    g.DrawEllipse(&inner, sealCx - sealR + S(6), sealCy - sealR + S(6),
                                  (sealR - S(6)) * 2, (sealR - S(6)) * 2);
                }
            }

            int cy = sealCy + sealR + S(12);
            SelectObject(hdc, g_hFontBodyNew);
            SetTextColor(hdc, GRAY_200);
            RECT nameR = {px, cy, rect.right - px, cy + S(18)};
            DrawTextW(hdc, L"UNIVERSITY OF LUZON", -1, &nameR, DT_CENTER | DT_SINGLELINE);
            cy += S(18);
            SetTextColor(hdc, GRAY_500);
            RECT ccsR = {px, cy, rect.right - px, cy + S(16)};
            DrawTextW(hdc, L"College of Computer Studies", -1, &ccsR, DT_CENTER | DT_SINGLELINE);
            cy += S(20);

            HPEN goldRule = CreatePen(PS_SOLID, 1, GOLD_500);
            HPEN oldGold = (HPEN)SelectObject(hdc, goldRule);
            MoveToEx(hdc, px, cy, nullptr);
            LineTo(hdc, rect.right - px, cy);
            SelectObject(hdc, oldGold);
            DeleteObject(goldRule);

            // ── ZONE 1 — DATA SOURCE ──────────────────────────
            // Fixed at 196 (design px, scaled) to match
            // layoutSidebarControls()'s button origin — see the
            // comment there for the coupling.
            cy = S(196);
            drawEyebrow(hdc, px, cy, pw, L"1  DATA SOURCE");
            cy += S(18);

            // Folder chip — monospaced path, muted when empty
            {
                RECT chip = {px, cy, rect.right - px, cy + S(38)};
                drawCard(hdc, chip, GRAY_850, GRAY_700, 6);

                SelectObject(hdc, g_hFontMonoSm);
                std::wstring shown;
                if (g_state.hasData && !g_state.dataFolder.empty()) {
                    SetTextColor(hdc, GRAY_200);
                    std::wstring full = g_state.dataFolder;
                    size_t last = full.find_last_of(L"\\/");
                    shown = (last != std::wstring::npos) ? full.substr(last + 1) : full;
                } else {
                    SetTextColor(hdc, GRAY_500);
                    shown = L"No folder selected";
                }
                RECT chipTxt = {px + S(10), cy, rect.right - px - S(10), cy + S(38)};
                shown = truncateMiddle(hdc, shown, (rect.right - px - S(10)) - (px + S(10)));
                DrawTextW(hdc, shown.c_str(), -1, &chipTxt,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

            // Buttons occupy 196+18+38+10 .. +3*(BUTTON_HEIGHT+8)
            int zone1End = S(196) + S(18) + S(38) + S(10) + (S(BUTTON_HEIGHT) + S(8)) * 3;

            // ── ZONE 2 — IMPORTED ─────────────────────────────
            cy = zone1End + S(8);
            drawEyebrow(hdc, px, cy, pw, L"2  IMPORTED");
            cy += S(20);

            if (!g_state.hasData && !g_state.analysisComplete) {
                // Broken item #5: never show em-dash placeholders.
                // Before any import there is simply nothing to report.
                SelectObject(hdc, g_hFontBodyNew);
                SetTextColor(hdc, GRAY_500);
                RECT emptyR = {px, cy, rect.right - px, cy + S(20)};
                DrawTextW(hdc, L"No data loaded", -1, &emptyR,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            } else {
                struct Row { std::wstring label; std::wstring value; };
                std::vector<Row> rows;
                rows.push_back({L"Files", std::to_wstring(g_state.fileCount)});
                if (g_state.analysisComplete && g_analysisResults.valid) {
                    rows.push_back({L"Students",
                        std::to_wstring(g_analysisResults.profiles.size())});
                    rows.push_back({L"Flagged pairs",
                        std::to_wstring(g_analysisResults.flaggedCount)});
                }
                for (const auto& r : rows) {
                    SelectObject(hdc, g_hFontBodyNew);
                    SetTextColor(hdc, GRAY_400);
                    RECT lR = {px, cy, rect.right - S(80), cy + S(20)};
                    DrawTextW(hdc, r.label.c_str(), -1, &lR,
                              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(hdc, g_hFontMonoSm);
                    SetTextColor(hdc, GRAY_200);
                    RECT vR = {rect.right - S(80), cy, rect.right - px, cy + S(20)};
                    DrawTextW(hdc, r.value.c_str(), -1, &vR,
                              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
                    cy += S(22);
                }
            }

            // ── ZONE 3 — ANALYSIS (bottom-pinned) ─────────────
            // Mirrors layoutSidebarControls(): run button sits at
            // h - 16 - 20 - 44, checkbox 10px above it.
            {
                int runY   = rect.bottom - S(16) - S(20) - S(44);
                int chkY   = runY - S(22) - S(10);
                int eyebrowY = chkY - S(26);

                drawEyebrow(hdc, px, eyebrowY, pw, L"3  ANALYSIS");

                // Status line beneath the primary action
                SelectObject(hdc, g_hFontBodyNew);
                int statusY = runY + S(44) + S(4);
                if (g_state.analysisRunning) {
                    SetTextColor(hdc, GOLD_500);
                    RECT sR = {px, statusY, rect.right - px, statusY + S(20)};
                    std::wstring msg = truncateMiddle(hdc, g_state.progressMsg, pw);
                    DrawTextW(hdc, msg.c_str(), -1, &sR,
                              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                } else if (g_state.analysisComplete) {
                    SetTextColor(hdc, RISK_LOW);
                    RECT sR = {px, statusY, rect.right - px, statusY + S(20)};
                    DrawTextW(hdc, L"Analysis complete", -1, &sR,
                              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                }
            }

            SelectObject(hdc, oldFont);
            BitBlt(hdcScreen, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY);
            SelectObject(hdc, oldBmp);
            DeleteObject(bmp);
            DeleteDC(hdc);

            // Keyboard focus (compliance sweep item 2) — rebuild the
            // sidebar's region of the combined focusable list fresh
            // each paint (same "rebuild every frame" pattern as every
            // other hit-rect vector in this file), then draw the ring
            // if the current combined focus resolves to one of these.
            // Disabled controls are skipped — nothing to activate.
            g_sidebarFocusables.clear();
            struct SidebarFocusDef { HWND h; FocusKind kind; int cmdId; };
            SidebarFocusDef defs[5] = {
                { g_hBtnSelectFolder, FocusKind::SidebarBtn,       ID_BTN_SELECT_FOLDER },
                { g_hBtnImportFiles,  FocusKind::SidebarBtn,       ID_BTN_IMPORT_FILES  },
                { g_hBtnGClassroom,   FocusKind::SidebarBtn,       ID_BTN_GCLASSROOM    },
                { g_hChkAiSummary,    FocusKind::SidebarCheckbox,  ID_CHK_AI_SUMMARY    },
                { g_hBtnRunAnalysis,  FocusKind::SidebarBtn,       ID_BTN_RUN_ANALYSIS  },
            };
            for (const auto& d : defs) {
                if (!d.h || !IsWindowVisible(d.h) || !IsWindowEnabled(d.h)) continue;
                RECT r = childRectInParent(d.h, hwnd);
                addFocusable(g_sidebarFocusables, d.kind, r, d.cmdId, false);
            }

            FocusableItem* focused = focusableAt(g_focusedIndex);
            if (g_focusVisible && focused && !focused->inContent &&
                g_focusedIndex < (int)g_sidebarFocusables.size())
            {
                drawFocusRing(hdcScreen, focused->rect);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }


        case WM_SIZE:
            // Re-pin zone ③ (AI toggle + Run analysis) to the bottom
            // on every resize, so the primary action never drifts.
            layoutSidebarControls(hwnd);
            return 0;

        case WM_MOUSEMOVE: {
            // Find which child button is under cursor
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            HWND child = ChildWindowFromPoint(hwnd, pt);
            if (child != g_hoveredBtn) {
                g_hoveredBtn = child;
                // Request mouse leave notification
                TRACKMOUSEEVENT tme = {};
                tme.cbSize    = sizeof(tme);
                tme.dwFlags   = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                // Repaint all buttons
                if (g_hBtnSelectFolder)  InvalidateRect(g_hBtnSelectFolder,  nullptr, TRUE);
                if (g_hBtnImportFiles)   InvalidateRect(g_hBtnImportFiles,   nullptr, TRUE);
                if (g_hBtnGClassroom)    InvalidateRect(g_hBtnGClassroom,    nullptr, TRUE);
                if (g_hBtnRunAnalysis)   InvalidateRect(g_hBtnRunAnalysis,   nullptr, TRUE);
            }
            return 0;
        }

        case WM_MOUSELEAVE: {
            g_hoveredBtn = nullptr;
            if (g_hBtnSelectFolder)  InvalidateRect(g_hBtnSelectFolder,  nullptr, TRUE);
            if (g_hBtnImportFiles)   InvalidateRect(g_hBtnImportFiles,   nullptr, TRUE);
            if (g_hBtnGClassroom)    InvalidateRect(g_hBtnGClassroom,    nullptr, TRUE);
            if (g_hBtnRunAnalysis)   InvalidateRect(g_hBtnRunAnalysis,   nullptr, TRUE);
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}
// ═════════════════════════════════════════════════════════════

// Global scroll state for content area
int g_scrollY    = 0;     // Current scroll offset
static int g_contentH   = 0;     // Total content height
int g_viewportH  = 0;     // Visible area height

ResultsPage g_currentPage = PAGE_OVERVIEW;
int g_selectedStudent = -1;  // -1 = grid view, otherwise index in profiles

static TabRect g_tabs[3] = {};

// Sliding underline animation (spec §6: 200ms, cubic-bezier ease-out).
// g_tabUnderlineX is the animated CURRENT position; target is set on
// tab switch and a timer interpolates toward it each frame.
static float g_tabUnderlineX = 0, g_tabUnderlineW = 0;
static float g_tabUnderlineTargetX = 0, g_tabUnderlineTargetW = 0;
static bool  g_tabUnderlineInit = false;

// Utility cluster hit regions (Export / Search / Help), right-aligned
// in the tab row.
static RECT g_exportBtnRect = {};
static RECT g_searchBtnRect = {};
static bool g_exportMenuOpen = false;
static bool g_searchActive  = false;
static HWND g_hSearchEdit   = nullptr; // real EDIT control, shown/hidden on toggle
std::wstring g_searchFilterLower; // lowercased, updated live from the edit control

// Classes grid sort control (spec §5.8: "Add Sort ▾ (name / submissions
// / flagged appearances)"). Cycles on click rather than a full popup
// menu — same lightweight pattern already used for "Filter: All"
// elsewhere in this file.
ClassesSortMode g_classesSortMode = SORT_NAME;
RECT g_sortControlRect = {};

// Sticky context strip's Re-run button
static RECT g_rerunBtnRect = {};

// Language-detection notice's "Show files" disclosure (spec §5.7) —
// collapsed by default, since this is a data-quality note, not a
// similarity finding worth competing with the statistics for attention.
bool g_langNoticeExpanded = false;
RECT g_langNoticeToggleRect = {};

// Student detail page's Flagged Appearances rows — clickable,
// deep-linking to Flagged Pairs (spec §5.8: "Rows are clickable →
// deep-link to the pair").
std::vector<FlaggedAppearanceRowRect> g_flaggedAppearanceRowRects;


// Ease-out cubic: 1 - (1-t)^3. Shared by every 0..1 animation-progress
// helper in this file (stat countup, pair expand/collapse, ...) so the
// easing curve only needs to be defined — and tuned — in one place.


// ── Block sub-tabs (Classes page only) ────────────────────────
// A second row of tabs shown only on the Classes page, one per
// block the instructor has synced/organized (e.g. "Programming 1 -
// BSIT Block 1"). Rebuilt from student profile data on every paint
// since blocks depend entirely on what's been synced. -1 = "All".
int g_currentBlockTab = -1; // -1 = "All", else index into g_blockTabNames
// g_blockTabNames moved to gui_students.cpp (Checkpoint 3 of the
// ContentProc split) — exclusively used there.
std::vector<BlockTabRect> g_blockTabRects;

// Per-file Assign/Delete button hit regions on the Unclassified card
// (Overview tab). Rebuilt each paint since the file list can change
// between analysis runs.
std::vector<UnclassifiedActionRect> g_unclassifiedActionRects;

// ── Empty-state option cards (spec §5.3) ──────────────────────
// Three horizontal cards that mirror the sidebar's source actions,
// so a first-time user has an actionable invitation on the canvas
// rather than a paragraph pointing at the sidebar.
std::vector<EmptyCardRect> g_emptyCardRects;
int g_hoveredEmptyCard = -1;

// Student card hit regions (rebuilt during paint)
std::vector<StudentCardRect> g_studentCardRects;

// Hit-test regions for clickable elements
// g_backButtonRect moved to gui_students.cpp (Checkpoint 3 of the
// ContentProc split) — exclusively used there.
RECT g_reportButtonRect = {};
RECT g_exactDuplicatesCardRect = {};
RECT g_cancelAnalysisBtnRect   = {}; // spec §5.5: Cancel analysis
static RECT g_helpLinkRect = {};
bool g_filterExactDuplicatesOnly = false;
RECT g_clearFilterButtonRect = {};

// ── Flagged Pairs severity filter chips (spec §5.9: "Flagged pairs
// (21) + severity filter chips All · High 13 · Moderate 8") ───
// Independent of g_filterExactDuplicatesOnly above: that boolean still
// drives the Overview "Exact Duplicates" stat-card deep link and its
// own clear-filter banner unchanged. Severity is a second, ANDed
// filter dimension — a pair must pass both to be shown. Two tiers
// only (High/Moderate), per spec's literal mockup — Low is a valid
// pairSeverityColor() tier but flagged pairs in practice sit at
// combinedScore >= (whatever analyzePair's flag threshold is, not
// attached), which per the current data is always >= 70%, so a Low
// chip would either always read 0 or duplicate "All". Confirmed with
// user: two-chip set only.
PairSeverityFilter g_pairSeverityFilter = SEV_FILTER_ALL;
std::vector<SeverityChipRect> g_severityChipRects;
// SEVERITY_CHIP_BAR_HEIGHT moved to gui_flagged.cpp (Checkpoint 2 of
// the ContentProc split) — exclusively used by drawFlaggedPairs there.

// Returns true if a pair's severity tier matches the given filter.
// SEV_FILTER_ALL always passes. Not static as of Checkpoint 2 of the
// ContentProc split: also called from copyFindingsForPair() here in
// gui.cpp, and from drawFlaggedPairs() in gui_flagged.cpp.
bool pairMatchesSeverityFilter(double combinedScorePct, PairSeverityFilter f) {
    if (f == SEV_FILTER_ALL) return true;
    if (f == SEV_FILTER_HIGH) return combinedScorePct >= 85.0;
    // SEV_FILTER_MODERATE: everything below High. This intentionally
    // also catches sub-70% "Low" pairs so no flagged pair silently
    // disappears from every chip if one ever does score under 70%.
    return combinedScorePct < 85.0;
}

// ── Flagged pair collapse/expand (spec §5.9: "Collapse pairs by
// default... 96px summary row; clicking expands it in place with a
// 240ms height animation"). Previously every pair rendered always
// fully expanded (tech doc §5) — this is new state, not a refactor
// of anything pre-existing.
//
// Indexed by position in g_analysisResults.flaggedPairs (the stable,
// unfiltered index), NOT the sequential on-screen pairNum, so expand
// state survives a chip-filter change instead of pointing at a
// different pair once the visible list is re-numbered. Not static as
// of Checkpoint 2 of the ContentProc split: also read by
// drawFlaggedPairs() in gui_flagged.cpp.
std::set<int> g_expandedPairs;
// PAIR_COLLAPSED_ROW_HEIGHT and pairExpandAnimProgress() moved to
// gui_flagged.cpp (Checkpoint 2) — exclusively used by
// drawFlaggedPairCollapsedRow()/drawFlaggedPairs() there.

std::map<int, PairAnimEntry> g_pairAnimating;

// Toggles a pair's expanded state and kicks off its open/close
// animation. Called from the click handler for both the collapsed
// row and the expanded block's header (which now also acts as the
// collapse trigger).
void togglePairExpanded(HWND hwnd, int pairIdx) {
    bool nowExpanded;
    auto found = g_expandedPairs.find(pairIdx);
    if (found != g_expandedPairs.end()) {
        g_expandedPairs.erase(found);
        nowExpanded = false;
    } else {
        g_expandedPairs.insert(pairIdx);
        nowExpanded = true;
    }
    PairAnimEntry anim;
    anim.startTick = GetTickCount();
    anim.opening = nowExpanded;
    g_pairAnimating[pairIdx] = anim;
    SetTimer(hwnd, TIMER_PAIR_EXPAND, 15, nullptr);
    InvalidateRect(hwnd, nullptr, FALSE);
}

// Collapsed-row hit regions, rebuilt each paint (spec §5.9 mockup —
// same per-item hit-rect-vector pattern as g_blockTabRects /
// g_dnaCellHits elsewhere in this file, not a single shared rect).
std::vector<CollapsedPairRowRect> g_collapsedPairRowRects;

// Expanded block's header band also toggles (collapse) — same vector
// pattern, one hit-rect per currently-drawn expanded pair's header.
std::vector<ExpandedPairHeaderRect> g_expandedPairHeaderRects;

// ── "Show all 14 features" disclosure (spec §5.9, line 298) ──────
// Per pair, per side (studentA/studentB each get their own table and
// their own toggle). Key = pairIdx*2 + side, side 0 = studentA, 1 =
// studentB — a plain flat set is enough since these are small ints
// and don't need a richer key type.
// Not static as of Checkpoint 2 of the ContentProc split: also read
// by drawAuthorshipCard()/estimateAuthorshipCardHeight() in
// gui_flagged.cpp. showAllFeaturesKey() itself moved there — it was
// exclusively used by those two.
std::set<int> g_showAllFeaturesKeys;
std::vector<ShowAllFeaturesRect> g_showAllFeaturesRects;

// ── "Copy findings" clipboard button (spec §5.9, verdict banner) ──
// New feature — tech doc §5 confirms nothing like this exists yet
// ("No Copy findings clipboard action on the verdict/comparative
// box"), so this is built as a per-pair vector from the start rather
// than starting as a single global and needing a later fix, same
// shape as g_showAllFeaturesRects/g_dnaCellHits above.
std::vector<CopyFindingsBtnRect> g_copyFindingsBtnRects;
// Brief "Copied!" confirmation shown on the clicked pair's button.
// Not static as of Checkpoint 2 of the ContentProc split: also read
// by drawComparativeBox() in gui_flagged.cpp.
int   g_copyFindingsFeedbackPairIdx = -1;
DWORD g_copyFindingsFeedbackUntilTick = 0;

// UTF-8 to wide string helper
std::wstring s2w(const std::string& s) {
    if (s.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(sz - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], sz);
    return w;
}

// narrow string helper (wide -> UTF-8), used by audit log and Classroom code
std::string toNarrowGC(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    return s;
}

// Format a double as percentage string
std::wstring fmtPct(double v) {
    std::wostringstream oss;
    oss << std::fixed << std::setprecision(1) << v << L"%";
    return oss.str();
}

// Update scrollbar based on content height
static void updateScrollbar(HWND hwnd) {
    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin   = 0;
    si.nMax   = g_contentH;
    si.nPage  = g_viewportH;
    si.nPos   = g_scrollY;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

// Ensures the currently focused item (if it lives in the scrollable
// page content) is visible in the current viewport, nudging g_scrollY
// if not. Content-space rects here store the same "+g_scrollY baked
// in" absolute position every other content hit-rect in this file
// uses, so projecting back to a screen position just subtracts the
// CURRENT g_scrollY — valid even though the rect itself was captured
// on a previous paint, since layout doesn't change from scrolling
// alone. Definition of the forward declaration up near the rest of
// the focus system state.
static void scrollFocusedIntoView() {
    FocusableItem* item = focusableAt(g_focusedIndex);
    if (!item || !item->inContent) return;

    int top = item->rect.top - g_scrollY;
    int bottom = item->rect.bottom - g_scrollY;
    int margin = 12;
    if (top < margin) {
        g_scrollY -= (margin - top);
    } else if (bottom > g_viewportH - margin) {
        g_scrollY += (bottom - (g_viewportH - margin));
    }
    if (g_scrollY < 0) g_scrollY = 0;
    if (g_hContent) updateScrollbar(g_hContent);
}

// Converts a real child HWND's rect into its parent's client
// coordinates — used to build FocusableItems for the sidebar's native
// controls, whose positions come from MoveWindow calls elsewhere and
// aren't otherwise tracked in a rect vector the way canvas items are.
RECT childRectInParent(HWND child, HWND parent) {
    RECT r = {};
    if (!child || !IsWindowVisible(child)) return r;
    GetWindowRect(child, &r);
    MapWindowPoints(HWND_DESKTOP, parent, (POINT*)&r, 2);
    return r;
}

// ── Rendering helpers ────────────────────────────────────────

// Draw a rounded card (Concept 3 style)
// GDI+ helper: build a rounded-rect GraphicsPath
GraphicsPath* makeRoundRectPath(float x, float y, float w, float h, float r) {
    GraphicsPath* path = new GraphicsPath();
    float d = r * 2;
    path->AddArc(x, y, d, d, 180, 90);
    path->AddArc(x + w - d, y, d, d, 270, 90);
    path->AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path->AddArc(x, y + h - d, d, d, 90, 90);
    path->CloseFigure();
    return path;
}

void drawCard(HDC hdc, const RECT& r, COLORREF bg, COLORREF border,
                      int radius, COLORREF accentLeft)
{
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    float x = (float)r.left, y = (float)r.top;
    float w = (float)(r.right - r.left), h = (float)(r.bottom - r.top);
    float rad = (float)radius;

    // Soft shadow — several offset translucent rounded rects
    for (int i = 4; i >= 1; --i) {
        GraphicsPath* shadowPath = makeRoundRectPath(
            x + i * 0.6f, y + i * 0.8f, w, h, rad);
        SolidBrush shadowBrush(Color(18, 0, 0, 0)); // very subtle alpha
        g.FillPath(&shadowBrush, shadowPath);
        delete shadowPath;
    }

    // Card body — anti-aliased rounded rect
    GraphicsPath* cardPath = makeRoundRectPath(x, y, w, h, rad);
    SolidBrush cardBrush(Color(255, GetRValue(bg), GetGValue(bg), GetBValue(bg)));
    g.FillPath(&cardBrush, cardPath);

    Pen borderPen(Color(255, GetRValue(border), GetGValue(border), GetBValue(border)), 1.0f);
    g.DrawPath(&borderPen, cardPath);
    delete cardPath;

    // Left accent bar (optional) — rounded on the left side only
    if (accentLeft != 0) {
        GraphicsPath* accentPath = makeRoundRectPath(x, y + 3, 4, h - 6, 2.0f);
        SolidBrush accentBrush(Color(255, GetRValue(accentLeft),
                                     GetGValue(accentLeft), GetBValue(accentLeft)));
        g.FillPath(&accentBrush, accentPath);
        delete accentPath;
    }
}

static COLORREF toastAccentColor(ToastKind k) {
    switch (k) {
        case TOAST_SUCCESS: return COLOR_SUCCESS;
        case TOAST_WARNING: return COLOR_WARNING;
        case TOAST_ERROR:   return COLOR_ERROR;
        default:            return INFO_;
    }
}

// Toast stack — bottom-right corner, newest nearest the edge.
// Non-blocking; each auto-dismisses after TOAST_DURATION_MS or on
// click. Drawn on hdcScreen (real screen DC, not the buffered content
// DC) so it stays fully opaque regardless of any page-fade animation
// in progress underneath it — same pattern as the DNA hover tooltip.
static void drawToasts(HDC hdcScreen, const RECT& viewport) {
    g_toastRects.clear();
    if (g_toasts.empty()) return;

    SetBkMode(hdcScreen, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(hdcScreen, g_hFontBodyNew);

    int toastW = S(320);
    int gap = S(10);
    int cy = viewport.bottom - S(20);

    for (int i = (int)g_toasts.size() - 1; i >= 0; --i) {
        const auto& t = g_toasts[i];

        RECT measureR = {0, 0, toastW - S(60), 0};
        DrawTextW(hdcScreen, t.text.c_str(), -1, &measureR,
                  DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
        int textH = measureR.bottom - measureR.top;
        int toastH = textH + S(24);
        if (toastH < S(48)) toastH = S(48);

        cy -= toastH;
        RECT toastR = {viewport.right - S(20) - toastW, cy,
                        viewport.right - S(20), cy + toastH};

        drawCard(hdcScreen, toastR, GRAY_850, GRAY_700, 10, toastAccentColor(t.kind));

        SetTextColor(hdcScreen, WHITE_);
        RECT textR = {toastR.left + S(18), toastR.top + S(12),
                       toastR.right - S(14), toastR.bottom - S(12)};
        DrawTextW(hdcScreen, t.text.c_str(), -1, &textR, DT_LEFT | DT_WORDBREAK);

        ToastRect hit;
        hit.r = toastR;
        hit.index = i;
        g_toastRects.push_back(hit);

        cy -= gap;
    }

    SelectObject(hdcScreen, oldFont);
}

// Blocking modal overlay — scrim + centered card. For messages that
// need an actual decision (MODAL_CONFIRM) or longer read-only content
// (MODAL_OK: About, Quick Start Guide). Same hdcScreen/after-blit
// pattern as drawToasts above.
static void drawAppModal(HDC hdcScreen, const RECT& viewport) {
    if (g_appModal.kind == MODAL_NONE) return;

    {
        Graphics g(hdcScreen);
        g.SetSmoothingMode(SmoothingModeNone);
        SolidBrush scrim(Color(170, 10, 10, 12));
        g.FillRectangle(&scrim, 0, 0, viewport.right, viewport.bottom);
    }

    SetBkMode(hdcScreen, TRANSPARENT);
    int modalW = S(480);
    int pad = S(28);
    int titleH = S(30);
    int btnH = S(40);

    HFONT oldFont = (HFONT)SelectObject(hdcScreen, g_hFontBodyNew);
    RECT measureR = {0, 0, modalW - pad * 2, 0};
    DrawTextW(hdcScreen, g_appModal.body.c_str(), -1, &measureR,
              DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
    int bodyH = measureR.bottom - measureR.top;

    int modalH = pad + titleH + S(12) + bodyH + S(20) + btnH + pad;
    int maxH = viewport.bottom - S(80);
    if (modalH > maxH) modalH = maxH;

    int mx = (viewport.right - modalW) / 2;
    int my = (viewport.bottom - modalH) / 2;
    if (my < S(20)) my = S(20);

    RECT modalR = {mx, my, mx + modalW, my + modalH};
    drawCard(hdcScreen, modalR, GRAY_900, GRAY_700, 14, g_appModal.accent);

    HFONT titleFont = (HFONT)SelectObject(hdcScreen, g_hFontH2);
    SetTextColor(hdcScreen, g_appModal.accent);
    RECT titleR = {mx + pad, my + pad, mx + modalW - pad, my + pad + titleH};
    DrawTextW(hdcScreen, g_appModal.title.c_str(), -1, &titleR,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdcScreen, titleFont);

    SetTextColor(hdcScreen, GRAY_200);
    RECT bodyR = {mx + pad, my + pad + titleH + S(12),
                  mx + modalW - pad, my + modalH - pad - btnH - S(16)};
    DrawTextW(hdcScreen, g_appModal.body.c_str(), -1, &bodyR, DT_LEFT | DT_WORDBREAK);

    int btnW = S(130);
    int btnY = my + modalH - pad - btnH;
    int btnX = mx + modalW - pad - btnW;

    RECT primaryR = {btnX, btnY, btnX + btnW, btnY + btnH};
    drawCard(hdcScreen, primaryR, g_appModal.accent, g_appModal.accent, 8);
    SetTextColor(hdcScreen, GRAY_950);
    DrawTextW(hdcScreen, g_appModal.primaryLabel.c_str(), -1, &primaryR,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    g_modalPrimaryBtnRect = primaryR;

    if (g_appModal.kind == MODAL_CONFIRM && !g_appModal.secondaryLabel.empty()) {
        int secX = btnX - S(12) - btnW;
        RECT secondaryR = {secX, btnY, secX + btnW, btnY + btnH};
        drawCard(hdcScreen, secondaryR, GRAY_800, GRAY_700, 8);
        SetTextColor(hdcScreen, GRAY_200);
        DrawTextW(hdcScreen, g_appModal.secondaryLabel.c_str(), -1, &secondaryR,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        g_modalSecondaryBtnRect = secondaryR;
    } else {
        g_modalSecondaryBtnRect = RECT{0, 0, 0, 0};
    }

    // Keyboard focus ring (item 2) — modal buttons get the same 2px
    // gold-400 outline as everything else. Tab toggles which one is
    // focused (main message loop); only shown after keyboard nav.
    if (g_focusVisible) {
        bool secondaryExists = (g_appModal.kind == MODAL_CONFIRM &&
                                 !g_appModal.secondaryLabel.empty());
        bool onPrimary = g_modalFocusOnPrimary || !secondaryExists;
        drawFocusRing(hdcScreen, onPrimary ? g_modalPrimaryBtnRect : g_modalSecondaryBtnRect);
    }

    SelectObject(hdcScreen, oldFont);
}


// ═════════════════════════════════════════════════════════════
// STYLE DNA STRIP — signature component
// ═════════════════════════════════════════════════════════════
// The thesis is about fingerprinting. This makes that literal.
//
// 14 cells, one per stylometric feature, in three groups:
//   LEXICAL (5) | LAYOUT (5) | SYNTACTIC (4)
// separated by a 6px gap.
//
// CRITICAL: a cell's value is encoded as bar HEIGHT, never color.
// Color is reserved exclusively for deviation. This means the strip
// stays fully legible to a colorblind reader, and it satisfies the
// accessibility floor (§9) — match/mismatch is carried by shape and
// position, not hue alone.
//
// Bars are bottom-anchored so the top edge forms a readable
// silhouette — that silhouette IS the fingerprint. Two students with
// different habits produce visibly different skylines in ~200ms,
// with no numbers read.
//
// Three render scales, one function:
//   · student card   — 4px cells, gray-400, no labels (barcode)
//   · detail page    — full width, labeled
//   · flagged pair   — two stacked, deviating cells lit RISK_HIGH

// Group boundaries within the DISPLAY order (5 lexical, 5 layout,
// 4 syntactic) — this is purely how many cells belong to each visual
// group, not the raw feature-vector index order (see below).
static const int DNA_GROUP_SIZES[3] = {5, 5, 4}; // lexical, layout, syntactic

// CRITICAL: the raw 14-value feature vector (profileVector /
// StyleNoteDisplay ordering) is NOT stored in sequential group order.
// This is proven by features.cpp's own comparison logic (the `checks`
// array in generateStyleComparisonNotes()): index 13 (ALL_CAPS) is
// lexical, index 4 (comment frequency) is layout, index 9 (line
// length) is layout, etc. — not a naive 0-4/5-9/10-13 split.
//
// DNA_DISPLAY_ORDER maps "position in the strip" -> "actual index in
// the raw feature vector", so the strip visually groups by TRUE
// semantic category. Drawing values[idx] directly (position == raw
// index) was the bug — it silently mislabeled every strip rendered
// before this fix.
static const int DNA_DISPLAY_ORDER[14] = {
    0, 1, 2, 3, 13,   // LEXICAL: var name length, single-letter, camelCase, snake_case, ALL_CAPS
    4, 5, 6, 9, 10,   // LAYOUT: comment freq, inline comment, bracket, line length, blank line
    7, 8, 11, 12      // SYNTACTIC: loop pref, operator spacing, function length, return placement
};

// Human-readable names in the SAME display order as above, for hover
// tooltips. Kept in sync manually with DNA_DISPLAY_ORDER's raw-index
// mapping and features.cpp's canonical names.
static const wchar_t* DNA_DISPLAY_NAMES[14] = {
    L"Variable name length", L"Single-letter variables", L"camelCase naming",
    L"snake_case naming", L"ALL_CAPS variables",
    L"Comment frequency", L"Inline comment style", L"Bracket placement",
    L"Line length", L"Blank line usage",
    L"Loop preference", L"Operator spacing", L"Function length", L"Return placement"
};

DnaStripStyle dnaStyleCard() {
    return { S(4), S(1), S(6), S(18), GRAY_400, RISK_HIGH };
}
// dnaStyleDetail() moved to gui_students.cpp (Checkpoint 3 of the
// ContentProc split) — exclusively used by drawStudentDetailPage()
// there.
// dnaStylePair() moved to gui_flagged.cpp (Checkpoint 2 of the
// ContentProc split) — exclusively used by drawFlaggedPairCollapsedRow().

// Total pixel width a strip will occupy — needed for right-alignment
// and hit-testing before the strip is actually drawn.
int dnaStripWidth(const DnaStripStyle& st) {
    int w = 0;
    for (int g = 0; g < 3; ++g) {
        int n = DNA_GROUP_SIZES[g];
        w += n * st.cellW + (n - 1) * st.cellGap;
        if (g < 2) w += st.groupGap;
    }
    return w;
}

// ── Hover tooltip support ─────────────────────────────────────
// Spec §4.4 explicitly calls the detail-page strip "interactive on
// hover" — this was built without that interactivity until now. Each
// drawDnaStrip() call records its 14 cell hit-rects (+ the value and
// name behind each) into this single shared buffer. Only one strip is
// normally visible/relevant to the mouse at a time (student card
// under the cursor, or the one detail-page strip), so a single
// shared buffer — overwritten each time a strip is drawn — is
// sufficient rather than needing a per-instance registry.
std::vector<DnaCellHit> g_dnaCellHits;
int g_dnaHoveredCell = -1; // index into g_dnaCellHits, -1 = none
// Not static as of Checkpoint 3 of the ContentProc split: also used
// by handleStudentsDetailMouseMove()/drawStudentDnaTooltip() in
// gui_students.cpp.
POINT g_dnaTooltipPos = {};

// values   : 14 normalized (0..1) feature values.
// deviant  : optional 14 flags; true = this feature deviates. nullptr
//            for none.
// valuesAreRawIndexed: true (default) means `values`/`deviant` are in
//            RAW feature-vector order (index 0..13 as profileVector
//            stores them) and need DNA_DISPLAY_ORDER remapping — this
//            is the case for student profiles. Pass false when the
//            caller's data is already pre-grouped into display order
//            (lexical→layout→syntactic sequentially) — this is the
//            case for flagged-pair comparisons via dnaValuesFromPair()/
//            dnaDeviationFlags(), whose source (StyleNoteDisplay) has
//            no raw index field to remap from.
// trackHits: if true, this call's cells are recorded for hover
//            tooltips (pass false for tiny/decorative uses where a
//            tooltip wouldn't be useful, e.g. deep in a dense list).
void drawDnaStrip(HDC hdc, int x, int y,
                           const std::vector<double>& values,
                           const std::vector<bool>* deviant,
                           const DnaStripStyle& st,
                           bool trackHits,
                           bool valuesAreRawIndexed)
{
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeNone); // crisp 1px edges on tiny cells

    // Track backdrop — a faint baseline so empty/low cells still read
    // as "measured but low" rather than "missing data".
    int totalW = dnaStripWidth(st);
    SolidBrush trackBrush(Color(255, GetRValue(GRAY_800),
                                 GetGValue(GRAY_800), GetBValue(GRAY_800)));
    g.FillRectangle(&trackBrush, x, y, totalW, st.height);

    if (trackHits) g_dnaCellHits.clear();

    int pos = 0; // position within the display strip (0..13)
    int cx  = x;

    for (int grp = 0; grp < 3; ++grp) {
        for (int i = 0; i < DNA_GROUP_SIZES[grp]; ++i, ++pos) {
            int lookupIdx = valuesAreRawIndexed ? DNA_DISPLAY_ORDER[pos] : pos;
            double v = (lookupIdx < (int)values.size()) ? values[lookupIdx] : 0.0;
            if (v < 0.0) v = 0.0;
            if (v > 1.0) v = 1.0;

            int barH = (int)(v * st.height);
            if (barH < S(2)) barH = S(2);          // floor so zero is visible
            int barY = y + (st.height - barH);   // bottom-anchored

            bool isDeviant = deviant && lookupIdx < (int)deviant->size() && (*deviant)[lookupIdx];
            COLORREF c = isDeviant ? st.deviantColor : st.baseColor;

            SolidBrush b(Color(255, GetRValue(c), GetGValue(c), GetBValue(c)));
            g.FillRectangle(&b, cx, barY, st.cellW, barH);

            // Deviant cells get a full-height ghost behind them so the
            // flagged position is findable even when its value is low —
            // position must carry information independently of height.
            if (isDeviant) {
                SolidBrush ghost(Color(60, GetRValue(c), GetGValue(c), GetBValue(c)));
                g.FillRectangle(&ghost, cx, y, st.cellW, st.height);
                SolidBrush solid(Color(255, GetRValue(c), GetGValue(c), GetBValue(c)));
                g.FillRectangle(&solid, cx, barY, st.cellW, barH);
            }

            if (trackHits) {
                DnaCellHit hit;
                hit.r = { cx, y, cx + st.cellW, y + st.height };
                hit.name = DNA_DISPLAY_NAMES[pos];
                hit.value = v;
                g_dnaCellHits.push_back(hit);
            }

            cx += st.cellW + ((i < DNA_GROUP_SIZES[grp] - 1) ? st.cellGap : 0);
        }
        if (grp < 2) cx += st.groupGap;
    }
}

// dnaDeviationFlags() and dnaValuesFromPair() moved to gui_flagged.cpp
// (Checkpoint 2 of the ContentProc split) — exclusively used by
// drawFlaggedPairCollapsedRow()/drawAuthorshipCard() there.

// ── Gold eyebrow label ────────────────────────────────────────
// The connective tissue of the design: 11px uppercase gold with
// +tracking, and a 1px hairline running from the end of the text to
// the container edge. Used for EVERY section head, nowhere else.
int drawEyebrow(HDC hdc, int x, int y, int width, const wchar_t* text) {
    SetBkMode(hdc, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontLabel);
    SetTextColor(hdc, GOLD_500);

    // Manual letter-spacing: GDI has no tracking property, so draw
    // character by character with an added advance.
    int cx = x;
    const int tracking = S(1); // ~+0.10em at 11px
    int rowH = S(16);
    for (const wchar_t* p = text; *p; ++p) {
        wchar_t ch[2] = { *p, 0 };
        SIZE sz;
        GetTextExtentPoint32W(hdc, ch, 1, &sz);
        RECT r = { cx, y, cx + sz.cx + tracking, y + rowH };
        DrawTextW(hdc, ch, 1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE);
        cx += sz.cx + tracking;
    }

    // Hairline from end of text to container edge
    int lineY = y + S(8);
    int lineStart = cx + S(10);
    int lineEnd   = x + width;
    if (lineEnd > lineStart) {
        HPEN pen = CreatePen(PS_SOLID, 1, GRAY_700);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, lineStart, lineY, nullptr);
        LineTo(hdc, lineEnd, lineY);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    SelectObject(hdc, oldFont);
    return rowH; // height consumed — callers add this to their own cy,
                 // so it must match what was actually drawn above.
}

// ── Middle truncation ─────────────────────────────────────────
// Filenames carry their identity in the TAIL (the ASGN fingerprint
// and student name), so end-ellipsis destroys exactly the part that
// disambiguates. Truncate the middle instead.
std::wstring truncateMiddle(HDC hdc, const std::wstring& text, int maxWidth) {
    SIZE sz;
    GetTextExtentPoint32W(hdc, text.c_str(), (int)text.length(), &sz);
    if (sz.cx <= maxWidth) return text;

    std::wstring ellipsis = L"\u2026";
    size_t lo = 0, hi = text.length() / 2;
    std::wstring best = ellipsis;

    while (lo <= hi) {
        size_t keep = (lo + hi) / 2;
        if (keep == 0) break;
        std::wstring candidate = text.substr(0, keep) + ellipsis +
                                 text.substr(text.length() - keep);
        GetTextExtentPoint32W(hdc, candidate.c_str(), (int)candidate.length(), &sz);
        if (sz.cx <= maxWidth) { best = candidate; lo = keep + 1; }
        else                   { hi = keep - 1; }
    }
    return best;
}

// drawProgressBar() and scoreColor() moved to gui_flagged.cpp
// (Checkpoint 2 of the ContentProc split) — exclusively used by
// drawAuthorshipCard() there. Despite the comment near SidebarProc's
// forward declarations mentioning drawProgressBar as one of "these
// primitives... defined later in the file", SidebarProc does not
// actually call it — verified with a call-site grep before moving.

// scoreColor() above (moved) is signed for AUTHORSHIP LIKELIHOOD (high = matches
// own profile = good = green). Pair SIMILARITY between two different
// students is the opposite: high = concerning. Spec §4.1 thresholds:
// risk-high >=85%, risk-mod 70-84%, risk-low <70%. Using scoreColor()
// on combinedScore was backwards (high similarity showing green) —
// this is the correctly-signed version for that specific value.
COLORREF pairSeverityColor(double combinedScorePct) {
    if (combinedScorePct >= 85.0) return RISK_HIGH;
    if (combinedScorePct >= 70.0) return RISK_MOD;
    return RISK_LOW;
}
const wchar_t* pairSeverityLabel(double combinedScorePct) {
    if (combinedScorePct >= 85.0) return L"High";
    if (combinedScorePct >= 70.0) return L"Moderate";
    return L"Low";
}

// ── Section drawers (return height used) ─────────────────────
// drawResultsHeader moved to gui_overview.cpp (Checkpoint 1 of the
// ContentProc split) — it's Overview-page-only, alongside its two
// siblings drawStatsDashboard/drawAISummary which were already there.

// Draw the stats dashboard (6 cards in a grid)
// Lightweight local mirror of main.cpp's extractStudentName() (which
// is static and not exported) — good enough for display purposes in
// CSV export. Strips the "{period}{type}__" prefix and "_ASGN..."
// fingerprint suffix that Classroom-synced/assigned files carry.
std::string extractStudentNameGC(const std::string& filename) {
    std::string name = filename;
    size_t dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot);
    size_t delim = name.rfind("__");
    if (delim != std::string::npos) name = name.substr(delim + 2);
    size_t fp = name.rfind("_ASGN");
    if (fp != std::string::npos) name = name.substr(0, fp);
    return name.empty() ? filename : name;
}


// Switches the active report tab. Shared by the tab-bar mouse click
// handler and activateFocused() (item 2) — identical to what the
// click did inline before this refactor.
static void switchToTab(HWND hwnd, ResultsPage page) {
    if (g_currentPage == page) return;
    g_currentPage = page;
    g_selectedStudent = -1;
    g_currentBlockTab = -1;
    g_scrollY = 0;
    g_contentFadeAlpha = 60;
    SetTimer(hwnd, TIMER_FADE, 15, nullptr);
    SetTimer(hwnd, TIMER_TAB_UNDERLINE, 12, nullptr);
    InvalidateRect(hwnd, nullptr, FALSE);
}

// Switches to Flagged Pairs, clears both filters, and expands the
// given pair. Shared by the AI-summary deep-link, the Flagged-
// Appearances row click, and activateFocused() — was duplicated
// between the first two before this refactor.
void jumpToFlaggedPair(HWND hwnd, int pairIndex) {
    g_currentPage = PAGE_FLAGGED;
    g_filterExactDuplicatesOnly = false;
    g_pairSeverityFilter = SEV_FILTER_ALL;
    if (pairIndex >= 0) g_expandedPairs.insert(pairIndex);
    g_selectedStudent = -1;
    g_scrollY = 0;
    g_contentFadeAlpha = 60;
    SetTimer(hwnd, TIMER_FADE, 15, nullptr);
    SetTimer(hwnd, TIMER_TAB_UNDERLINE, 12, nullptr);
    InvalidateRect(hwnd, nullptr, FALSE);
}

// ── Classes screen shared handlers (item 2) ───────────────────────
void selectBlockTab(HWND hwnd, int tabIdx) {
    if (g_currentBlockTab == tabIdx) return;
    g_currentBlockTab = tabIdx;
    g_scrollY = 0;
    InvalidateRect(hwnd, nullptr, FALSE);
}

void openStudentDetail(HWND hwnd, int studentIdx) {
    g_selectedStudent = studentIdx;
    g_scrollY = 0;
    InvalidateRect(hwnd, nullptr, FALSE);
}

void closeStudentDetail(HWND hwnd) {
    g_selectedStudent = -1;
    g_scrollY = 0;
    InvalidateRect(hwnd, nullptr, FALSE);
}

// Generates and opens the currently-selected student's similarity
// report. Shared by the "Generate Similarity Report" mouse click and
// activateFocused().
void generateAndOpenStudentReport() {
    if (g_selectedStudent < 0 ||
        g_selectedStudent >= (int)g_analysisResults.profiles.size())
        return;

    const std::string& name = g_analysisResults.profiles[g_selectedStudent].name;
    std::string path = generateStudentSimilarityReport(name);
    if (!path.empty()) {
        std::string hash = audit_sha256File(path);
        audit_log("STUDENT REPORT GENERATED",
            "Student: " + name + "\n" +
            "Report file: " + path + "\n" +
            "Report: " + hash);
        ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    } else {
        showToast(L"Failed to generate the similarity report. "
                   L"Please make sure the analysis has been run.", TOAST_WARNING);
    }
}

// Toggles the search box open/closed. Shared by the search-icon mouse
// click and activateFocused() (item 2).
static void toggleSearchBox(HWND hwnd) {
    g_searchActive = !g_searchActive;
    if (g_searchActive && g_hSearchEdit) {
        // Position just left of the search icon, vertically centered
        // in the tab row.
        int editW = S(200), editH = S(26);
        int editX = g_searchBtnRect.left - editW - S(8);
        int editY = S(4) + (S(TAB_BAR_HEIGHT) - editH) / 2;
        MoveWindow(g_hSearchEdit, editX, editY, editW, editH, TRUE);
        ShowWindow(g_hSearchEdit, SW_SHOW);
        SetFocus(g_hSearchEdit);
    } else if (g_hSearchEdit) {
        ShowWindow(g_hSearchEdit, SW_HIDE);
        SetWindowTextW(g_hSearchEdit, L"");
        g_searchFilterLower.clear();
    }
    InvalidateRect(hwnd, nullptr, FALSE);
}

// Puts UTF-16 text on the clipboard. Standard Win32 clipboard
// sequence — OpenClipboard/EmptyClipboard/GlobalAlloc(GMEM_MOVEABLE)/
// GlobalLock+memcpy/GlobalUnlock/SetClipboardData(CF_UNICODETEXT).
// Returns false (silently) on any failure; callers can beep or leave
// the button label unchanged rather than raising a MessageBox for
// what is a best-effort convenience action.
static bool copyTextToClipboardW(HWND hwnd, const std::wstring& text) {
    if (!OpenClipboard(hwnd)) return false;
    EmptyClipboard();
    size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    bool ok = false;
    if (hMem) {
        void* dst = GlobalLock(hMem);
        if (dst) {
            memcpy(dst, text.c_str(), bytes);
            GlobalUnlock(hMem);
            if (SetClipboardData(CF_UNICODETEXT, hMem)) ok = true;
        }
        if (!ok) GlobalFree(hMem); // ownership only transfers on success
    }
    CloseClipboard();
    return ok;
}

// Plain-text findings summary for one flagged pair (spec §5.9: "a
// plain-text summary of that pair on the clipboard for pasting into
// an email or an incident form").
static std::wstring buildFindingsText(const PairAnalysisDisplay& pair, int pairNumber) {
    std::wostringstream os;
    os << L"Flagged Pair #" << pairNumber << L" \u2014 " << s2w(pair.similarityLabel) << L"\r\n";
    os << s2w(pair.studentA.studentName) << L" (" << s2w(pair.studentA.filename) << L")"
       << L"  vs  "
       << s2w(pair.studentB.studentName) << L" (" << s2w(pair.studentB.filename) << L")"
       << L"\r\n\r\n";
    os << L"Combined similarity: " << fmtPct(pair.combinedScore)
       << L"  (Token: " << fmtPct(pair.tokenScore)
       << L" \u00b7 Style: " << fmtPct(pair.styleScore) << L")\r\n\r\n";

    auto sideLine = [&os](const AuthorshipDisplay& s) {
        os << s2w(s.studentName) << L": " << fmtPct(s.scorePct) << L" \u00b7 "
           << s2w(s.label) << L", profile from " << s.profileSize << L" submission"
           << (s.profileSize != 1 ? L"s" : L"") << L", " << s.matchedCount << L" of "
           << s.totalFeatures << L" features matched\r\n";
    };
    sideLine(pair.studentA);
    sideLine(pair.studentB);

    os << L"\r\n" << s2w(pair.interpretation) << L"\r\n\r\n";
    os << L"Generated by CALSS \u2014 statistical evidence only; academic integrity "
          L"determinations rest with the instructor.";
    return os.str();
}

// ── Flagged Pairs screen shared handlers (item 2) ─────────────────
// togglePairExpanded() already existed as a shared function (used by
// both the collapsed-row click and the expanded-header click before
// this pass) — reused as-is below. The four here are new extractions
// of logic that was previously only reachable from the mouse handler.

void toggleShowAllFeatures(HWND hwnd, int key) {
    if (g_showAllFeaturesKeys.count(key)) g_showAllFeaturesKeys.erase(key);
    else g_showAllFeaturesKeys.insert(key);
    InvalidateRect(hwnd, nullptr, FALSE);
}

// Copies one pair's findings to the clipboard and starts the brief
// "Copied" feedback window on its button.
void copyFindingsForPair(HWND hwnd, int pairIdx) {
    if (pairIdx < 0 || pairIdx >= (int)g_analysisResults.flaggedPairs.size())
        return;

    const auto& pr = g_analysisResults.flaggedPairs[pairIdx];
    // pairNumber shown in the copied text: recompute the on-screen
    // sequential number this pair currently holds under the active
    // filters, rather than its raw flaggedPairs position.
    int seq = 0;
    for (size_t k = 0; k <= (size_t)pairIdx; ++k) {
        const auto& p2 = g_analysisResults.flaggedPairs[k];
        if (g_filterExactDuplicatesOnly && p2.similarityLabel != "Exact Duplicate")
            continue;
        if (!pairMatchesSeverityFilter(p2.combinedScore, g_pairSeverityFilter))
            continue;
        ++seq;
    }
    copyTextToClipboardW(hwnd, buildFindingsText(pr, seq));
    g_copyFindingsFeedbackPairIdx = pairIdx;
    g_copyFindingsFeedbackUntilTick = GetTickCount() + COPY_FEEDBACK_MS;
    SetTimer(hwnd, TIMER_COPY_FEEDBACK, COPY_FEEDBACK_MS + 50, nullptr);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void selectSeverityFilter(HWND hwnd, PairSeverityFilter f) {
    if (g_pairSeverityFilter == f) return;
    g_pairSeverityFilter = f;
    g_scrollY = 0;
    InvalidateRect(hwnd, nullptr, FALSE);
}

void clearExactDuplicatesFilter(HWND hwnd) {
    g_filterExactDuplicatesOnly = false;
    g_scrollY = 0;
    InvalidateRect(hwnd, nullptr, FALSE);
}

// Lightweight local mirror of main.cpp's extractPeriod() (also static,
// not exported) — same reasoning as extractStudentNameGC above: good
// enough for display purposes, this time for the Flagged Pairs
// collapsed-row period label (spec §5.9 mockup: "Final Exam").
std::string extractPeriodGC(const std::string& filename) {
    std::string name = filename;
    size_t dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot);
    size_t delimPos = name.find("__");
    if (delimPos == std::string::npos) return "general";
    std::string tag = name.substr(0, delimPos);
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
    if (tag.find("semifinal") != std::string::npos) return "semifinal";
    if (tag.find("midterm")   != std::string::npos) return "midterm";
    if (tag.find("final")     != std::string::npos) return "final";
    if (tag.find("prelim")    != std::string::npos) return "prelim";
    return "general";
}

// Display label for the period bucket above — "Final Exam" per the
// spec's collapsed-row mockup, not just the raw bucket name.
std::wstring periodDisplayLabelGC(const std::string& period) {
    if (period == "semifinal") return L"Semifinal Exam";
    if (period == "midterm")   return L"Midterm Exam";
    if (period == "final")     return L"Final Exam";
    if (period == "prelim")    return L"Prelim Exam";
    return L"General";
}

// Exports all analyzed pairs to CSV (spec §5.6 Export ▾, §3 item #10:
// "a professor cannot leave the room with this"). CSV is the concrete
// deliverable here — it opens cleanly in Excel/Sheets for further
// sorting/filtering, which is arguably more useful to an instructor
// than a static PDF. Full PDF/print rendering is a larger surface
// (a proper paginated layout engine) and is not part of this pass;
// the button is honest about that rather than faking it.
bool exportPairsToCsv(HWND hwnd) {
    OPENFILENAMEA ofn = {};
    char filePath[MAX_PATH] = "calss_flagged_pairs.csv";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = hwnd;
    ofn.lpstrFilter = "CSV files (*.csv)\0*.csv\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = filePath;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrDefExt = "csv";
    ofn.Flags       = OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameA(&ofn)) return false;

    std::ofstream f(filePath);
    if (!f.is_open()) return false;

    f << "Student A,Student B,Combined Score,Token Score,Style Score,"
         "Severity,Flagged\n";
    for (const auto& p : g_analysisResults.allPairs) {
        std::string sevLabel = p.combinedScore >= 85.0 ? "High"
                             : p.combinedScore >= 70.0 ? "Moderate" : "Low";
        auto csvEscape = [](std::string s) {
            std::string out;
            for (char c : s) { if (c == '"') out += '"'; out += c; }
            return "\"" + out + "\"";
        };
        f << csvEscape(extractStudentNameGC(p.filenameA)) << ","
          << csvEscape(extractStudentNameGC(p.filenameB)) << ","
          << std::fixed << std::setprecision(1) << p.combinedScore << ","
          << p.tokenScore << "," << p.styleScore << ","
          << sevLabel << "," << (p.flagged ? "Yes" : "No") << "\n";
    }

    // Compliance fix item 8: this is the one score-bearing artifact
    // that leaves the app entirely (forwarded, printed, attached to
    // an email) — the disclaimer matters here at least as much as
    // in-app, arguably more since the app's own framing won't travel
    // with it. Blank separator row keeps it visually distinct from
    // the data rows in Excel/Sheets rather than reading as one more
    // pair.
    f << "\n\"Statistical evidence only. All academic integrity "
         "determinations rest with the instructor.\"\n";

    f.close();
    return true;
}

// Forward declarations for stage 5B

// ── Tab bar drawing ───────────────────────────────────────────
// Returns height used (TAB_BAR_HEIGHT)
// Segmented tab bar (spec §5.6). 40px tall, gray-850 track, active
// segment gets a maroon-800 fill with a 2px gold-500 underline that
// SLIDES between segments in 200ms — not snaps. Right-aligned utility
// cluster: Export / Search / Help.
static int drawTabBar(HDC hdc, int x, int y, int width) {
    SetBkMode(hdc, TRANSPARENT);

    // TAB_BAR_HEIGHT is also referenced unscaled in WM_PAINT's layout
    // math and WM_LBUTTONDOWN's hit-testing (both scaled to match, see
    // those call sites) — cached once here for this function's own use.
    int tbH = S(TAB_BAR_HEIGHT);

    RECT barRect = {x, y, x + width, y + tbH};
    HBRUSH bgBr = CreateSolidBrush(GRAY_850);
    FillRect(hdc, &barRect, bgBr);
    DeleteObject(bgBr);

    HPEN sepPen = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldPen = (HPEN)SelectObject(hdc, sepPen);
    MoveToEx(hdc, x, y + tbH - 1, nullptr);
    LineTo(hdc, x + width, y + tbH - 1);
    SelectObject(hdc, oldPen);
    DeleteObject(sepPen);

    const wchar_t* labels[3] = { L"Overview", L"Classes", L"Flagged pairs" };
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBodyNew);

    // Segments sized to their text + padding, laid out left to right
    int segX = x + S(8);
    int segPad = S(20);
    RECT activeRect = {0,0,0,0};

    for (int i = 0; i < 3; ++i) {
        SIZE sz;
        GetTextExtentPoint32W(hdc, labels[i], (int)wcslen(labels[i]), &sz);
        int segW = sz.cx + segPad * 2;
        bool active = (g_currentPage == (ResultsPage)i);

        g_tabs[i].x = segX;
        g_tabs[i].y = y;
        g_tabs[i].w = segW;
        g_tabs[i].h = tbH;
        g_tabs[i].page = (ResultsPage)i;

        if (active) {
            RECT fillR = {segX, y + S(3), segX + segW, y + tbH - S(3)};
            HBRUSH fb = CreateSolidBrush(MAROON_800);
            FillRect(hdc, &fillR, fb);
            DeleteObject(fb);
            activeRect = RECT{segX, y, segX + segW, y + tbH};
        }

        SetTextColor(hdc, active ? WHITE_ : GRAY_400);
        RECT tr = {segX, y, segX + segW, y + tbH};
        DrawTextW(hdc, labels[i], -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        addFocusable(g_contentFocusables, FocusKind::Tab, tr, i, false);

        segX += segW + S(4);
    }

    // Target the underline to the active segment; a timer (wired at
    // the call site) interpolates g_tabUnderlineX/W toward this each
    // frame. First paint snaps instantly (no animation from nothing).
    g_tabUnderlineTargetX = (float)activeRect.left;
    g_tabUnderlineTargetW = (float)(activeRect.right - activeRect.left);
    if (!g_tabUnderlineInit) {
        g_tabUnderlineX = g_tabUnderlineTargetX;
        g_tabUnderlineW = g_tabUnderlineTargetW;
        g_tabUnderlineInit = true;
    }
    if (g_tabUnderlineW > 0) {
        RECT underlineR = {
            (int)g_tabUnderlineX, y + tbH - S(2),
            (int)(g_tabUnderlineX + g_tabUnderlineW), y + tbH
        };
        HBRUSH ub = CreateSolidBrush(GOLD_500);
        FillRect(hdc, &underlineR, ub);
        DeleteObject(ub);
    }

    // ── Utility cluster: Export / Search / Help, right-aligned ────
    SelectObject(hdc, g_hFontBodyNew);
    int rx = x + width - S(20);

    // Help (rightmost)
    SIZE helpSz; GetTextExtentPoint32W(hdc, L"Help", 4, &helpSz);
    RECT helpRect = {rx - helpSz.cx, y, rx, y + tbH};
    rx -= helpSz.cx + S(24);
    SetTextColor(hdc, GRAY_400);
    DrawTextW(hdc, L"Help", -1, &helpRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    g_helpLinkRect = helpRect;

    // Search icon (magnifying glass, drawn — no glyph font dependency)
    RECT searchR = {rx - S(28), y + S(8), rx, y + tbH - S(8)};
    g_searchBtnRect = searchR;
    {
        Graphics g(hdc);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        COLORREF sc = g_searchActive ? GOLD_500 : GRAY_400;
        Pen pen(Color(255, GetRValue(sc), GetGValue(sc), GetBValue(sc)), 1.6f);
        int cx = searchR.left + S(8), cy = (searchR.top + searchR.bottom) / 2 - 1;
        g.DrawEllipse(&pen, cx - S(6), cy - S(6), S(10), S(10));
        g.DrawLine(&pen, cx + S(2), cy + S(2), cx + S(8), cy + S(8));
    }
    rx -= S(28) + S(20);

    // Export ▾
    SIZE expSz; GetTextExtentPoint32W(hdc, L"Export \u25BE", 8, &expSz);
    RECT expRect = {rx - expSz.cx - S(16), y + S(6), rx, y + tbH - S(6)};
    g_exportBtnRect = expRect;
    drawCard(hdc, expRect, g_exportMenuOpen ? GRAY_700 : GRAY_800, GRAY_700, 6);
    SetTextColor(hdc, GRAY_200);
    DrawTextW(hdc, L"Export \u25BE", -1, &expRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Focusables for the utility cluster, added here (after all three
    // rects are known) in true left-to-right visual order — Export,
    // Search, Help — even though the code above computes them
    // right-to-left (each rect anchored off the previous one's rx).
    addFocusable(g_contentFocusables, FocusKind::ExportBtn, expRect, 0, false);
    addFocusable(g_contentFocusables, FocusKind::SearchBtn, searchR, 0, false);
    addFocusable(g_contentFocusables, FocusKind::HelpLink, helpRect, 0, false);

    SelectObject(hdc, oldFont);
    return tbH;
}

// Sticky context strip (spec §5.6) — drawn immediately below the tab
// row at a FIXED position, so it stays visible while content scrolls.
// "Generated {date} · {time} · {N} files · {M} students · {folder}"
static int drawContextStrip(HDC hdc, int x, int y, int width) {
    SetBkMode(hdc, TRANSPARENT);
    // CONTEXT_STRIP_HEIGHT is also referenced unscaled in WM_PAINT's
    // layout math and WM_LBUTTONDOWN's hit-testing (both scaled to
    // match, see those call sites).
    int csH = S(CONTEXT_STRIP_HEIGHT);
    RECT bar = {x, y, x + width, y + csH};
    HBRUSH bg = CreateSolidBrush(GRAY_900);
    FillRect(hdc, &bar, bg);
    DeleteObject(bg);

    HPEN sep = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldPen = (HPEN)SelectObject(hdc, sep);
    MoveToEx(hdc, x, y + csH - 1, nullptr);
    LineTo(hdc, x + width, y + csH - 1);
    SelectObject(hdc, oldPen);
    DeleteObject(sep);

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontMonoSm);
    SetTextColor(hdc, GRAY_400);

    const AnalysisResults& r = g_analysisResults;
    std::wstring line = L"Generated " + s2w(r.timestamp) +
        L"  \u00B7  " + std::to_wstring(g_state.fileCount) + L" files" +
        L"  \u00B7  " + std::to_wstring(r.profiles.size()) + L" students" +
        L"  \u00B7  " + truncateMiddle(hdc, g_state.dataFolder, S(260));

    RECT lr = {x + S(24), y, x + width - S(100), y + csH};
    DrawTextW(hdc, line.c_str(), -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SelectObject(hdc, g_hFontBodyNew);
    RECT rerunR = {x + width - S(90), y + S(4), x + width - S(20), y + csH - S(4)};
    g_rerunBtnRect = rerunR;
    SetTextColor(hdc, GOLD_500);
    DrawTextW(hdc, L"Re-run", -1, &rerunR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    addFocusable(g_contentFocusables, FocusKind::RerunBtn, rerunR, 0, false);

    SelectObject(hdc, oldFont);
    return csH;
}


// ── Main content area procedure ──────────────────────────────
static LRESULT CALLBACK ContentProc(HWND hwnd, UINT uMsg,
                                      WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_COMMAND: {
            // Live search filtering — fires on every keystroke in the
            // real EDIT control (spec §5.6/§5.8: "reuse the global
            // search to filter cards live").
            if (LOWORD(wParam) == ID_SEARCH_EDIT && HIWORD(wParam) == EN_CHANGE) {
                wchar_t buf[128] = {};
                GetWindowTextW(g_hSearchEdit, buf, 128);
                std::wstring lower = buf;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
                g_searchFilterLower = lower;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        // Esc closes search (clears filter); Ctrl+F opens it — the
        // exact keyboard path the accessibility floor (§9) requires.
        case WM_KEYDOWN: {
            // Modal gate (compliance fix item 5, updated for item 2) —
            // Esc always cancels/dismisses regardless of which button
            // has focus (standard convention). Enter activates
            // whichever button is currently focused — g_modalFocusOnPrimary,
            // toggled by Tab in the main message loop — rather than
            // always the primary one. Must come before every other
            // keyboard branch below, including search's own Esc.
            if (g_appModal.kind != MODAL_NONE) {
                if (wParam == VK_ESCAPE) {
                    dismissAppModal(false);
                    return 0;
                }
                if (wParam == VK_RETURN) {
                    bool secondaryExists = (g_appModal.kind == MODAL_CONFIRM &&
                                             !g_appModal.secondaryLabel.empty());
                    bool onPrimary = g_modalFocusOnPrimary || !secondaryExists;
                    dismissAppModal(onPrimary);
                    return 0;
                }
                return 0;
            }

            // Help carousel gate — same shape as the AppModal gate
            // above, mutually exclusive with it by construction.
            // Left/Right change step (spec §6), Esc closes, Enter
            // activates whichever of Skip/Next currently has the
            // keyboard focus ring (g_helpCarouselFocusOnNext, toggled
            // by Tab in the main message loop — same Tab-parity
            // pattern as g_modalFocusOnPrimary above).
            if (g_helpCarouselState == HelpCarouselState::Open) {
                if (wParam == VK_ESCAPE) {
                    closeHelpCarousel();
                    return 0;
                }
                if (wParam == VK_RIGHT) {
                    helpCarouselNext();
                    return 0;
                }
                if (wParam == VK_LEFT) {
                    helpCarouselPrev();
                    return 0;
                }
                if (wParam == VK_RETURN) {
                    bool skipExists = (g_helpCarouselSkipRect.right >
                                        g_helpCarouselSkipRect.left);
                    bool onNext = g_helpCarouselFocusOnNext || !skipExists;
                    if (onNext) helpCarouselNext();
                    else        helpCarouselSkip();
                    return 0;
                }
                return 0;
            }

            if (wParam == VK_ESCAPE && g_searchActive) {
                g_searchActive = false;
                if (g_hSearchEdit) {
                    ShowWindow(g_hSearchEdit, SW_HIDE);
                    SetWindowTextW(g_hSearchEdit, L"");
                }
                g_searchFilterLower.clear();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (wParam == 'F' && (GetKeyState(VK_CONTROL) & 0x8000) && !g_searchActive) {
                g_searchActive = true;
                if (g_hSearchEdit) {
                    int editW = S(200), editH = S(26);
                    int editX = g_searchBtnRect.left - editW - S(8);
                    int editY = S(4) + (S(TAB_BAR_HEIGHT) - editH) / 2;
                    if (editX < S(20)) editX = S(20); // fallback if rect not yet painted
                    MoveWindow(g_hSearchEdit, editX, editY, editW, editH, TRUE);
                    ShowWindow(g_hSearchEdit, SW_SHOW);
                    SetFocus(g_hSearchEdit);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            // Compliance fix item 2: Ctrl+E exports. Only meaningful
            // once a report exists; mirrors the Export button's own
            // CSV-only behavior (exportPairsToCsv) rather than opening
            // the Export ▾ menu, since there's nothing to choose
            // between yet (PDF/print are still explicitly deferred).
            if (wParam == 'E' && (GetKeyState(VK_CONTROL) & 0x8000) &&
                g_state.analysisComplete)
            {
                if (exportPairsToCsv(hwnd)) {
                    showToast(L"Exported all analyzed pairs to CSV.", TOAST_SUCCESS);
                }
                return 0;
            }
            // Compliance fix item 2: arrow keys move between report
            // tabs. Mirrors the tab-click handler's own logic exactly
            // (WM_LBUTTONDOWN, g_tabs[i] branch above) so keyboard and
            // mouse tab switches behave identically, including the
            // sliding underline animation.
            if ((wParam == VK_LEFT || wParam == VK_RIGHT) && g_state.analysisComplete &&
                !g_searchActive)
            {
                int dir = (wParam == VK_RIGHT) ? 1 : -1;
                int newPageIdx = (int)g_currentPage + dir;
                if (newPageIdx < 0) newPageIdx = 0;
                if (newPageIdx > 2) newPageIdx = 2;
                if (newPageIdx != (int)g_currentPage) {
                    g_currentPage = (ResultsPage)newPageIdx;
                    g_selectedStudent = -1;
                    g_currentBlockTab = -1;
                    g_scrollY = 0;
                    g_contentFadeAlpha = 60;
                    SetTimer(hwnd, TIMER_FADE, 15, nullptr);
                    SetTimer(hwnd, TIMER_TAB_UNDERLINE, 12, nullptr);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }

        case WM_VSCROLL: {
            int code = LOWORD(wParam);
            int newPos = g_scrollY;
            int lineH = 30;
            int pageH = g_viewportH;

            switch (code) {
                case SB_LINEUP:   newPos -= lineH; break;
                case SB_LINEDOWN: newPos += lineH; break;
                case SB_PAGEUP:   newPos -= pageH; break;
                case SB_PAGEDOWN: newPos += pageH; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION:
                    newPos = HIWORD(wParam);
                    break;
                case SB_TOP:      newPos = 0; break;
                case SB_BOTTOM:   newPos = g_contentH; break;
            }

            int maxScroll = g_contentH - g_viewportH;
            if (maxScroll < 0) maxScroll = 0;
            if (newPos < 0) newPos = 0;
            if (newPos > maxScroll) newPos = maxScroll;

            if (newPos != g_scrollY) {
                g_scrollY = newPos;
                updateScrollbar(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int scrollAmount = (delta / WHEEL_DELTA) * 80;
            int newPos = g_scrollY - scrollAmount;

            int maxScroll = g_contentH - g_viewportH;
            if (maxScroll < 0) maxScroll = 0;
            if (newPos < 0) newPos = 0;
            if (newPos > maxScroll) newPos = maxScroll;

            if (newPos != g_scrollY) {
                g_scrollY = newPos;
                updateScrollbar(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_SIZE: {
            RECT rect;
            GetClientRect(hwnd, &rect);
            g_viewportH = rect.bottom;
            updateScrollbar(hwnd);
            return 0;
        }

        case WM_TIMER: {
            if (wParam == TIMER_FADE) {
                g_contentFadeAlpha += 45;
                if (g_contentFadeAlpha >= 255) {
                    g_contentFadeAlpha = 255;
                    KillTimer(hwnd, TIMER_FADE);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else if (wParam == TIMER_TAB_UNDERLINE) {
                // Ease toward the target — not a snap. ~200ms feel via
                // a simple exponential approach, consistent with the
                // motion spec's standard-transition duration.
                float dx = g_tabUnderlineTargetX - g_tabUnderlineX;
                float dw = g_tabUnderlineTargetW - g_tabUnderlineW;
                if (fabsf(dx) < 0.5f && fabsf(dw) < 0.5f) {
                    g_tabUnderlineX = g_tabUnderlineTargetX;
                    g_tabUnderlineW = g_tabUnderlineTargetW;
                    KillTimer(hwnd, TIMER_TAB_UNDERLINE);
                } else {
                    g_tabUnderlineX += dx * 0.35f;
                    g_tabUnderlineW += dw * 0.35f;
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else if (wParam == TIMER_PAIR_EXPAND) {
                // Prune any pair whose 240ms open/close animation has
                // finished; keep ticking while at least one is in flight.
                for (auto it = g_pairAnimating.begin(); it != g_pairAnimating.end(); ) {
                    DWORD elapsed = GetTickCount() - it->second.startTick;
                    if (elapsed >= PAIR_EXPAND_ANIM_MS) it = g_pairAnimating.erase(it);
                    else ++it;
                }
                if (g_pairAnimating.empty()) KillTimer(hwnd, TIMER_PAIR_EXPAND);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else if (wParam == TIMER_COPY_FEEDBACK) {
                // One-shot: fires once the "Copied" feedback window has
                // elapsed, just to force the repaint that reverts the
                // button label back to "Copy findings".
                KillTimer(hwnd, TIMER_COPY_FEEDBACK);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else if (wParam == TIMER_TOAST) {
                DWORD now = GetTickCount();
                g_toasts.erase(
                    std::remove_if(g_toasts.begin(), g_toasts.end(),
                        [now](const AppToast& t) { return now >= t.showUntilTick; }),
                    g_toasts.end());
                if (g_toasts.empty()) KillTimer(hwnd, TIMER_TOAST);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            else if (wParam == TIMER_HELP_CAROUSEL) {
                // Unlike every other timer above, this one is NOT
                // self-terminating on a completion condition — it
                // keeps ticking for as long as the carousel is open,
                // since the current step's ring pulse (see
                // drawHelpCarouselRing) loops indefinitely by design.
                // openHelpCarousel()/closeHelpCarousel() are the only
                // places that Set/KillTimer this one. Just forces the
                // repaint that recomputes the sweep/pulse progress
                // from elapsed time each tick.
                if (g_helpCarouselState != HelpCarouselState::Open) {
                    KillTimer(hwnd, TIMER_HELP_CAROUSEL);
                } else {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_MOUSEMOVE: {
            // Style DNA strip hover (spec §4.4) — moved into
            // gui_students.cpp (Checkpoint 3 of the ContentProc
            // split). This handler is otherwise page-agnostic (see
            // the empty-state hover chunk right below, unrelated and
            // unmoved), so this Students-only chunk was easy to miss.
            if (g_currentPage == PAGE_STUDENTS) {
                handleStudentsDetailMouseMove(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }

            // Empty-state option card hover (spec §5.3: lift to
            // gray-800 with a gold-400 border, 120ms).
            if (!g_state.analysisComplete && !g_emptyCardRects.empty()) {
                int mx = GET_X_LPARAM(lParam);
                int my = GET_Y_LPARAM(lParam) + g_scrollY;
                int found = -1;
                for (size_t i = 0; i < g_emptyCardRects.size(); ++i) {
                    const auto& c = g_emptyCardRects[i];
                    if (mx >= c.x && mx < c.x + c.w &&
                        my >= c.y && my < c.y + c.h) { found = (int)i; break; }
                }
                if (found != g_hoveredEmptyCard) {
                    g_hoveredEmptyCard = found;
                    SetCursor(LoadCursor(nullptr, found >= 0 ? IDC_HAND : IDC_ARROW));
                    InvalidateRect(hwnd, nullptr, FALSE);
                    TRACKMOUSEEVENT tme = {};
                    tme.cbSize    = sizeof(tme);
                    tme.dwFlags   = TME_LEAVE;
                    tme.hwndTrack = hwnd;
                    TrackMouseEvent(&tme);
                }
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            if (g_dnaHoveredCell != -1) {
                g_dnaHoveredCell = -1;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            if (g_hoveredEmptyCard != -1) {
                g_hoveredEmptyCard = -1;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;

        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);

            // Modal gate (compliance fix item 5) — while an AppModal
            // is up, it owns all mouse input; everything else below is
            // unreachable until it's resolved. Must be the very first
            // check in this handler.
            if (g_appModal.kind != MODAL_NONE) {
                if (mx >= g_modalPrimaryBtnRect.left && mx < g_modalPrimaryBtnRect.right &&
                    my >= g_modalPrimaryBtnRect.top && my < g_modalPrimaryBtnRect.bottom)
                {
                    dismissAppModal(true);
                    return 0;
                }
                if (g_appModal.kind == MODAL_CONFIRM &&
                    mx >= g_modalSecondaryBtnRect.left && mx < g_modalSecondaryBtnRect.right &&
                    my >= g_modalSecondaryBtnRect.top && my < g_modalSecondaryBtnRect.bottom)
                {
                    dismissAppModal(false);
                    return 0;
                }
                return 0; // swallow clicks outside the modal's own buttons
            }

            // Help carousel gate — same shape as the AppModal gate
            // above, and mutually exclusive with it by construction
            // (openHelpCarousel() refuses to open over an active
            // AppModal). Must come before toast/page handling below,
            // same reasoning as the modal gate.
            if (g_helpCarouselState == HelpCarouselState::Open) {
                if (mx >= g_helpCarouselCloseRect.left && mx < g_helpCarouselCloseRect.right &&
                    my >= g_helpCarouselCloseRect.top && my < g_helpCarouselCloseRect.bottom)
                {
                    closeHelpCarousel();
                    return 0;
                }
                if (g_helpCarouselPrevRect.right > g_helpCarouselPrevRect.left &&
                    mx >= g_helpCarouselPrevRect.left && mx < g_helpCarouselPrevRect.right &&
                    my >= g_helpCarouselPrevRect.top && my < g_helpCarouselPrevRect.bottom)
                {
                    helpCarouselPrev();
                    return 0;
                }
                if (mx >= g_helpCarouselNextRect.left && mx < g_helpCarouselNextRect.right &&
                    my >= g_helpCarouselNextRect.top && my < g_helpCarouselNextRect.bottom)
                {
                    helpCarouselNext();
                    return 0;
                }
                if (g_helpCarouselSkipRect.right > g_helpCarouselSkipRect.left &&
                    mx >= g_helpCarouselSkipRect.left && mx < g_helpCarouselSkipRect.right &&
                    my >= g_helpCarouselSkipRect.top && my < g_helpCarouselSkipRect.bottom)
                {
                    helpCarouselSkip();
                    return 0;
                }
                for (int i = 0; i < HELP_STEP_COUNT; ++i) {
                    const RECT& d = g_helpCarouselDotRects[i];
                    if (mx >= d.left && mx < d.right && my >= d.top && my < d.bottom) {
                        helpCarouselGoToStep(i, i > g_helpCarouselStep ? 1 : -1);
                        return 0;
                    }
                }
                return 0; // swallow clicks outside the carousel's own controls
            }

            // Toast click-to-dismiss — checked before everything else
            // below since toasts float above all page content.
            if (!g_toastRects.empty()) {
                for (const auto& tr : g_toastRects) {
                    if (mx >= tr.r.left && mx < tr.r.right &&
                        my >= tr.r.top && my < tr.r.bottom)
                    {
                        if (tr.index >= 0 && tr.index < (int)g_toasts.size())
                            g_toasts.erase(g_toasts.begin() + tr.index);
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                }
            }

            // Tab click? (Tabs are at fixed Y at top, not scrolled)
            if (my >= S(4) && my < S(4) + S(TAB_BAR_HEIGHT)) {
                for (int i = 0; i < 3; ++i) {
                    if (mx >= g_tabs[i].x && mx < g_tabs[i].x + g_tabs[i].w &&
                        my >= g_tabs[i].y && my < g_tabs[i].y + g_tabs[i].h)
                    {
                        switchToTab(hwnd, g_tabs[i].page);
                        return 0;
                    }
                }

                // Help link click — same fixed (non-scrolled) zone as the tabs
                if (mx >= g_helpLinkRect.left && mx < g_helpLinkRect.right &&
                    my >= g_helpLinkRect.top  && my < g_helpLinkRect.bottom)
                {
                    SendMessage(GetParent(hwnd), WM_COMMAND,
                                MAKEWPARAM(ID_HELP_DOCUMENTATION, 0), 0);
                    return 0;
                }

                // Export (spec §5.6, broken item #10) — CSV is the
                // concrete deliverable; see exportPairsToCsv() for why
                // PDF/print aren't part of this pass.
                if (mx >= g_exportBtnRect.left && mx < g_exportBtnRect.right &&
                    my >= g_exportBtnRect.top  && my < g_exportBtnRect.bottom)
                {
                    if (exportPairsToCsv(hwnd)) {
                        showToast(L"Exported all analyzed pairs to CSV.", TOAST_SUCCESS);
                    }
                    return 0;
                }

                // Search icon — reveals a real input box (spec §5.6/
                // §5.8), live-filters the current tab's cards.
                if (mx >= g_searchBtnRect.left && mx < g_searchBtnRect.right &&
                    my >= g_searchBtnRect.top  && my < g_searchBtnRect.bottom)
                {
                    toggleSearchBox(hwnd);
                    return 0;
                }
            }

            // Sticky context strip: Re-run button (spec §5.6)
            if (my >= S(4) + S(TAB_BAR_HEIGHT) &&
                my < S(4) + S(TAB_BAR_HEIGHT) + S(CONTEXT_STRIP_HEIGHT) &&
                mx >= g_rerunBtnRect.left && mx < g_rerunBtnRect.right &&
                my >= g_rerunBtnRect.top  && my < g_rerunBtnRect.bottom)
            {
                SendMessageW(GetParent(hwnd), WM_COMMAND,
                             MAKEWPARAM(ID_BTN_RUN_ANALYSIS, 0), 0);
                return 0;
            }

            // Translate to scrolled coords for student card / back button hit tests
            int scrolledY = my + g_scrollY;

            // Students page: block sub-tabs, sort control, flagged-
            // appearance rows, card/back/report — moved into
            // gui_students.cpp (Checkpoint 3 of the ContentProc
            // split). The four original blocks were NOT contiguous
            // here (interleaved with the cancel-analysis/empty-state
            // blocks below and the Overview/Flagged Pairs calls), so
            // consolidating them into this one call moves two of them
            // (flagged-appearance rows, card/back/report) earlier
            // relative to cancel-analysis/empty-state. Verified safe:
            // g_cancelAnalysisBtnRect and g_emptyCardRects occupy
            // screen regions that only exist in states (analysis
            // running; analysis not yet complete) where the Students
            // tab's own hit-rects are stale/empty — WM_PAINT shows the
            // welcome/progress screen instead of tab content in both
            // those states — so at most one of the reordered checks
            // could ever match a given click regardless of order.
            if (g_currentPage == PAGE_STUDENTS) {
                if (handleStudentsClick(hwnd, mx, my)) return 0;
            }

            // Cancel analysis (spec §5.5) — cooperative: sets a flag
            // the pairwise-comparison loop checks at its next
            // checkpoint in runAnalysisPipeline().
            if (g_state.analysisRunning &&
                mx >= g_cancelAnalysisBtnRect.left && mx < g_cancelAnalysisBtnRect.right &&
                scrolledY >= g_cancelAnalysisBtnRect.top && scrolledY < g_cancelAnalysisBtnRect.bottom)
            {
                g_analysisCancelRequested = true;
                return 0;
            }

            // Empty state: option cards fire the same commands as their
            // sidebar twins, so the canvas is actionable rather than
            // just pointing the user at the sidebar.
            if (!g_state.analysisComplete && !g_emptyCardRects.empty()) {
                for (const auto& c : g_emptyCardRects) {
                    if (mx >= c.x && mx < c.x + c.w &&
                        scrolledY >= c.y && scrolledY < c.y + c.h)
                    {
                        SendMessageW(GetParent(hwnd), WM_COMMAND,
                                     MAKEWPARAM(c.cmdId, 0), 0);
                        return 0;
                    }
                }
            }

            // Overview page: exact-dup card, AI-summary deep-links,
            // language-notice toggle, unclassified actions — all moved
            // into gui_overview.cpp (Checkpoint 1 of the ContentProc
            // split). Page-gated inside handleOverviewClick itself, same
            // as every other per-page block here, so this check's
            // position relative to the (page-mutually-exclusive) Students
            // block right below is not behavior-relevant.
            if (g_currentPage == PAGE_OVERVIEW) {
                if (handleOverviewClick(hwnd, mx, scrolledY)) return 0;
            }

            // Flagged Pairs page: collapsed row/header/disclosure/copy/
            // severity-chip/clear-filter clicks — moved into
            // gui_flagged.cpp (Checkpoint 2 of the ContentProc split).
            // The six original blocks were already contiguous here, so
            // this replacement needed no reordering relative to
            // anything around it.
            if (g_currentPage == PAGE_FLAGGED) {
                if (handleFlaggedPairsClick(hwnd, mx, scrolledY)) return 0;
            }

            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdcScreen = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);
            g_viewportH = rect.bottom;

            // Double-buffered painting
            HDC hdcMem = CreateCompatibleDC(hdcScreen);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdcScreen,
                                                      rect.right, rect.bottom);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

            HDC hdc = hdcMem;

            // Keyboard focus (item 2): this screen's region of the
            // combined focusable list is rebuilt fresh from here down
            // (each addFocusable(g_contentFocusables, ...) call sits
            // at the exact point that item's existing hit-rect vector
            // is already populated, same "rebuild every frame"
            // pattern as those vectors themselves).
            g_contentFocusables.clear();

            // Background
            FillRect(hdc, &rect, g_hbrMain);

            // Top accent strip (gold)
            HBRUSH goldBr = CreateSolidBrush(UL_GOLD);
            RECT goldStrip = {0, 0, rect.right, 4};
            FillRect(hdc, &goldStrip, goldBr);
            DeleteObject(goldBr);

            int contentX = 0;
            int contentW = rect.right;
            int totalUsed = 0;

            if (g_state.analysisComplete && g_analysisResults.valid) {
                drawTabBar(hdc, 0, S(4), contentW);

                int pageContentStartY = S(4) + S(TAB_BAR_HEIGHT);

                // Sticky context strip (spec §5.6) — fixed, not
                // scrolled, so the instructor always knows which run
                // they're looking at regardless of scroll position.
                drawContextStrip(hdc, 0, pageContentStartY, contentW);
                pageContentStartY += S(CONTEXT_STRIP_HEIGHT);

                // Classes page grid view gets a second fixed row: the
                // block sub-tab bar. Drawn here (not scrolled) so it
                // stays visible while the student grid below scrolls —
                // same fixed-position pattern as the main tab bar.
                // BLOCK_TAB_BAR_HEIGHT scaled here for consistency with
                // its siblings above even though drawBlockTabBar's own
                // internal drawing isn't scaled until the Classes batch.
                if (g_currentPage == PAGE_STUDENTS && g_selectedStudent < 0) {
                    drawBlockTabBar(hdc, 0, pageContentStartY, contentW);
                    pageContentStartY += S(BLOCK_TAB_BAR_HEIGHT);
                }

                int contentY = pageContentStartY - g_scrollY;

                // Bug fix: scrollable content and the fixed header
                // above it (tab bar / context strip / block tab bar)
                // were drawn onto the same unclipped memory DC — once
                // scrolled, content drawn at contentY could move up
                // into the header's own rows and paint over it, since
                // it's drawn after the header in the same bitmap.
                // Clipping everything below to y >= pageContentStartY
                // keeps it from ever reaching the fixed header zone,
                // regardless of scroll position.
                RECT clipR = {0, pageContentStartY, contentW, rect.bottom};
                HRGN clipRgn = CreateRectRgnIndirect(&clipR);
                SelectClipRgn(hdc, clipRgn);

                if (g_currentPage == PAGE_OVERVIEW) {
                    totalUsed += drawResultsHeader(hdc, contentX, contentY + totalUsed, contentW);
                    totalUsed += drawStatsDashboard(hdc, contentX, contentY + totalUsed, contentW);
                    totalUsed += drawAISummary(hdc, contentX, contentY + totalUsed, contentW);
                }
                else if (g_currentPage == PAGE_STUDENTS) {
                    // Button-rect setup (detail view) + detail/grid
                    // draw call — moved into gui_students.cpp
                    // (Checkpoint 3 of the ContentProc split).
                    totalUsed += drawStudentsPage(hdc, contentX, contentY + totalUsed,
                                                   contentW, pageContentStartY);
                }
                else if (g_currentPage == PAGE_FLAGGED) {
                    totalUsed += drawFlaggedPairs(hdc, contentX, contentY + totalUsed, contentW);
                }

                // Footer
                {
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, RGB(96, 96, 110));
                    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontSmall);
                    RECT ftRect = {contentX + S(30), contentY + totalUsed + S(20),
                                   contentX + contentW - S(30),
                                   contentY + totalUsed + S(70)};
                    DrawTextW(hdc,
                        L"This system provides statistical evidence only.\n"
                        L"All determinations rest with the instructor.\n"
                        L"University of Luzon  |  College of Computer Studies",
                        -1, &ftRect, DT_CENTER);
                    SelectObject(hdc, oldFont);
                    totalUsed += S(90);
                }

                // Release the clip region set above the per-page block.
                SelectClipRgn(hdc, nullptr);
                DeleteObject(clipRgn);
            } else {
                int contentY = S(4) - g_scrollY;
                totalUsed += drawWelcomeScreen(hdc, contentX, contentY + totalUsed, contentW);
            }

            // Update content height
            int extraBarH = (g_currentPage == PAGE_STUDENTS && g_selectedStudent < 0)
                ? S(BLOCK_TAB_BAR_HEIGHT) : 0;
            int newContentH = totalUsed + S(4) +
                (g_state.analysisComplete ? S(TAB_BAR_HEIGHT) + S(CONTEXT_STRIP_HEIGHT) + extraBarH : 0);
            if (newContentH != g_contentH) {
                g_contentH = newContentH;
                updateScrollbar(hwnd);
            }

            // Overlay layer (bug fix: was drawn directly on hdcScreen
            // AFTER the blit below — fine at the low repaint frequency
            // everything here used to run at, but the help carousel's
            // pulse animation forces a full repaint ~60 times/sec, and
            // that turned this into a dozen-plus separate unbuffered
            // GDI/GDI+ calls landing on the real screen every 16ms —
            // each one briefly visible before the next overwrote it,
            // which read as the whole window flickering, not just the
            // carousel. Composited into the memory DC (hdc, same
            // handle as hdcMem) instead, so it goes out in the SAME
            // single BitBlt/AlphaBlend as the main content below —
            // one atomic screen update, no incremental visible draws
            // regardless of repaint frequency.
            //
            // Trade-off, knowingly accepted: the original comments on
            // these (still true below, just no longer describing
            // where they draw) called out staying crisp during an
            // in-progress tab-switch fade as the reason for drawing
            // post-blit. Now composited pre-blit, this overlay WILL
            // fade along with the page for the ~200ms a fade is
            // actually running, in the rare case one is also showing
            // at that moment. A short, rare, cosmetic dimming is a
            // much smaller cost than continuous flicker.

            // Style DNA hover tooltip (spec §4.4) — moved into
            // gui_students.cpp (Checkpoint 3 of the ContentProc
            // split). Sits here in the shared-overlay section, AFTER
            // the main per-page if/else-if above rather than inside
            // it, so it was easy to miss on a first pass. Extracted in
            // place — no reordering relative to its neighbors below.
            if (g_currentPage == PAGE_STUDENTS) {
                drawStudentDnaTooltip(hdc, rect);
            }

            // In-app toast/modal/carousel overlay (compliance fix item 5)
            drawToasts(hdc, rect);
            drawAppModal(hdc, rect);
            drawHelpCarousel(hdc, rect);

            // Keyboard focus ring (item 2) — only for items that
            // resolve into THIS window's region of the combined list
            // (g_contentFocusables; sidebar items are rung by
            // SidebarProc's own WM_PAINT). Content-space items store
            // the same "+g_scrollY baked in" absolute rect every other
            // content hit-rect in this file uses, so the current
            // screen position is that minus the CURRENT g_scrollY —
            // fixed-chrome items (tabs, search/export icons) are
            // already absolute screen coordinates and need no
            // adjustment. Suppressed while a modal OR the help
            // carousel is open — both draw their own ring, and
            // showing a third underneath would be confusing.
            if (g_focusVisible && g_appModal.kind == MODAL_NONE &&
                g_helpCarouselState == HelpCarouselState::Closed) {
                FocusableItem* focused = focusableAt(g_focusedIndex);
                // Range check now needs an upper bound too — title bar
                // was added as a third region after content, so
                // "content" is [sidebar.size(), sidebar.size()+content.size())
                // rather than "everything from sidebar.size() onward".
                int contentBase = (int)g_sidebarFocusables.size();
                int contentEnd  = contentBase + (int)g_contentFocusables.size();
                if (focused && g_focusedIndex >= contentBase && g_focusedIndex < contentEnd) {
                    RECT ringR = focused->rect;
                    if (focused->inContent) {
                        ringR.top    -= g_scrollY;
                        ringR.bottom -= g_scrollY;
                    }
                    // Skip drawing if it scrolled entirely out of the
                    // visible client area (shouldn't normally happen —
                    // scrollFocusedIntoView() keeps it in view — but a
                    // stale index after a list-shrink is possible).
                    if (ringR.bottom > 0 && ringR.top < rect.bottom) {
                        drawFocusRing(hdc, ringR);
                    }
                }
            }

            // Blit to screen — alpha blended when fading in after tab
            // switch. Overlay is now baked into hdc above, so this one
            // call presents the complete frame atomically.
            if (g_contentFadeAlpha < 255) {
                BLENDFUNCTION bf = {};
                bf.BlendOp             = AC_SRC_OVER;
                bf.BlendFlags          = 0;
                bf.SourceConstantAlpha = (BYTE)g_contentFadeAlpha;
                bf.AlphaFormat         = 0; // no per-pixel alpha, constant blend
                AlphaBlend(hdcScreen, 0, 0, rect.right, rect.bottom,
                           hdcMem, 0, 0, rect.right, rect.bottom, bf);
            } else {
                BitBlt(hdcScreen, 0, 0, rect.right, rect.bottom,
                       hdcMem, 0, 0, SRCCOPY);
            }

            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);

            EndPaint(hwnd, &ps);
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

// ═════════════════════════════════════════════════════════════
// ACTION HANDLERS
// ═════════════════════════════════════════════════════════════

static void onSelectFolder(HWND hwnd) {
    std::wstring path;
    if (!pickFolder(hwnd, path)) return;

    int count = countCFiles(path);
    if (count == 0) {
        showToast(L"The selected folder contains no .c files. Please choose "
                   L"a folder with student C source files.", TOAST_WARNING);
        return;
    }

    g_state.dataFolder       = path;
    g_state.fileCount        = count;
    g_state.hasData          = true;
    g_state.analysisComplete = false;
    updateUI();

    audit_log("FOLDER SELECTED",
        "Path: " + toNarrowGC(path) + "\n" +
        "Files found: " + std::to_string(count));
}

static void onImportFiles(HWND hwnd) {
    std::vector<std::wstring> files;
    if (!pickFiles(hwnd, files)) return;

    std::wstring importFolder;
    if (!importFilesToFolder(files, importFolder)) {
        showToast(L"Failed to import files. Please check file permissions.",
                   TOAST_ERROR);
        return;
    }

    wchar_t fullPath[MAX_PATH];
    if (GetFullPathNameW(importFolder.c_str(), MAX_PATH, fullPath, nullptr) > 0) {
        g_state.dataFolder = fullPath;
    } else {
        g_state.dataFolder = importFolder;
    }

    g_state.fileCount        = countCFiles(g_state.dataFolder);
    g_state.hasData          = (g_state.fileCount > 0);
    g_state.analysisComplete = false;
    updateUI();

    std::wstring msg = L"Successfully imported ";
    msg += std::to_wstring(files.size());
    msg += L" file(s).";
    showToast(msg, TOAST_SUCCESS);
}


void onRunAnalysis(HWND hwnd) {
    if (!g_state.hasData || g_state.analysisRunning) return;

    // The AI summary is decided BEFORE the run via the sidebar
    // checkbox (zone ③), not by interrupting mid-flow with a
    // yes/no popup. Fixes broken item #12.
    bool useAI = g_hChkAiSummary &&
                 (SendMessageW(g_hChkAiSummary, BM_GETCHECK, 0, 0) == BST_CHECKED);

    audit_log("ANALYSIS STARTED",
        "Data folder: " + toNarrowGC(g_state.dataFolder) + "\n" +
        "File count: " + std::to_string(g_state.fileCount) + "\n" +
        "AI summary requested: " + std::string(useAI ? "Yes" : "No"));

    g_state.analysisRunning  = true;
    g_state.analysisComplete = false;
    g_state.progressMsg      = L"Starting analysis...";
    g_currentPage      = PAGE_OVERVIEW;
    g_selectedStudent  = -1;
    g_currentBlockTab  = -1;
    g_scrollY          = 0;
    g_progressAnim     = 0;
    g_animRunning      = true;
    SetTimer(g_hMainWindow, TIMER_ANIM, 40, nullptr);
    updateUI();

    // Run analysis in background thread
    std::thread t(analysisThread, g_state.dataFolder, useAI);
    t.detach();
}

static void onViewReport(HWND hwnd) {
    if (g_state.lastReport.empty()) {
        showToast(L"No report available yet. Please run an analysis first.", TOAST_INFO);
        return;
    }

    ShellExecuteW(nullptr, L"open", g_state.lastReport.c_str(),
                  nullptr, nullptr, SW_SHOWNORMAL);
}

// ═════════════════════════════════════════════════════════════
// MENU COMMAND HANDLING
// ═════════════════════════════════════════════════════════════
static void handleMenuCommand(HWND hwnd, int commandId) {
    switch (commandId) {
        case ID_FILE_OPEN_FOLDER:
        case ID_BTN_SELECT_FOLDER:
            onSelectFolder(hwnd);
            break;

        case ID_FILE_IMPORT:
        case ID_BTN_IMPORT_FILES:
            onImportFiles(hwnd);
            break;

        case ID_FILE_GCLASSROOM:
        case ID_BTN_GCLASSROOM:
            onGoogleClassroom(hwnd);
            break;

        case ID_BTN_RUN_ANALYSIS:
            onRunAnalysis(hwnd);
            break;

        case ID_FILE_EXIT:
            DestroyWindow(hwnd);
            break;

        case ID_VIEW_REPORT:
            onViewReport(hwnd);
            break;

        case ID_VIEW_PROFILES:
            showToast(L"Detailed profile viewer will be implemented in Stage 6. "
                       L"For now, you can view student profiles in the generated "
                       L"HTML report.", TOAST_INFO);
            break;

        case ID_VIEW_AUDIT_LOG: {
            std::string logPath = audit_getLogPath();
            DWORD attr = GetFileAttributesA(logPath.c_str());
            if (attr == INVALID_FILE_ATTRIBUTES) {
                showAppModal(L"Audit log",
                    L"No audit log entries yet.\n\n"
                    L"The audit log records folder selections, analysis runs, "
                    L"report generation, and Google Classroom sync activity. "
                    L"It will be created automatically once you use the system.",
                    MODAL_OK, INFO_, L"Close");
                break;
            }
            // Open in Notepad
            ShellExecuteA(nullptr, "open", "notepad.exe",
                          logPath.c_str(), nullptr, SW_SHOWNORMAL);
            break;
        }

        case ID_HELP_DOCUMENTATION:
            // Opens the image carousel. (The old text-based Quick Start
            // modal, showLegacyHelpTextModal(), was removed in the
            // dead-code cleanup pass — see project history if it's
            // ever needed again.)
            openHelpCarousel();
            break;

        case ID_HELP_ABOUT:
            showAppModal(L"About CALSS",
                L"Code Authorship Likelihood System\n"
                L"Version 0.4.0 (Proposal Prototype)\n\n"
                L"An AI-Assisted Code Authorship Likelihood System "
                L"Using Student-Specific Programming Style Fingerprinting "
                L"for Academic Integrity Support.\n\n"
                L"University of Luzon\n"
                L"College of Computer Studies\n\n"
                L"This system provides statistical evidence only. All "
                L"academic integrity determinations rest with the instructor.",
                MODAL_OK, GOLD_500, L"Close");
            break;
    }
}

// Activates whatever is currently focused (Enter key, from the main
// message loop). One switch over FocusKind, filled in screen by
// screen — each case calls the exact same handler function its mouse
// equivalent already calls, so the two paths can't drift apart. Only
// the chrome/sidebar screen's cases are implemented so far; the rest
// resolve to the default no-op until their screens are wired.
static void activateFocused() {
    FocusableItem* item = focusableAt(g_focusedIndex);
    if (!item) return;

    switch (item->kind) {
        case FocusKind::SidebarBtn:
            // dataIndex holds the same command ID the button's own
            // WM_COMMAND forwards here — identical dispatch to a real
            // click, not a re-implementation of it.
            handleMenuCommand(g_hMainWindow, item->dataIndex);
            break;

        case FocusKind::SidebarCheckbox: {
            LRESULT cur = SendMessageW(g_hChkAiSummary, BM_GETCHECK, 0, 0);
            SendMessageW(g_hChkAiSummary, BM_SETCHECK,
                         cur == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED, 0);
            if (g_hSidebar) InvalidateRect(g_hSidebar, nullptr, FALSE);
            break;
        }

        // ── Shared chrome (Overview screen pass, but drawn on every
        // page since drawTabBar()/drawContextStrip() are called
        // unconditionally) ──────────────────────────────────────
        case FocusKind::Tab:
            switchToTab(g_hContent, (ResultsPage)item->dataIndex);
            break;

        case FocusKind::SearchBtn:
            toggleSearchBox(g_hContent);
            break;

        case FocusKind::ExportBtn:
            if (exportPairsToCsv(g_hContent)) {
                showToast(L"Exported all analyzed pairs to CSV.", TOAST_SUCCESS);
            }
            break;

        case FocusKind::HelpLink:
            handleMenuCommand(g_hMainWindow, ID_HELP_DOCUMENTATION);
            break;

        case FocusKind::RerunBtn:
            handleMenuCommand(g_hMainWindow, ID_BTN_RUN_ANALYSIS);
            break;

        // ── Overview screen ───────────────────────────────────────
        case FocusKind::UnclassifiedAssign:
            handleUnclassifiedAction(g_hContent, item->dataIndex, false);
            break;

        case FocusKind::UnclassifiedDelete:
            handleUnclassifiedAction(g_hContent, item->dataIndex, true);
            break;

        case FocusKind::AISummaryLink:
            jumpToFlaggedPair(g_hContent, item->dataIndex);
            break;

        case FocusKind::LangNoticeToggle:
            g_langNoticeExpanded = !g_langNoticeExpanded;
            InvalidateRect(g_hContent, nullptr, FALSE);
            break;

        // ── Classes screen ────────────────────────────────────────
        case FocusKind::BlockChip:
            selectBlockTab(g_hContent, item->dataIndex);
            break;

        case FocusKind::StudentCard:
            openStudentDetail(g_hContent, item->dataIndex);
            break;

        case FocusKind::BackButton:
            closeStudentDetail(g_hContent);
            break;

        case FocusKind::ReportButton:
            generateAndOpenStudentReport();
            break;

        case FocusKind::FlaggedAppearanceRow:
            jumpToFlaggedPair(g_hContent, item->dataIndex);
            break;

        // ── Flagged Pairs screen ──────────────────────────────────
        case FocusKind::SeverityChip:
            selectSeverityFilter(g_hContent, (PairSeverityFilter)item->dataIndex);
            break;

        case FocusKind::ClearFilterBtn:
            clearExactDuplicatesFilter(g_hContent);
            break;

        case FocusKind::CollapsedPairRow:
        case FocusKind::ExpandedPairHeader:
            // Same toggle either way — collapsed row expands it,
            // expanded header collapses it, exactly like the mouse
            // click handlers for each (which also both already called
            // the same shared togglePairExpanded()).
            togglePairExpanded(g_hContent, item->dataIndex);
            break;

        case FocusKind::ShowAllFeaturesToggle:
            toggleShowAllFeatures(g_hContent, item->dataIndex);
            break;

        case FocusKind::CopyFindingsBtn:
            copyFindingsForPair(g_hContent, item->dataIndex);
            break;

        // ── Empty state / analysis progress ───────────────────────
        case FocusKind::EmptyStateCard:
            handleMenuCommand(g_hMainWindow, item->dataIndex);
            break;

        case FocusKind::CancelAnalysisBtn:
            g_analysisCancelRequested = true;
            break;

        // ── Title bar (folded into this combined index, not a
        // separate system — see g_titleBarFocusables' declaration) ──
        case FocusKind::TitleBarMenu:
            if (g_hTitleBar) showTitleBarMenu(g_hTitleBar, item->dataIndex);
            break;

        case FocusKind::TitleBarWinBtn:
            titleBarWinBtnAction(item->dataIndex);
            break;

        default:
            // Not yet wired for this item's screen.
            break;
    }
}


// ═════════════════════════════════════════════════════════════
// CREATE SIDEBAR BUTTONS
// ═════════════════════════════════════════════════════════════
// Positions sidebar controls. Called on creation AND on every resize
// so zone ③ stays pinned to the bottom regardless of window height —
// the primary action is always in the same physical place, which is
// what kills the ~200px of dead space the old fixed layout left.
static void layoutSidebarControls(HWND hSidebar) {
    if (!hSidebar) return;
    RECT rc; GetClientRect(hSidebar, &rc);
    int h = rc.bottom;
    int btnX = S(16);
    // Derived from the real (already-scaled) client width rather than
    // re-deriving from SIDEBAR_WIDTH, so it can't drift from whatever
    // the window was actually sized to.
    int btnW = rc.right - 2 * S(16);

    // ── Zone ① DATA SOURCE — flows from the header block ──
    // header (seal + names + rule) ≈ 196, eyebrow 18, folder chip 38.
    // Kept as the same literal design-space numbers used in
    // SidebarProc's WM_PAINT (each independently wrapped in S()) —
    // see the comment there for why this coupling exists.
    int y = S(196) + S(18) + S(38) + S(10);

    if (g_hBtnSelectFolder)
        MoveWindow(g_hBtnSelectFolder, btnX, y, btnW, S(BUTTON_HEIGHT), TRUE);
    y += S(BUTTON_HEIGHT) + S(8);
    if (g_hBtnImportFiles)
        MoveWindow(g_hBtnImportFiles, btnX, y, btnW, S(BUTTON_HEIGHT), TRUE);
    y += S(BUTTON_HEIGHT) + S(8);
    if (g_hBtnGClassroom)
        MoveWindow(g_hBtnGClassroom, btnX, y, btnW, S(BUTTON_HEIGHT), TRUE);

    // ── Zone ③ ANALYSIS — pinned to the bottom ──
    const int runH   = S(44);   // spec §5.2: 44px primary
    const int chkH   = S(22);
    const int statusH = S(20);  // "Analysis complete · 20:10" line
    int runY = h - S(16) - statusH - runH;
    int chkY = runY - chkH - S(10);

    if (g_hChkAiSummary)
        MoveWindow(g_hChkAiSummary, btnX, chkY, btnW, chkH, TRUE);
    if (g_hBtnRunAnalysis)
        MoveWindow(g_hBtnRunAnalysis, btnX, runY, btnW, runH, TRUE);
}

static void createSidebarButtons(HWND hSidebarParent) {
    HINSTANCE hInst = GetModuleHandle(nullptr);
    int btnW = S(SIDEBAR_WIDTH) - S(32);

    // Zone ① — three secondary source actions. Initial sizes here only
    // matter for the brief instant before layoutSidebarControls() (at
    // the end of this function) repositions everything to its real,
    // scaled steady state — scaled anyway for consistency / to avoid
    // any visible unscaled flash.
    g_hBtnSelectFolder = CreateWindowW(
        L"BUTTON", L"Select folder",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        S(16), 0, btnW, S(BUTTON_HEIGHT),
        hSidebarParent, (HMENU)ID_BTN_SELECT_FOLDER, hInst, nullptr);

    g_hBtnImportFiles = CreateWindowW(
        L"BUTTON", L"Import .c files",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        S(16), 0, btnW, S(BUTTON_HEIGHT),
        hSidebarParent, (HMENU)ID_BTN_IMPORT_FILES, hInst, nullptr);

    g_hBtnGClassroom = CreateWindowW(
        L"BUTTON", L"Sync Google Classroom",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        S(16), 0, btnW, S(BUTTON_HEIGHT),
        hSidebarParent, (HMENU)ID_BTN_GCLASSROOM, hInst, nullptr);

    // Zone ③ — the AI toggle decided BEFORE the run. This is what
    // deletes the mid-flow yes/no popup (broken item #12).
    g_hChkAiSummary = CreateWindowW(
        L"BUTTON", L"Include AI summary",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        S(16), 0, btnW, S(22),
        hSidebarParent, (HMENU)ID_CHK_AI_SUMMARY, hInst, nullptr);
    SendMessageW(g_hChkAiSummary, BM_SETCHECK, BST_CHECKED, 0);

    g_hBtnRunAnalysis = CreateWindowW(
        L"BUTTON", L"Run analysis",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        S(16), 0, btnW, S(44),
        hSidebarParent, (HMENU)ID_BTN_RUN_ANALYSIS, hInst, nullptr);

    SendMessage(g_hBtnSelectFolder, WM_SETFONT, (WPARAM)g_hFontButton, TRUE);
    SendMessage(g_hBtnImportFiles,  WM_SETFONT, (WPARAM)g_hFontButton, TRUE);
    SendMessage(g_hBtnGClassroom,   WM_SETFONT, (WPARAM)g_hFontButton, TRUE);
    SendMessage(g_hChkAiSummary,    WM_SETFONT, (WPARAM)g_hFontBodyNew, TRUE);
    SendMessage(g_hBtnRunAnalysis,  WM_SETFONT, (WPARAM)g_hFontButton, TRUE);

    layoutSidebarControls(hSidebarParent);

    EnableWindow(g_hBtnRunAnalysis, FALSE);
}

// ═════════════════════════════════════════════════════════════
// MAIN WINDOW PROCEDURE
// ═════════════════════════════════════════════════════════════
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg,
                                     WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            registerTitleBarClass(hInst);
            g_hTitleBar = CreateWindowExW(0, L"CALSSTitleBar", L"",
                WS_CHILD | WS_VISIBLE,
                0, 0, 800, TITLEBAR_HEIGHT,
                hwnd, nullptr, hInst, nullptr);

            WNDCLASSEXW wcSidebar = {};
            wcSidebar.cbSize        = sizeof(WNDCLASSEXW);
            wcSidebar.lpfnWndProc   = SidebarProc;
            wcSidebar.hInstance     = hInst;
            wcSidebar.hCursor       = LoadCursor(nullptr, IDC_ARROW);
            wcSidebar.lpszClassName = L"AuthorshipSidebar";
            RegisterClassExW(&wcSidebar);

            WNDCLASSEXW wcContent = {};
            wcContent.cbSize        = sizeof(WNDCLASSEXW);
            wcContent.lpfnWndProc   = ContentProc;
            wcContent.hInstance     = hInst;
            wcContent.hCursor       = LoadCursor(nullptr, IDC_ARROW);
            wcContent.lpszClassName = L"AuthorshipContent";
            RegisterClassExW(&wcContent);

            g_hSidebar = CreateWindowExW(0, L"AuthorshipSidebar", L"",
                WS_CHILD | WS_VISIBLE,
                0, 0, SIDEBAR_WIDTH, 600,
                hwnd, nullptr, hInst, nullptr);

            g_hContent = CreateWindowExW(0, L"AuthorshipContent", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                SIDEBAR_WIDTH, 0, 800, 600,
                hwnd, nullptr, hInst, nullptr);

            // Real search input (spec §5.6/§5.8) — hidden by default,
            // shown/positioned when the search icon is toggled on.
            // A genuine EDIT control rather than custom-drawn text
            // entry, since text input (cursor, selection, IME) is a
            // solved problem the native control already handles
            // correctly.
            g_hSearchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL,
                0, 0, 200, 26,
                g_hContent, (HMENU)ID_SEARCH_EDIT, hInst, nullptr);
            SendMessage(g_hSearchEdit, WM_SETFONT, (WPARAM)g_hFontBodyNew, TRUE);

            createSidebarButtons(g_hSidebar);

            InitCommonControls();
            g_hStatus = CreateWindowExW(0, STATUSCLASSNAMEW, L"",
                WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                0, 0, 0, 0, hwnd, (HMENU)4001, hInst, nullptr);

            int parts[] = {S(350), S(600), -1};
            SendMessage(g_hStatus, SB_SETPARTS, 3, (LPARAM)parts);
            SendMessageW(g_hStatus, SB_SETTEXTW, 0,
                         (LPARAM)L" Ready - no data loaded");
            SendMessageW(g_hStatus, SB_SETTEXTW, 1,
                         (LPARAM)L" Profiles: 0");
            SendMessageW(g_hStatus, SB_SETTEXTW, 2,
                         (LPARAM)L" UL - College of Computer Studies");

            return 0;
        }

        // Frameless popup windows maximize to the full screen by
        // default, covering the taskbar. Clamp to the monitor's work
        // area so maximize behaves like a normal window.
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = {};
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfoW(mon, &mi)) {
                mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
                mmi->ptMaxPosition.y = mi.rcWork.top  - mi.rcMonitor.top;
                mmi->ptMaxSize.x     = mi.rcWork.right  - mi.rcWork.left;
                mmi->ptMaxSize.y     = mi.rcWork.bottom - mi.rcWork.top;
            }
            mmi->ptMinTrackSize.x = 1100;  // below this the layout breaks
            mmi->ptMinTrackSize.y = 700;
            return 0;
        }

        // ── Frameless window (spec §5.1) ──────────────────────
        // Returning 0 with wParam TRUE removes the entire non-client
        // area (caption + borders), letting the custom title bar own
        // the top of the window. WM_NCHITTEST below restores resize
        // behavior manually, so the window still behaves normally.
        case WM_NCCALCSIZE:
            if (wParam == TRUE) return 0;
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);

        case WM_NCHITTEST: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT wr; GetWindowRect(hwnd, &wr);

            // Maximized windows have no resize edges
            if (IsZoomed(hwnd)) return HTCLIENT;

            bool left   = pt.x < wr.left   + RESIZE_BORDER;
            bool right  = pt.x > wr.right  - RESIZE_BORDER;
            bool top    = pt.y < wr.top    + RESIZE_BORDER;
            bool bottom = pt.y > wr.bottom - RESIZE_BORDER;

            if (top    && left)  return HTTOPLEFT;
            if (top    && right) return HTTOPRIGHT;
            if (bottom && left)  return HTBOTTOMLEFT;
            if (bottom && right) return HTBOTTOMRIGHT;
            if (left)   return HTLEFT;
            if (right)  return HTRIGHT;
            if (top)    return HTTOP;
            if (bottom) return HTBOTTOM;
            return HTCLIENT;
        }

        case WM_SIZE:
            layoutChildren(hwnd);
            if (g_hTitleBar) InvalidateRect(g_hTitleBar, nullptr, FALSE);
            return 0;

        case WM_COMMAND:
            handleMenuCommand(hwnd, LOWORD(wParam));
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_ANIM && g_animRunning) {
                // Animate progress bar: bounce between 0-100 smoothly
                g_progressAnim += 3;
                if (g_progressAnim > 100) g_progressAnim = 0;
                if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
            }
            else if (wParam == TIMER_STAT_COUNTUP && g_statsAnimActive) {
                // Hero stats are painted by the CONTENT child window,
                // not the main window this timer fires on — same
                // pattern as TIMER_ANIM above. Self-kills once the
                // last-staggered card (index 2, 120ms delay + 600ms
                // ramp) has fully completed.
                if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
                DWORD elapsed = GetTickCount() - g_statsAnimStartTick;
                if (elapsed > (2 * STAT_COUNTUP_STAGGER_MS + STAT_COUNTUP_MS + 50)) {
                    g_statsAnimActive = false;
                    KillTimer(hwnd, TIMER_STAT_COUNTUP);
                }
            }
            return 0;

        case WM_ANALYSIS_PROGRESS:
            updateUI();
            return 0;

        case WM_ANALYSIS_COMPLETE:
            g_state.analysisRunning  = false;
            g_state.analysisComplete = true;
            g_animRunning = false;
            g_progressAnim = 100;
            KillTimer(hwnd, TIMER_ANIM);

            // Kick off the hero-stat count-up (spec §5.7) — once per
            // run, not on every repaint. ~900ms covers 3 staggered
            // 600ms ramps (0/60/120ms delays).
            g_statsAnimStartTick = GetTickCount();
            g_statsAnimActive = true;
            SetTimer(hwnd, TIMER_STAT_COUNTUP, 16, nullptr);

            updateUI();

            // Log completion with report hash for tamper-evidence
            if (g_analysisResults.valid) {
                std::string reportPath = toNarrowGC(g_state.lastReport);
                std::string hash = reportPath.empty() ? "" : audit_sha256File(reportPath);
                audit_log("ANALYSIS COMPLETE",
                    "Files analyzed: " + std::to_string(g_state.fileCount) + "\n" +
                    "Students profiled: " + std::to_string(g_analysisResults.profiles.size()) + "\n" +
                    "Total pairs: " + std::to_string(g_analysisResults.totalPairs) + "\n" +
                    "Pairs flagged: " + std::to_string(g_analysisResults.flaggedCount) + "\n" +
                    "Exact duplicates: " + std::to_string(g_analysisResults.exactDuplicates) + "\n" +
                    "Report file: " + reportPath + "\n" +
                    "Report: " + hash);
            }

            // Auto-open the report
            if (!g_state.lastReport.empty()) {
                ShellExecuteW(nullptr, L"open", g_state.lastReport.c_str(),
                              nullptr, nullptr, SW_SHOWNORMAL);
            }
            return 0;

        case WM_DESTROY:
            destroyFonts();
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

// ═════════════════════════════════════════════════════════════
// GUI ENTRY POINT
// ═════════════════════════════════════════════════════════════
int runGUI(HINSTANCE hInstance)
{
    // Item 4 — DPI awareness. Must happen before ANY window is
    // created (including the splash screen just below), or Windows
    // locks in the legacy "system DPI aware" behavior for this
    // process instead. Per-Monitor-V2 disables Windows' own automatic
    // bitmap-stretch scaling, so our manual S()/font scaling (see
    // g_dpiScale's declaration) is the only scaling applied — without
    // this, the two would compound. GetDpiForSystem() needs no window
    // handle, consistent with the "startup-time detection only, no
    // WM_DPICHANGED" scope agreed for this pass. Both loaded
    // dynamically — see g_dpiScale's declaration for why (Windows 7
    // target pin means these aren't SDK-declared here).
    {
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        if (hUser32) {
            auto pSetCtx = (PFN_SetProcessDpiAwarenessContext)
                GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
            auto pGetDpi = (PFN_GetDpiForSystem)
                GetProcAddress(hUser32, "GetDpiForSystem");
            if (pSetCtx) pSetCtx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2_VALUE);
            if (pGetDpi) g_dpiScale = pGetDpi() / 96.0;
        }
    }

    // GDI+ must init before any drawing (including splash screen)
    initGdiPlus();

    // Show splash screen while initializing
    showSplashScreen(hInstance);

    createFonts();
    CoInitialize(nullptr);

    WNDCLASSEXW wc      = {};
    wc.cbSize           = sizeof(WNDCLASSEXW);
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = WindowProc;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)CreateSolidBrush(BG_MAIN);
    wc.lpszClassName    = WINDOW_CLASS_NAME;
    wc.hIconSm          = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"Failed to register window class",
                    L"Error", MB_OK | MB_ICONERROR);
        destroyFonts();
        return 1;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - WINDOW_WIDTH)  / 2;
    int posY = (screenH - WINDOW_HEIGHT) / 2;

    // The menu is NOT attached to the window — it's owned here and
    // its submenus are opened by the custom title bar via
    // TrackPopupMenu. This keeps every existing menu command working
    // while removing the native menu bar's light chrome.
    g_hAppMenu = createMainMenu();

    // WS_POPUP + WS_THICKFRAME: no caption, but still resizable and
    // still animates/snaps like a normal window. WM_NCCALCSIZE strips
    // the remaining non-client area; WM_NCHITTEST restores the edges.
    g_hMainWindow = CreateWindowExW(
        0, WINDOW_CLASS_NAME, WINDOW_TITLE,
        WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
        WS_SYSMENU | WS_CLIPCHILDREN,
        posX, posY, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hMainWindow) {
        destroyFonts();
        return 1;
    }

    // Force a frame recalculation so WM_NCCALCSIZE applies immediately
    SetWindowPos(g_hMainWindow, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

    ShowWindow(g_hMainWindow, SW_SHOW);
    UpdateWindow(g_hMainWindow);
    layoutChildren(g_hMainWindow);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        // Keyboard focus (compliance sweep item 2): Tab/Shift+Tab and
        // Enter are intercepted here, centrally, rather than left to
        // whichever HWND happens to hold real Win32 focus — the
        // "logically focused" item (which may be a sidebar button we
        // never actually SetFocus()) is independent of that. Scoped
        // to the main window and its two direct children, PLUS the
        // sync sheet (its own separate branch just below, since it
        // has its own separate focus list — see the comment at
        // g_syncSheetFocusables for why) — any other native dialog's
        // own keyboard handling is untouched, out of scope for this
        // pass. Skipped while the search edit box has real focus
        // (typing there is a distinct interaction mode).
        bool inScopeWindow = (msg.hwnd == g_hMainWindow || msg.hwnd == g_hSidebar ||
                               msg.hwnd == g_hContent);
        bool searchEditFocused = g_hSearchEdit && (GetFocus() == g_hSearchEdit);

        if (msg.message == WM_KEYDOWN && inScopeWindow && !searchEditFocused) {
            if (msg.wParam == VK_TAB) {
                bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                if (g_appModal.kind == MODAL_CONFIRM) {
                    g_modalFocusOnPrimary = !g_modalFocusOnPrimary;
                } else if (g_helpCarouselState == HelpCarouselState::Open) {
                    // Same unconditional-toggle idiom as MODAL_CONFIRM
                    // above — harmless even on the last step where
                    // Skip doesn't exist, since drawHelpCarousel's own
                    // onNext computation already falls back to true
                    // whenever skipExists is false.
                    g_helpCarouselFocusOnNext = !g_helpCarouselFocusOnNext;
                } else if (g_appModal.kind == MODAL_NONE) {
                    focusMove(shift ? -1 : 1);
                    scrollFocusedIntoView();
                }
                if (g_hSidebar) InvalidateRect(g_hSidebar, nullptr, FALSE);
                if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
                continue;
            }
            // Carousel excluded here too (not just the Tab branch
            // above) — Enter must fall through to dispatch so
            // ContentProc's own WM_KEYDOWN carousel gate handles it,
            // exactly the same reasoning as why AppModal is excluded:
            // this branch is only for the "normal page" case.
            if (msg.wParam == VK_RETURN && g_appModal.kind == MODAL_NONE &&
                g_helpCarouselState == HelpCarouselState::Closed) {
                activateFocused();
                continue;
            }
        }

        // Sync sheet's own Tab/Enter branch — separate from the block
        // above since it drives g_syncSheetFocusables/g_syncFocusedIndex
        // rather than the sidebar/content system. Skipped while the
        // tree view has real Win32 focus, so its own native keyboard
        // handling (arrow keys, Space) gets those keys instead — see
        // the FocusKind::SyncTree comment where it's built for the
        // known limitation this implies (Tab doesn't hand focus back
        // out of the tree once it's there).
        bool syncSheetOpen = g_hSyncSheet && (msg.hwnd == g_hSyncSheet);
        bool syncTreeFocused = g_hSyncTree && (GetFocus() == g_hSyncTree);

        if (msg.message == WM_KEYDOWN && syncSheetOpen && !syncTreeFocused) {
            if (msg.wParam == VK_TAB) {
                bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                syncFocusMove(shift ? -1 : 1);
                // Hand real focus to the tree the instant Tab lands on
                // its stop, so its native keyboard handling is ready
                // to take over immediately rather than needing an
                // extra keypress.
                if (g_syncFocusedIndex >= 0 &&
                    g_syncFocusedIndex < (int)g_syncSheetFocusables.size() &&
                    g_syncSheetFocusables[g_syncFocusedIndex].kind == FocusKind::SyncTree &&
                    g_hSyncTree)
                {
                    SetFocus(g_hSyncTree);
                }
                InvalidateRect(g_hSyncSheet, nullptr, FALSE);
                continue;
            }
            if (msg.wParam == VK_RETURN) {
                activateSyncFocused();
                continue;
            }
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        // Native sidebar buttons take real Win32 focus on mouse click
        // (default button behavior); reclaim it for g_hContent right
        // after so the centralized interception above stays
        // authoritative regardless of what was just clicked with the
        // mouse. Also clears the focus-visible ring, matching the
        // same "any mouse click clears it" handling already in
        // ContentProc's own WM_LBUTTONDOWN for canvas clicks — sidebar
        // clicks never reach that handler, so it's mirrored here.
        // Triggered on WM_LBUTTONUP, not WM_LBUTTONDOWN — stealing
        // focus between a button's down and up breaks its internal
        // press-tracking (needed a second click to register). Up
        // fires only after the button has already completed its own
        // click (BN_CLICKED already sent), so reclaiming focus here
        // can't interfere with it.
        if (msg.message == WM_LBUTTONUP &&
            (msg.hwnd == g_hBtnSelectFolder || msg.hwnd == g_hBtnImportFiles ||
             msg.hwnd == g_hBtnGClassroom   || msg.hwnd == g_hChkAiSummary ||
             msg.hwnd == g_hBtnRunAnalysis))
        {
            SetFocus(g_hContent);
            g_focusVisible = false;
            if (g_hSidebar) InvalidateRect(g_hSidebar, nullptr, FALSE);
        }
    }

    CoUninitialize();
    shutdownGdiPlus();
    return (int)msg.wParam;
}

#endif // _WIN32