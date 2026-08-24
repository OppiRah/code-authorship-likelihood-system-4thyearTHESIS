#ifndef GUI_COMMON_H
#define GUI_COMMON_H
// ─────────────────────────────────────────────────────────────
// gui_common.h
// Shared contract for the split gui_*.cpp translation units.
//
// This header is the ONLY thing the per-screen gui_*.cpp files
// share. It carries, in order:
//   1. the include prologue (order-sensitive — see below)
//   2. app-wide #defines (timer IDs, control IDs, custom WM_*)
//   3. the design-system palette and layout constants
//   4. types named in any cross-file signature or extern global
//   5. extern declarations for the 66 cross-screen globals
//   6. prototypes for the 55 cross-screen functions
//
// Every gui_*.cpp includes this FIRST, before anything else, so
// that all translation units see an identical preprocessor state
// (UNICODE/_UNICODE/WINVER in particular — see the note below).
//
// Globals declared `extern` here are DEFINED exactly once, in
// gui_common.cpp. Adding an extern here without a matching
// definition there is an undefined-reference link error; defining
// one in two places is a duplicate-symbol link error. Neither
// shows up in a single-file syntax check.
// ─────────────────────────────────────────────────────────────

// Must be defined before any Windows header is pulled in. The sync
// sheet's checkbox tree (SysTreeView32 + TVS_CHECKBOXES) needs
// TVN_ITEMCHANGEDW, which requires comctl32 v6 semantics — targeting
// anything older silently drops that notification.
// Must be defined before any Windows header loads. Without this, ALL
// commctrl.h macros (TreeView_InsertItem, TreeView_SetCheckState,
// etc.) silently resolve to their ANSI (*_A) message variants — even
// though this file exclusively builds and passes WIDE (TVITEMW /
// TVINSERTSTRUCTW) structures throughout. Windows then reads each
// wchar_t* pszText pointer as if it were a char* — and since a UTF-16
// character like 'P' is the byte sequence 0x50 0x00, it renders as
// the single letter "P" and stops at that null byte. This was the
// exact cause of tree rows showing single letters ("P", "F", "S", "M")
// instead of full text.
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0700
#endif

// IMPORTANT: analysis headers BEFORE Windows headers.
// Windows defines macros like IDENTIFIER that collide with our enums.
#include <string>
#include <vector>

#include "tokenizer.h"
#include "features.h"
#include "knn_model.h"
#include "similarity.h"
#include "gemini.h"
#include "results_data.h"
#include "google_classroom.h"
#include "audit_log.h"

// Pulls in windows.h
#include "gui.h"

#ifdef _WIN32


#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <windowsx.h>
#include <thread>
#include <atomic>
#include <cmath>
#include <cwctype>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <set>
#include <algorithm>
#include <map>

// GDI+ for smooth rounded corners, shadows, gradients, fades
#include <objidl.h>
#include <gdiplus.h>
#include <wingdi.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")
// <functional> is used by showAppModal's std::function callback. It
// arrived transitively in the single-file build; named explicitly
// here so every split TU gets it deterministically.
#include <functional>
using namespace Gdiplus;

