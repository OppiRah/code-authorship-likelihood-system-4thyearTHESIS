// ─────────────────────────────────────────────────────────────
// gui_welcome.cpp
// Welcome / idle screen, the splash screen, and the background
// analysis thread with the live phase-progress state it drives.
//
// Split out of gui.cpp unchanged. Grouped here because the welcome
// canvas IS the analysis-progress display: drawWelcomeScreen renders
// the phase list from g_analysisPhase/Current/Total, which only
// analysisThread() writes. Keeping the producer and the consumer in
// one file lets all three of those globals stay private.
//
// Assembled from five separate regions of gui.cpp; order within each
// is unchanged. getULLoadingLogo, utf8ToWide, analysisPhaseLabel and
// drawSourceIcon came along because this file is their only caller.
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"

#ifdef _WIN32

// Loading-screen seal (ulcclogoloadingscreen.png, shipped next to the
// .exe) — a separate asset from the in-app seal above, used only by
// showSplashScreen(). Same load-once/fallback pattern as getULLogo():
// returns nullptr if the file isn't found so the splash screen falls
// back to the drawn gold circle.
static Image* getULLoadingLogo() {
    static Image* s_logoImg = nullptr;
    static bool s_logoTried = false;
    if (!s_logoTried) {
        s_logoTried = true;
        s_logoImg = new Image(L"ulcclogoloadingscreen.png");
        if (s_logoImg->GetLastStatus() != Ok) {
            delete s_logoImg;
            s_logoImg = nullptr;
        }
    }
    return s_logoImg;
}

static std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(sz - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], sz);
    return w;
}

// ═════════════════════════════════════════════════════════════
// ANALYSIS PROGRESS STATE (spec §5.5)
// ═════════════════════════════════════════════════════════════
// Updated live by the AnalysisProgressCallback registered in
// analysisThread() below, via real checkpoints inside
// runAnalysisPipeline() — not simulated Sleep()-based fake progress.
static AnalysisPhase g_analysisPhase        = AnalysisPhase::LoadingFiles;
static int           g_analysisPhaseCurrent = 0;
static int           g_analysisPhaseTotal   = 0;
std::atomic<bool> g_analysisCancelRequested { false };

static const wchar_t* analysisPhaseLabel(AnalysisPhase p) {
    switch (p) {
        case AnalysisPhase::LoadingFiles:      return L"Reading files";
        case AnalysisPhase::BuildingProfiles:  return L"Building style profiles";
        case AnalysisPhase::ComparingPairs:    return L"Comparing pairs";
        case AnalysisPhase::GeneratingSummary: return L"Generating AI summary";
    }
    return L"";
}

// ═════════════════════════════════════════════════════════════
// ANALYSIS THREAD
// Runs in background to avoid blocking UI
// ═════════════════════════════════════════════════════════════
void analysisThread(std::wstring folder, bool useAI) {
    g_analysisCancelRequested = false;
    g_analysisPhase        = AnalysisPhase::LoadingFiles;
    g_analysisPhaseCurrent = 0;
    g_analysisPhaseTotal   = 0;

    // Real progress, not simulated: this callback is invoked from
    // genuine checkpoints inside runAnalysisPipeline() (file count
    // after load, per-file during profile building, per-pair during
    // comparison with a real precomputed total, before/after the AI
    // call). Returning false requests cancellation, checked at the
    // pairwise-comparison loop.
    setAnalysisProgressCallback(
        [](AnalysisPhase phase, int current, int total) -> bool {
            g_analysisPhase        = phase;
            g_analysisPhaseCurrent = current;
            g_analysisPhaseTotal   = total;
            PostMessage(g_hMainWindow, WM_ANALYSIS_PROGRESS, 0, 0);
            return !g_analysisCancelRequested.load();
        });

    std::string folderUtf8 = wideToUtf8(folder);
    std::string outputFile = "report.html";

    std::string resultMsg = runAnalysisPipeline(folderUtf8, outputFile, useAI);

    setAnalysisProgressCallback(nullptr); // detach before this thread exits

    g_state.lastReport  = utf8ToWide(outputFile);
    g_state.progressMsg = utf8ToWide(resultMsg);

    PostMessage(g_hMainWindow, WM_ANALYSIS_COMPLETE, 0, 0);
}

// Draw the welcome/idle screen
// Vector icons for the empty-state cards. Drawn as paths rather than
// glyphs — Segoe MDL2 / emoji fonts aren't guaranteed on Win10, and a
// missing glyph renders as a tofu box.
static void drawSourceIcon(HDC hdc, int cx, int cy, int kind, COLORREF color) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    Pen pen(Color(255, GetRValue(color), GetGValue(color), GetBValue(color)), 1.8f);

    if (kind == 0) {          // folder
        Point pts[] = {
            Point(cx-S(11), cy+S(8)), Point(cx-S(11), cy-S(7)), Point(cx-S(4), cy-S(7)),
            Point(cx-S(2), cy-S(4)),  Point(cx+S(11), cy-S(4)), Point(cx+S(11), cy+S(8))
        };
        g.DrawPolygon(&pen, pts, 6);
    } else if (kind == 1) {   // document
        Point pts[] = {
            Point(cx-S(8), cy-S(10)), Point(cx+S(3), cy-S(10)), Point(cx+S(8), cy-S(5)),
            Point(cx+S(8), cy+S(10)), Point(cx-S(8), cy+S(10))
        };
        g.DrawPolygon(&pen, pts, 5);
        g.DrawLine(&pen, cx+S(3), cy-S(10), cx+S(3), cy-S(5));
        g.DrawLine(&pen, cx+S(3), cy-S(5),  cx+S(8), cy-S(5));
    } else {                  // cloud / sync
        g.DrawArc(&pen, cx-S(11), cy-S(4), S(12), S(12), 100.0f, 240.0f);
        g.DrawArc(&pen, cx-S(3),  cy-S(9), S(14), S(14), 200.0f, 250.0f);
        g.DrawLine(&pen, cx-S(5), cy+S(8), cx+S(9), cy+S(8));
    }
}