// ── App-wide macros ───────────────────────────────────────────
#define TIMER_ANIM 1001
#define TIMER_FADE 1002
#define ID_FILE_OPEN_FOLDER     1001
#define ID_FILE_IMPORT          1002
#define ID_FILE_GCLASSROOM      1003
#define ID_FILE_EXIT            1010
#define ID_VIEW_REPORT          2001
#define ID_VIEW_PROFILES        2002
#define ID_VIEW_AUDIT_LOG       2003
#define ID_HELP_ABOUT           3001
#define ID_HELP_DOCUMENTATION   3002
#define ID_BTN_SELECT_FOLDER    5001
#define ID_BTN_IMPORT_FILES     5002
#define ID_BTN_GCLASSROOM       5003
#define ID_BTN_RUN_ANALYSIS     5004
#define ID_CHK_AI_SUMMARY       5005
#define WM_ANALYSIS_PROGRESS    (WM_USER + 1)
#define WM_ANALYSIS_COMPLETE    (WM_USER + 2)
#define WM_SYNC_AUTHDONE        (WM_USER + 3)
#define WM_SYNC_COURSESDONE     (WM_USER + 4)
#define WM_SYNC_PROGRESS        (WM_USER + 5)
#define WM_SYNC_DONE            (WM_USER + 6)
#define TOAST_DURATION_MS 4000
#define TOAST_MAX_STACK   3
#define TIMER_TOAST 1007
#define TIMER_HELP_CAROUSEL 1008
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2_VALUE ((HANDLE)(-4))
#define TIMER_TAB_UNDERLINE 1003
#define ID_SEARCH_EDIT 6001
#define TIMER_STAT_COUNTUP 1004
#define STAT_COUNTUP_MS    600
#define STAT_COUNTUP_STAGGER_MS 60
#define TIMER_PAIR_EXPAND 1005
#define PAIR_EXPAND_ANIM_MS 240
#define TIMER_COPY_FEEDBACK 1006
#define COPY_FEEDBACK_MS 1200
#define toNarrow(w) toNarrowGC(w)

// ── Default dimensions ────────────────────────────────────────
// WINDOW_CLASS_NAME / WINDOW_TITLE deliberately do NOT live here.
// runGUI() is their only user, and because they are `const wchar_t*`
// POINTERS rather than folded integer constants, every translation
// unit that included this header without referencing them emitted an
// unused-variable warning under -Wall. They now sit in gui.cpp beside
// their sole call site.
static const int WINDOW_WIDTH       = 1400;
static const int WINDOW_HEIGHT      = 860;
static const int SIDEBAR_WIDTH      = 300;  // spec §5.2
static const int TITLEBAR_HEIGHT    = 40;   // spec §5.1 custom chrome
static const int GOLD_RULE_HEIGHT   = 2;    // the one identity element kept verbatim
static const int STATUSBAR_HEIGHT   = 26;
static const int BUTTON_HEIGHT      = 40;
static const int SIDEBAR_PADDING    = 20;
static const int RESIZE_BORDER      = 6;    // hit-test margin for frameless resize

// ── CALSS DESIGN SYSTEM — PALETTE ─────────────────────────────
// Three roles, strictly separated:
//   BRAND    maroon = structure & severity, gold = wayfinding
//   SURFACE  gray   = everything else (~85% of the UI)
//   SEMANTIC risk   = deliberately NOT brand gold
//
// HARD RULE: gold-500 never signals risk. Previously COLOR_WARNING
// was byte-identical to UL_GOLD, so the brand accent and the
// "moderate risk" signal were the same color — meaning gold read as
// a warning everywhere it appeared. Moderate risk is now risk-mod
// (a browner amber) and must never sit adjacent to gold-500 in the
// same component.
//
// Maroon fills are reserved for: primary action, active tab, flagged
// severity headers, verdict banner. More than 4 maroon fills on one
// screen and maroon has stopped meaning anything.

// BRAND
static const COLORREF MAROON_900 = RGB(59,  10,  17);  // deepest fills, severity banners
static const COLORREF MAROON_800 = RGB(87,  16,  24);  // card headers, active nav fill
static const COLORREF MAROON_700 = RGB(122, 22,  38);  // primary button rest
static const COLORREF MAROON_600 = RGB(148, 32,  46);  // primary button hover
static const COLORREF GOLD_500   = RGB(212, 165, 55);  // brand accent, eyebrows, active indicator
static const COLORREF GOLD_400   = RGB(232, 194, 95);  // hover / focus ring
static const COLORREF GOLD_100   = RGB(245, 230, 188); // rare — high-emphasis numerals only
static const COLORREF SPLASH_GOLD = RGB(212, 175, 55); // splash-screen accent — visually close to
                                                         // GOLD_500 but kept as its own token per
                                                         // explicit sign-off (not confirmed identical)

// SURFACE
static const COLORREF GRAY_950 = RGB(19,  19,  22);   // app background
static const COLORREF GRAY_900 = RGB(26,  26,  30);   // content canvas
static const COLORREF GRAY_850 = RGB(33,  33,  38);   // card
static const COLORREF GRAY_800 = RGB(42,  42,  49);   // card hover / input
static const COLORREF GRAY_700 = RGB(56,  56,  64);   // hairline borders
static const COLORREF GRAY_500 = RGB(108, 108, 120);  // disabled text, dividers
static const COLORREF GRAY_400 = RGB(154, 154, 166);  // secondary text, labels
static const COLORREF GRAY_200 = RGB(214, 214, 222);  // body text
static const COLORREF WHITE_   = RGB(244, 244, 247);  // headings, key numerals

// SEMANTIC — decoupled from brand gold
static const COLORREF RISK_HIGH = RGB(198, 57,  74);  // >=85% similarity, mismatch rows
static const COLORREF RISK_MOD  = RGB(201, 138, 43);  // 70-84% moderate  (NOT gold)
static const COLORREF RISK_LOW  = RGB(79,  158, 114); // <70%, match rows, success
static const COLORREF INFO_     = RGB(74,  135, 184); // data-quality notices, neutral system

// ── Legacy aliases ────────────────────────────────────────────
// Existing call sites throughout this file still reference the old
// names. Rather than a risky 400-site rename in one pass, these map
// old names onto the new tokens so the redesign lands incrementally
// and every screen keeps compiling between steps. Screens get
// migrated to the new names as each is rebuilt.
static const COLORREF UL_MAROON          = MAROON_700;
static const COLORREF UL_MAROON_DARK     = MAROON_800;
static const COLORREF UL_MAROON_LIGHT    = MAROON_600;
static const COLORREF UL_GOLD            = GOLD_500;
static const COLORREF UL_GOLD_DARK       = RGB(165, 130, 30);

static const COLORREF BG_MAIN            = GRAY_900;
static const COLORREF BG_SIDEBAR         = GRAY_950;
static const COLORREF BG_PANEL           = GRAY_850;
static const COLORREF BG_PANEL_HOVER     = GRAY_800;
static const COLORREF BORDER_COLOR       = GRAY_700;
static const COLORREF BORDER_GOLD        = GOLD_500;

static const COLORREF TEXT_MAIN          = WHITE_;
static const COLORREF TEXT_DIM           = GRAY_400;
static const COLORREF TEXT_INFO          = GRAY_200;

static const COLORREF COLOR_SUCCESS      = RISK_LOW;
static const COLORREF COLOR_SUCCESS_BG   = RGB(27,  50,  32);
static const COLORREF COLOR_SUCCESS_BG_ALT = RGB(35, 50, 42); // near COLOR_SUCCESS_BG but kept
                                                                 // as its own token per explicit
                                                                 // sign-off (not confirmed identical)
static const COLORREF COLOR_SUCCESS_TEXT = RGB(120, 200, 155);
static const COLORREF COLOR_WARNING      = RISK_MOD;   // ← was UL_GOLD; now decoupled
static const COLORREF COLOR_ERROR        = RISK_HIGH;
static const COLORREF COLOR_ERROR_BG     = RGB(60,  20,  20);
static const COLORREF COLOR_ERROR_TEXT   = RGB(229, 115, 115);

// ─────────────────────────────────────────────────────────────
// SHARED TYPES
// ─────────────────────────────────────────────────────────────

// ── Application state ─────────────────────────────────────────
struct AppState {
    std::wstring dataFolder;
    int          fileCount;
    bool         hasData;
    bool         analysisRunning;
    bool         analysisComplete;
    std::wstring progressMsg;
    std::wstring lastReport;
};