int drawWelcomeScreen(HDC hdc, int x, int y, int width) {
    int startY = y;
    SetBkMode(hdc, TRANSPARENT);
    g_emptyCardRects.clear();

    // ── Analysis in progress — full-canvas phase display (spec §5.5) ──
    // Driven by REAL checkpoints from runAnalysisPipeline() via the
    // callback registered in analysisThread(), not simulated Sleep()
    // progress. Completed phases show gray-500 with a check; the
    // active phase is gold-500.
    if (g_state.analysisRunning) {
        HFONT oldF = (HFONT)SelectObject(hdc, g_hFontH1);
        SetTextColor(hdc, WHITE_);
        RECT t = {x, y + S(90), x + width, y + S(130)};
        DrawTextW(hdc, L"Analyzing submissions", -1, &t,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldF);

        // Phase chain: "Reading files → Building style profiles →
        // Comparing pairs → Generating AI summary"
        const AnalysisPhase phases[4] = {
            AnalysisPhase::LoadingFiles, AnalysisPhase::BuildingProfiles,
            AnalysisPhase::ComparingPairs, AnalysisPhase::GeneratingSummary
        };
        SelectObject(hdc, g_hFontBodyNew);
        SIZE arrowSz; GetTextExtentPoint32W(hdc, L"  \u2192  ", 5, &arrowSz);

        int chainY = y + S(144);
        // Measure total width first so the chain can be centered
        int totalChainW = 0;
        SIZE sz;
        for (int i = 0; i < 4; ++i) {
            GetTextExtentPoint32W(hdc, analysisPhaseLabel(phases[i]),
                                  (int)wcslen(analysisPhaseLabel(phases[i])), &sz);
            totalChainW += sz.cx;
            if (i < 3) totalChainW += arrowSz.cx;
        }
        int chainX = x + (width - totalChainW) / 2;

        int activeIdx = (int)g_analysisPhase;
        for (int i = 0; i < 4; ++i) {
            bool completed = i < activeIdx;
            bool active    = i == activeIdx;
            SetTextColor(hdc, active ? GOLD_500 : (completed ? GRAY_500 : GRAY_700));

            std::wstring label = completed
                ? (std::wstring(L"\u2713 ") + analysisPhaseLabel(phases[i]))
                : analysisPhaseLabel(phases[i]);

            GetTextExtentPoint32W(hdc, label.c_str(), (int)label.length(), &sz);
            RECT lr = {chainX, chainY, chainX + sz.cx, chainY + S(20)};
            DrawTextW(hdc, label.c_str(), -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            chainX += sz.cx;

            if (i < 3) {
                SetTextColor(hdc, GRAY_700);
                RECT ar = {chainX, chainY, chainX + arrowSz.cx, chainY + S(20)};
                DrawTextW(hdc, L"  \u2192  ", -1, &ar, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                chainX += arrowSz.cx;
            }
        }

        // Determinate bar — real percentage from the active phase's
        // current/total, not a fake animation.
        int barX = x + width / 2 - S(200), barY = y + S(184);
        int barW = S(400), barH = S(6);
        RECT track = {barX, barY, barX + barW, barY + barH};
        HBRUSH tBr = CreateSolidBrush(GRAY_800);
        FillRect(hdc, &track, tBr); DeleteObject(tBr);

        int pct = (g_analysisPhaseTotal > 0)
            ? (g_analysisPhaseCurrent * 100 / g_analysisPhaseTotal) : 0;
        int fillW = (barW * pct) / 100;
        if (fillW > 0) {
            RECT fill = {barX, barY, barX + fillW, barY + barH};
            HBRUSH fBr = CreateSolidBrush(GOLD_500);
            FillRect(hdc, &fill, fBr); DeleteObject(fBr);
        }

        // "Comparing pairs · 48 of 78"
        SetTextColor(hdc, GRAY_400);
        RECT curR = {x, barY + S(18), x + width, barY + S(40)};
        std::wstring curTxt = analysisPhaseLabel(g_analysisPhase);
        if (g_analysisPhaseTotal > 0) {
            curTxt += L"  \u00B7  " + std::to_wstring(g_analysisPhaseCurrent) +
                      L" of " + std::to_wstring(g_analysisPhaseTotal);
        }
        DrawTextW(hdc, curTxt.c_str(), -1, &curR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Cancel — cooperative: sets a flag the pairwise loop checks
        // at its next checkpoint. Not instantaneous, but bounded to
        // roughly one comparison's worth of latency.
        RECT cancelR = {x + width / 2 - S(60), barY + S(56), x + width / 2 + S(60), barY + S(84)};
        g_cancelAnalysisBtnRect.left   = cancelR.left;
        g_cancelAnalysisBtnRect.top    = cancelR.top + g_scrollY;
        g_cancelAnalysisBtnRect.right  = cancelR.right;
        g_cancelAnalysisBtnRect.bottom = cancelR.bottom + g_scrollY;
        addFocusable(g_contentFocusables, FocusKind::CancelAnalysisBtn,
                     g_cancelAnalysisBtnRect, 0, true);
        drawCard(hdc, cancelR, GRAY_850, GRAY_700, 6);
        SetTextColor(hdc, GRAY_200);
        DrawTextW(hdc, L"Cancel analysis", -1, &cancelR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        return S(420);
    }

    // ── Data loaded, ready to analyze ─────────────────────────
    if (g_state.hasData) {
        HFONT oldF = (HFONT)SelectObject(hdc, g_hFontH1);
        SetTextColor(hdc, WHITE_);
        RECT t = {x, y + S(150), x + width, y + S(190)};
        std::wstring msg = std::to_wstring(g_state.fileCount) +
                           L" submissions ready to analyze";
        DrawTextW(hdc, msg.c_str(), -1, &t, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_400);
        RECT s = {x, y + S(196), x + width, y + S(218)};
        DrawTextW(hdc, L"Run analysis from the sidebar to begin.", -1, &s,
                  DT_CENTER | DT_SINGLELINE);
        SelectObject(hdc, oldF);
        return S(400);
    }

    // ── Empty state (spec §5.3) ───────────────────────────────
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontDisplay);
    SetTextColor(hdc, WHITE_);
    RECT titleRect = {x, y + S(88), x + width, y + S(132)};
    DrawTextW(hdc, L"Load student submissions to begin", -1, &titleRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_400);
    RECT subRect = {x, y + S(138), x + width, y + S(162)};
    DrawTextW(hdc,
        L"CALSS builds a style profile for each student, then checks whether "
        L"new submissions match it.",
        -1, &subRect, DT_CENTER | DT_SINGLELINE);

    // Three option cards
    struct Opt { const wchar_t* title; const wchar_t* desc; int cmd; int icon; };
    const Opt opts[3] = {
        { L"Select a folder",  L"Scan a directory of .c submissions", ID_BTN_SELECT_FOLDER, 0 },
        { L"Import .c files",  L"Pick individual files by hand",      ID_BTN_IMPORT_FILES,  1 },
        { L"Sync Classroom",   L"Pull submissions per class block",   ID_BTN_GCLASSROOM,    2 },
    };

    const int cardW = S(232), cardH = S(156), gap = S(16);
    int totalW = cardW * 3 + gap * 2;
    int cardX  = x + (width - totalW) / 2;
    int cardY  = y + S(196);

    for (int i = 0; i < 3; ++i) {
        bool hov = (g_hoveredEmptyCard == i);
        RECT c = {cardX, cardY, cardX + cardW, cardY + cardH};

        drawCard(hdc, c, hov ? GRAY_800 : GRAY_850,
                 hov ? GOLD_400 : GRAY_700, 10);

        drawSourceIcon(hdc, cardX + cardW / 2, cardY + S(44),
                       opts[i].icon, hov ? GOLD_400 : GRAY_400);

        SelectObject(hdc, g_hFontH2);
        SetTextColor(hdc, WHITE_);
        RECT tr = {cardX + S(14), cardY + S(74), cardX + cardW - S(14), cardY + S(100)};
        DrawTextW(hdc, opts[i].title, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_400);
        RECT dr = {cardX + S(16), cardY + S(102), cardX + cardW - S(16), cardY + S(142)};
        DrawTextW(hdc, opts[i].desc, -1, &dr,
                  DT_CENTER | DT_WORDBREAK);

        EmptyCardRect hit { cardX, cardY + g_scrollY, cardW, cardH, opts[i].cmd };
        g_emptyCardRects.push_back(hit);
        addFocusable(g_contentFocusables, FocusKind::EmptyStateCard,
                     RECT{hit.x, hit.y, hit.x + hit.w, hit.y + hit.h},
                     hit.cmdId, true);

        cardX += cardW + gap;
    }

    // Ethical frame — required on any surface, not just report footers
    SelectObject(hdc, g_hFontBodyNew);
    SetTextColor(hdc, GRAY_500);
    RECT footerRect = {x, cardY + cardH + S(40), x + width, cardY + cardH + S(64)};
    DrawTextW(hdc,
        L"Statistical evidence only  \u00B7  determinations rest with the instructor",
        -1, &footerRect, DT_CENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    return (cardY + cardH + S(90)) - startY;
}

void showSplashScreen(HINSTANCE hInst) {
    const int SW = 480, SH = 300;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW sc = {};
    sc.cbSize       = sizeof(sc);
    sc.lpfnWndProc  = [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
        // Offscreen buffer, cached across repaints — created once on first
        // paint and reused for every frame of the loading-bar animation
        // rather than allocated/freed ~50 times in under a second (which
        // was what made the animation feel slow after double-buffering
        // was added). Safe to cache because this window's size is fixed
        // (SW×SH, never resized). Torn down on WM_DESTROY.
        static HDC     s_hdc    = nullptr;
        static HBITMAP s_bmp    = nullptr;
        static HBITMAP s_oldBmp = nullptr;

        if (m == WM_DESTROY) {
            if (s_hdc) {
                SelectObject(s_hdc, s_oldBmp);
                DeleteObject(s_bmp);
                DeleteDC(s_hdc);
                s_hdc = nullptr; s_bmp = nullptr; s_oldBmp = nullptr;
            }
        }
        if (m == WM_ERASEBKGND) return 1; // suppress default erase — we fill the whole
                                            // client rect ourselves in the offscreen buffer,
                                            // same convention as every other WM_PAINT in
                                            // this file (see TitleBarProc/SidebarProc/
                                            // ContentProc/SyncSheetProc).
        if (m == WM_PAINT) {
            PAINTSTRUCT ps;
            HDC hdcScreen = BeginPaint(h, &ps);
            RECT r; GetClientRect(h, &r);

            // First paint: allocate the buffer once and keep it for every
            // subsequent frame of the animation.
            if (!s_hdc) {
                s_hdc = CreateCompatibleDC(hdcScreen);
                s_bmp = CreateCompatibleBitmap(hdcScreen, r.right, r.bottom);
                s_oldBmp = (HBITMAP)SelectObject(s_hdc, s_bmp);
            }
            HDC hdc = s_hdc;

            // Background
            HBRUSH bg = CreateSolidBrush(RGB(30, 32, 38));
            FillRect(hdc, &r, bg); DeleteObject(bg);

            SetBkMode(hdc, TRANSPARENT);

            // Gold top border
            HBRUSH topBr = CreateSolidBrush(SPLASH_GOLD);
            RECT topLine = {0, 0, r.right, 4};
            FillRect(hdc, &topLine, topBr); DeleteObject(topBr);

            // UL Seal
            int cx = r.right / 2, cy = 105, sr = 52;
            Image* splashLogo = getULLoadingLogo();
            if (splashLogo) {
                Graphics g(hdc);
                g.SetSmoothingMode(SmoothingModeAntiAlias);
                int sr2 = (int)(sr * 1.2); // ~20% larger, same as the sidebar seal
                g.DrawImage(splashLogo, cx - sr2, cy - sr2, sr2 * 2, sr2 * 2);
            } else {
                HPEN gp  = CreatePen(PS_SOLID, 3, SPLASH_GOLD);
                HBRUSH sb = CreateSolidBrush(RGB(85, 20, 28));
                HPEN op  = (HPEN)SelectObject(hdc, gp);
                HBRUSH ob = (HBRUSH)SelectObject(hdc, sb);
                Ellipse(hdc, cx-sr, cy-sr, cx+sr, cy+sr);
                SelectObject(hdc, op); SelectObject(hdc, ob);
                DeleteObject(gp); DeleteObject(sb);

                // Inner ring
                HPEN ip = CreatePen(PS_SOLID, 1, RGB(160, 130, 40));
                HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH);
                HPEN oi = (HPEN)SelectObject(hdc, ip);
                HBRUSH oni = (HBRUSH)SelectObject(hdc, nb);
                int ir = sr - 9;
                Ellipse(hdc, cx-ir, cy-ir, cx+ir, cy+ir);
                SelectObject(hdc, oi); SelectObject(hdc, oni);
                DeleteObject(ip);

                // "UL" text in seal
                HFONT f1 = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                HFONT of1 = (HFONT)SelectObject(hdc, f1);
                SetTextColor(hdc, SPLASH_GOLD);
                RECT sealTxt = {cx-sr, cy-sr, cx+sr, cy+sr};
                DrawTextW(hdc, L"UL", -1, &sealTxt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                SelectObject(hdc, of1); DeleteObject(f1);
            }

            // App name
            HFONT f2 = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            HFONT of2 = (HFONT)SelectObject(hdc, f2);
            SetTextColor(hdc, RGB(245, 242, 235));
            RECT nameR = {20, cy + sr + 12, r.right - 20, cy + sr + 44};
            DrawTextW(hdc, L"Code Authorship Likelihood System",
                      -1, &nameR, DT_CENTER | DT_SINGLELINE);
            SelectObject(hdc, of2); DeleteObject(f2);

            // Subtitle
            HFONT f3 = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            HFONT of3 = (HFONT)SelectObject(hdc, f3);
            SetTextColor(hdc, RGB(154, 154, 160));
            RECT subR = {20, cy + sr + 48, r.right - 20, cy + sr + 66};
            DrawTextW(hdc, L"University of Luzon  \u00B7  College of Computer Studies",
                      -1, &subR, DT_CENTER | DT_SINGLELINE);

            // Loading bar background
            int bx = 60, by = r.bottom - 42, bw = r.right - 120, bh = 4;
            RECT barBg = {bx, by, bx+bw, by+bh};
            HBRUSH bbg = CreateSolidBrush(RGB(50, 53, 62));
            FillRect(hdc, &barBg, bbg); DeleteObject(bbg);

            // Animated loading fill (position stored in window data)
            int progress = (int)GetWindowLongPtr(h, GWLP_USERDATA);
            int fillW = (bw * progress) / 100;
            if (fillW > 0) {
                RECT barFill = {bx, by, bx + fillW, by + bh};
                HBRUSH bf = CreateSolidBrush(SPLASH_GOLD);
                FillRect(hdc, &barFill, bf); DeleteObject(bf);
            }

            // "Loading..." text
            RECT loadR = {20, r.bottom - 28, r.right - 20, r.bottom - 10};
            SetTextColor(hdc, RGB(100, 100, 110));
            DrawTextW(hdc, L"Loading...", -1, &loadR, DT_CENTER | DT_SINGLELINE);
            SelectObject(hdc, of3); DeleteObject(f3);

            BitBlt(hdcScreen, 0, 0, r.right, r.bottom, hdc, 0, 0, SRCCOPY);

            EndPaint(h, &ps);
        }
        if (m == WM_TIMER) InvalidateRect(h, nullptr, FALSE);
        return DefWindowProcW(h, m, w, l);
    };
    sc.hInstance    = hInst;
    sc.lpszClassName = L"CALSSSplash";
    sc.hCursor      = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&sc);

    HWND splash = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"CALSSSplash", L"CALSS",
        WS_POPUP | WS_VISIBLE,
        (screenW - SW) / 2, (screenH - SH) / 2, SW, SH,
        nullptr, nullptr, hInst, nullptr);

    // Animate the loading bar over ~1.8 seconds
    SetTimer(splash, 1, 18, nullptr);
    int prog = 0;
    MSG msg = {};
    while (prog <= 100) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        SetWindowLongPtr(splash, GWLP_USERDATA, prog);
        InvalidateRect(splash, nullptr, FALSE);
        UpdateWindow(splash);
        Sleep(18);
        prog += 2;
    }

    KillTimer(splash, 1);
    DestroyWindow(splash);
}

#endif // _WIN32