// ── In-app notification/confirmation system (compliance fix item 5)
// Replaces native MessageBox for anything that isn't the one
// unrecoverable startup failure (window-class registration). Two
// primitives, both drawn as an overlay on g_hContent's own WM_PAINT
// — same "drawn on hdcScreen after the main blit, always opaque
// regardless of page-fade" pattern already used for the DNA hover
// tooltip:
//   - Toast: non-blocking, auto-dismisses, stacks bottom-right. For
//     success/info/error messages that don't need a decision.
//   - AppModal: blocking overlay with a scrim, for messages that need
//     an actual decision (delete confirmation) or longer read-only
//     content (About, Quick Start Guide).
// Both trigger functions target g_hContent directly rather than
// taking an hwnd parameter, so any call site anywhere in the file —
// sidebar handlers, menu commands, ContentProc's own click handlers —
// can call them uniformly.
enum ToastKind { TOAST_INFO, TOAST_SUCCESS, TOAST_WARNING, TOAST_ERROR };
struct AppToast {
    std::wstring text;
    ToastKind kind;
    DWORD showUntilTick;
};

// Click-to-dismiss hit regions, rebuilt each paint — same per-item
// vector pattern used throughout this file (g_blockTabRects etc.).
struct ToastRect { RECT r; int index; };

enum ModalKind { MODAL_NONE, MODAL_OK, MODAL_CONFIRM };
struct AppModal {
    ModalKind kind = MODAL_NONE;
    std::wstring title;
    std::wstring body;
    std::wstring primaryLabel;
    std::wstring secondaryLabel;
    COLORREF accent = 0;
    std::function<void(bool)> onResult; // true = primary pressed, false = secondary/Esc
};

static const int HELP_STEP_COUNT = 10;

enum class HelpCarouselState { Closed, Open };

// caused a surprise swallowing Escape in the Assign dialog in Item 5).
//
// Sidebar buttons never receive real Win32 focus under this system;
// Enter on a focused sidebar item calls its handler function directly
// (onSelectFolder(), etc.) — the same function the mouse click already
// calls — so keyboard and mouse activation can never drift apart.
enum class FocusKind {
    None,
    SidebarBtn, SidebarCheckbox,
    Tab, SearchBtn, ExportBtn, HelpLink, RerunBtn,
    BlockChip, SeverityChip, ClearFilterBtn,
    StudentCard, BackButton, ReportButton, FlaggedAppearanceRow,
    UnclassifiedAssign, UnclassifiedDelete, AISummaryLink, LangNoticeToggle,
    CollapsedPairRow, ExpandedPairHeader, ShowAllFeaturesToggle, CopyFindingsBtn,
    EmptyStateCard, CancelAnalysisBtn,
    // Sync sheet — SyncPrimaryBtn/SyncSecondaryBtn are reused across
    // all four sheet states (same as the underlying g_syncPrimaryBtnRect/
    // g_syncSecondaryBtnRect globals they mirror); activateFocused()
    // dispatches on g_syncState at activation time, exactly like the
    // mouse handler already does. SyncTree is a stop that hands real
    // Win32 focus to the native tree control rather than staying
    // virtual — see the comment where it's built.
    SyncPrimaryBtn, SyncSecondaryBtn, SyncSelectAll, SyncSelectNone,
    SyncExcludePrelim, SyncMinimize, SyncTree,
    // Title bar — TitleBarMenu (dataIndex 0..2 = File/View/Help) opens
    // the same popup menu the mouse click does; TitleBarWinBtn
    // (dataIndex 0..2 = minimize/maximize/close) dispatches through
    // the same shared handler the mouse click uses.
    TitleBarMenu, TitleBarWinBtn,
};

struct FocusableItem {
    FocusKind kind = FocusKind::None;
    RECT rect = {};       // screen-space (matches whatever space that
                           // item's existing hit-rect already uses —
                           // content-space items already carry +g_scrollY
                           // baked in, same as their mouse hit-test rects)
    int  dataIndex = -1;  // generic payload: pair/student/file index,
                           // or a filter enum cast to int
    bool inContent = true; // false = lives in the fixed chrome
                           // (sidebar/tab bar), true = scrolls with
                           // page content — determines which window's
                           // WM_PAINT draws its ring and whether
                           // scroll-into-view applies
};

// ── Page navigation state ────────────────────────────────────
enum ResultsPage {
    PAGE_OVERVIEW = 0,    // Stats + AI Summary
    PAGE_STUDENTS = 1,    // Student profiles grid (or detail)
    PAGE_FLAGGED  = 2     // Flagged pairs
};

static const int TAB_BAR_HEIGHT = 40;   // spec §5.6: segmented control, 40px
static const int CONTEXT_STRIP_HEIGHT = 32; // spec §5.6: sticky run metadata strip

// Tab regions for hit-testing
struct TabRect {
    int x, y, w, h;
    ResultsPage page;
};

enum ClassesSortMode { SORT_NAME, SORT_SUBMISSIONS, SORT_FLAGGED };

struct FlaggedAppearanceRowRect { RECT r; int flaggedPairIndex; };

static const int BLOCK_TAB_BAR_HEIGHT = 46;

struct BlockTabRect { int x, y, w, h; int tabIdx; };

struct UnclassifiedActionRect { int x, y, w, h; int fileIdx; bool isDelete; };

struct EmptyCardRect { int x, y, w, h; int cmdId; };

struct StudentCardRect {
    int x, y, w, h;
    int studentIdx;
};

enum PairSeverityFilter { SEV_FILTER_ALL, SEV_FILTER_HIGH, SEV_FILTER_MODERATE };

struct SeverityChipRect { int x, y, w, h; PairSeverityFilter filter; };

struct PairAnimEntry { DWORD startTick; bool opening; }; // opening: animating toward expanded; false = toward collapsed

struct CollapsedPairRowRect { RECT r; int pairIdx; };

struct ExpandedPairHeaderRect { RECT r; int pairIdx; };

struct ShowAllFeaturesRect { RECT r; int key; };

struct CopyFindingsBtnRect { RECT r; int pairIdx; };


struct DnaStripStyle {
    int      cellW;         // width of one cell
    int      cellGap;       // gap between cells within a group
    int      groupGap;      // gap between the three groups
    int      height;        // full track height
    COLORREF baseColor;     // fill for normal cells
    COLORREF deviantColor;  // fill for flagged cells
};

struct DnaCellHit { RECT r; std::wstring name; double value; };

struct AISummaryLinkRect { RECT r; int pairIndex; };

// ─────────────────────────────────────────────────────────────
// FROM main.cpp — analysis pipeline and the global results object.
// Declarations only; all four are DEFINED in main.cpp, which does not
// include this header. Moved out of gui.cpp because every screen file
// from here on needs g_analysisResults, and the analysis-thread and
// student-report screens will need the other three.
//
// AnalysisPhase / AnalysisProgressCallback come from results_data.h
// (already included above) — declared once there so both main.cpp and
// the GUI see the identical enum rather than two copies that could
// drift apart.
// ─────────────────────────────────────────────────────────────
extern std::string runAnalysisPipeline(const std::string& dataFolder,
                                       const std::string& outputFile,
                                       bool useAI);
extern std::string generateStudentSimilarityReport(const std::string& studentName);
extern AnalysisResults g_analysisResults;
extern void setAnalysisProgressCallback(AnalysisProgressCallback cb);

// ─────────────────────────────────────────────────────────────
// SHARED GLOBALS — declared here, DEFINED ONCE in gui_common.cpp
// ─────────────────────────────────────────────────────────────
extern HFONT g_hFontDisplay;
extern HFONT g_hFontH1;
extern HFONT g_hFontH2;
extern HFONT g_hFontBodyNew;
extern HFONT g_hFontLabel;
extern HFONT g_hFontMono;
extern HFONT g_hFontMonoSm;
extern HFONT g_hFontStat;
extern HFONT g_hFontTitle;
extern HFONT g_hFontHeading;
extern HFONT g_hFontBody;
extern HFONT g_hFontSmall;
extern HFONT g_hFontButton;
extern HWND g_hMainWindow;
extern HWND g_hContent;
extern AppState g_state;
extern double g_dpiScale;
extern int g_contentFadeAlpha;
extern std::atomic<bool> g_analysisCancelRequested;
extern int g_scrollY;
extern int g_viewportH;
extern std::vector<FocusableItem> g_contentFocusables;
extern bool g_focusVisible;
extern ResultsPage g_currentPage;
extern int g_selectedStudent;
extern int g_currentBlockTab;
extern std::wstring g_searchFilterLower;
extern AppModal g_appModal;
extern HelpCarouselState g_helpCarouselState;
extern int g_helpCarouselStep;
extern bool g_helpCarouselFocusOnNext;
extern RECT g_helpCarouselCloseRect;
extern RECT g_helpCarouselPrevRect;
extern RECT g_helpCarouselNextRect;
extern RECT g_helpCarouselSkipRect;
extern RECT g_helpCarouselDotRects[HELP_STEP_COUNT];
extern HWND g_hSyncSheet;
extern HWND g_hSyncTree;
extern int g_syncFocusedIndex;
extern std::vector<FocusableItem> g_syncSheetFocusables;
extern std::vector<AISummaryLinkRect> g_aiSummaryLinkRects;
extern std::vector<BlockTabRect> g_blockTabRects;
extern RECT g_cancelAnalysisBtnRect;
extern ClassesSortMode g_classesSortMode;
extern RECT g_clearFilterButtonRect;
extern std::vector<CollapsedPairRowRect> g_collapsedPairRowRects;
extern std::vector<CopyFindingsBtnRect> g_copyFindingsBtnRects;
extern std::vector<DnaCellHit> g_dnaCellHits;
extern int g_dnaHoveredCell;
extern std::vector<EmptyCardRect> g_emptyCardRects;
extern RECT g_exactDuplicatesCardRect;
extern std::vector<ExpandedPairHeaderRect> g_expandedPairHeaderRects;
extern bool g_filterExactDuplicatesOnly;
extern std::vector<FlaggedAppearanceRowRect> g_flaggedAppearanceRowRects;
extern int g_hoveredEmptyCard;
extern bool g_langNoticeExpanded;
extern RECT g_langNoticeToggleRect;
extern std::map<int, PairAnimEntry> g_pairAnimating;
extern RECT g_reportButtonRect;
extern std::vector<SeverityChipRect> g_severityChipRects;
extern std::vector<ShowAllFeaturesRect> g_showAllFeaturesRects;
extern RECT g_sortControlRect;
extern bool g_statsAnimActive;
extern DWORD g_statsAnimStartTick;
extern std::vector<StudentCardRect> g_studentCardRects;
extern std::vector<UnclassifiedActionRect> g_unclassifiedActionRects;

// ─────────────────────────────────────────────────────────────
// SHARED INLINE HELPERS
// Defined in the header (not gui_common.cpp) because they were
// `static inline` before the split: keeping them inline preserves
// the original codegen at the ~50 call sites per paint.
// ─────────────────────────────────────────────────────────────
inline int S(int designPx) {
    return (int)std::lround(designPx * g_dpiScale);
}

inline float easeOutCubic(float t) {
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

// ─────────────────────────────────────────────────────────────
// SHARED FUNCTIONS
// ─────────────────────────────────────────────────────────────
void addFocusable(std::vector<FocusableItem>& region, FocusKind kind,
                          const RECT& r, int dataIndex, bool inContent);
RECT childRectInParent(HWND child, HWND parent);
int countCFiles(const std::wstring& folder);
int dnaStripWidth(const DnaStripStyle& st);
DnaStripStyle dnaStyleCard();
void drawCard(HDC hdc, const RECT& r, COLORREF bg, COLORREF border,
              int radius = 10, COLORREF accentLeft = 0);
void drawDnaStrip(HDC hdc, int x, int y,
                           const std::vector<double>& values,
                           const std::vector<bool>* deviant,
                           const DnaStripStyle& st,
                           bool trackHits = false,
                           bool valuesAreRawIndexed = true);
int drawEyebrow(HDC hdc, int x, int y, int width, const wchar_t* text);
void drawFocusRing(HDC hdcScreen, const RECT& r);
std::wstring fmtPct(double v);
GraphicsPath* makeRoundRectPath(float x, float y, float w, float h, float r);
void onRunAnalysis(HWND hwnd);
COLORREF pairSeverityColor(double combinedScorePct);
const wchar_t* pairSeverityLabel(double combinedScorePct);
std::wstring s2w(const std::string& s);
void showAppModal(const std::wstring& title, const std::wstring& body,
                          ModalKind kind, COLORREF accent,
                          const std::wstring& primaryLabel,
                          const std::wstring& secondaryLabel = L"",
                          std::function<void(bool)> onResult = nullptr);
void showToast(const std::wstring& text, ToastKind kind);
std::string toNarrowGC(const std::wstring& w);
std::wstring truncateMiddle(HDC hdc, const std::wstring& text, int maxWidth);
void updateUI();
std::string wideToUtf8(const std::wstring& w);
std::string extractPeriodGC(const std::string& filename);
std::string extractStudentNameGC(const std::string& filename);
void handleUnclassifiedAction(HWND hwnd, int fileIdx, bool isDelete);
std::wstring periodDisplayLabelGC(const std::string& period);
void closeStudentDetail(HWND hwnd);
int drawBlockTabBar(HDC hdc, int x, int y, int width);
int drawStudentDetailPage(HDC hdc, int x, int y, int width,
                                   int studentIdx);
int drawStudentProfiles(HDC hdc, int x, int y, int width);
void generateAndOpenStudentReport();
void openStudentDetail(HWND hwnd, int studentIdx);
void selectBlockTab(HWND hwnd, int tabIdx);
void clearExactDuplicatesFilter(HWND hwnd);
void copyFindingsForPair(HWND hwnd, int pairIdx);
int drawFlaggedPairs(HDC hdc, int x, int y, int width);
bool exportPairsToCsv(HWND hwnd);
void jumpToFlaggedPair(HWND hwnd, int pairIndex);
void selectSeverityFilter(HWND hwnd, PairSeverityFilter f);
void togglePairExpanded(HWND hwnd, int pairIdx);
void toggleShowAllFeatures(HWND hwnd, int key);
void closeHelpCarousel();
void drawHelpCarousel(HDC hdcScreen, const RECT& viewport);
void helpCarouselGoToStep(int step, int dir);
void helpCarouselNext();
void helpCarouselPrev();
void helpCarouselSkip();
void openHelpCarousel();
int drawAISummary(HDC hdc, int x, int y, int width);
int drawStatsDashboard(HDC hdc, int x, int y, int width);
void activateSyncFocused();
void onGoogleClassroom(HWND hwnd);
void syncFocusMove(int delta);
int drawWelcomeScreen(HDC hdc, int x, int y, int width);
void showSplashScreen(HINSTANCE hInst);
// Started by onRunAnalysis() (gui.cpp) via std::thread; the body and all
// the analysis-progress state it drives live in gui_welcome.cpp.
void analysisThread(std::wstring folder, bool useAI);

#endif // _WIN32
#endif // GUI_COMMON_H