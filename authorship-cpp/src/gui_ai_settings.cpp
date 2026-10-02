// ─────────────────────────────────────────────────────────────
// gui_ai_settings.cpp
// "AI settings" dialog — where each installation enters its OWN Gemini
// API key (instead of a shared key shipping next to the exe). The key
// is stored in Windows Credential Manager for the current Windows user
// (see secret_store.h / gemini.h), never in a file.
//
// Same lightweight native-dialog pattern as gui_assign_dialog.cpp:
// plain system controls, its own small modal loop.
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"
#include "../include/secret_store.h"

#ifdef _WIN32

// ── Cached "is a key saved?" (read once, refreshed on change) ─
static int g_aiKeyState = -1;  // -1 unknown, 0 none, 1 saved

bool aiKeyConfigured() {
    if (g_aiKeyState < 0) g_aiKeyState = gemini_hasKey() ? 1 : 0;
    return g_aiKeyState == 1;
}

void invalidateAiKeyState() { g_aiKeyState = -1; }

// ── Dialog ────────────────────────────────────────────────────
enum {
    ID_AI_EDIT = 1301, ID_AI_SHOW, ID_AI_LINK, ID_AI_TEST, ID_AI_SAVE,
    ID_AI_REMOVE, ID_AI_CANCEL
};

static HWND     g_aiDlg = nullptr, g_aiEdit = nullptr, g_aiStatus = nullptr,
                g_aiLink = nullptr, g_aiSavedLabel = nullptr;
static bool     g_aiDlgOpen = false;
static bool     g_aiChanged = false;
static bool     g_aiClassRegistered = false;
static COLORREF g_aiStatusColor = RGB(60, 60, 60);
static HFONT    g_aiLinkFont = nullptr;

static const wchar_t* AI_KEY_URL = L"https://aistudio.google.com/apikey";

static std::string aiEditText() {
    int n = GetWindowTextLengthW(g_aiEdit);
    std::wstring w(n, L'\0');
    if (n > 0) GetWindowTextW(g_aiEdit, &w[0], n + 1);
    std::string s = toNarrowGC(w);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
        s.pop_back();
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    return s.substr(i);
}

static void aiSetStatus(const std::wstring& text, COLORREF color) {
    g_aiStatusColor = color;
    SetWindowTextW(g_aiStatus, text.c_str());
    InvalidateRect(g_aiStatus, nullptr, TRUE);
    UpdateWindow(g_aiStatus);
}

static void aiCloseDialog(HWND hwnd) {
    g_aiDlgOpen = false;
    DestroyWindow(hwnd);
}

static LRESULT CALLBACK AiSettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CTLCOLORSTATIC: {
            HDC dc = (HDC)wParam;
            HWND ctl = (HWND)lParam;
            SetBkMode(dc, TRANSPARENT);
            if (ctl == g_aiLink)   SetTextColor(dc, RGB(0, 102, 204));
            if (ctl == g_aiStatus) SetTextColor(dc, g_aiStatusColor);
            return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
        }
        case WM_SETCURSOR:
            if ((HWND)wParam == g_aiLink) {
                SetCursor(LoadCursor(nullptr, IDC_HAND));
                return TRUE;
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_AI_LINK && HIWORD(wParam) == STN_CLICKED) {
                ShellExecuteW(nullptr, L"open", AI_KEY_URL, nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            }
            if (id == ID_AI_SHOW) {
                bool show = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;
                SendMessageW(g_aiEdit, EM_SETPASSWORDCHAR, show ? 0 : (WPARAM)0x25CF, 0);
                InvalidateRect(g_aiEdit, nullptr, TRUE);
                return 0;
            }
            if (id == ID_AI_TEST) {
                std::string key = aiEditText();
                bool usingSaved = false;
                if (key.empty()) { key = loadApiKey(); usingSaved = true; }
                if (key.empty()) {
                    aiSetStatus(L"Paste a key into the box first.", RGB(160, 30, 30));
                    return 0;
                }
                aiSetStatus(L"Testing the key with Google…", RGB(60, 60, 60));
                HCURSOR old = SetCursor(LoadCursor(nullptr, IDC_WAIT));
                std::string err;
                bool ok = gemini_testKey(key, err);
                SetCursor(old);
                if (ok)
                    aiSetStatus(usingSaved ? L"✓ Your saved key works."
                                           : L"✓ This key works. Click Save to keep it.",
                                RGB(20, 120, 50));
                else
                    aiSetStatus(L"✗ " + s2w(err), RGB(160, 30, 30));
                return 0;
            }
            if (id == ID_AI_SAVE) {
                std::string key = aiEditText();
                if (key.empty()) {
                    aiSetStatus(L"Paste a key into the box first.", RGB(160, 30, 30));
                    return 0;
                }
                if (!gemini_saveKey(key)) {
                    // No file fallback — the key is simply not saved.
                    aiSetStatus(L"\u2717 Couldn't save to Windows Credential Manager "
                                L"(Windows error " + std::to_wstring(secretLastError()) +
                                L"). The key was not saved anywhere.",
                                RGB(160, 30, 30));
                    return 0;
                }
                g_aiChanged = true;
                aiCloseDialog(hwnd);
                return 0;
            }
            if (id == ID_AI_REMOVE) {
                gemini_removeKey();
                g_aiChanged = true;
                aiCloseDialog(hwnd);
                return 0;
            }
            if (id == ID_AI_CANCEL || id == IDCANCEL) {
                aiCloseDialog(hwnd);
                return 0;
            }
            return 0;
        }
        case WM_CLOSE:
            aiCloseDialog(hwnd);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// Returns true if the saved key was added, replaced or removed.
static bool showAiSettingsDialog(HWND parent) {
    g_aiChanged = false;
    HINSTANCE hInst = GetModuleHandle(nullptr);
    if (!g_aiClassRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = AiSettingsProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = L"CALSSAiSettingsDialog";
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        RegisterClassExW(&wc);
        g_aiClassRegistered = true;
    }

    const int DW = S(500), DH = S(390);
    RECT pr; GetWindowRect(parent ? parent : GetDesktopWindow(), &pr);
    int dx = pr.left + ((pr.right - pr.left) - DW) / 2;
    int dy = pr.top + ((pr.bottom - pr.top) - DH) / 2;

    g_aiDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"CALSSAiSettingsDialog",
        L"AI settings", WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dx, dy, DW, DH, parent, nullptr, hInst, nullptr);
    HWND d = g_aiDlg;
    HFONT f = g_hFontBodyNew;
    int pad = S(20), w = DW - S(56);
    auto mk = [&](const wchar_t* cls, const wchar_t* text, DWORD style,
                  int x, int y, int cw, int ch, int id) {
        HWND h = CreateWindowW(cls, text, WS_CHILD | WS_VISIBLE | style,
                               x, y, cw, ch, d, (HMENU)(INT_PTR)id, hInst, nullptr);
        SendMessageW(h, WM_SETFONT, (WPARAM)f, TRUE);
        return h;
    };

    mk(L"STATIC",
       L"CALSS can add AI-written summaries using Google Gemini. Paste your own "
       L"Gemini API key below. It is stored in Windows Credential Manager for "
       L"your Windows account only — never in a file.",
       0, pad, S(16), w, S(66), 0);

    g_aiLink = mk(L"STATIC", L"Get a free Gemini API key at aistudio.google.com/apikey",
                  SS_NOTIFY, pad, S(88), w, S(22), ID_AI_LINK);
    if (!g_aiLinkFont) {
        LOGFONTW lf = {};
        GetObjectW(f, sizeof(lf), &lf);
        lf.lfUnderline = TRUE;
        g_aiLinkFont = CreateFontIndirectW(&lf);
    }
    SendMessageW(g_aiLink, WM_SETFONT, (WPARAM)g_aiLinkFont, TRUE);

    // What's saved right now (never the key itself — just its last 4)
    std::string saved = loadApiKey();
    std::wstring savedText = saved.empty()
        ? L"No key is saved on this computer yet."
        : L"A key is saved on this computer (ending in …" +
          s2w(saved.size() > 4 ? saved.substr(saved.size() - 4) : saved) +
          L"). Paste a new one to replace it.";
    g_aiSavedLabel = mk(L"STATIC", savedText.c_str(), 0, pad, S(122), w, S(40), 0);

    mk(L"STATIC", L"Gemini API key:", 0, pad, S(168), w, S(22), 0);
    g_aiEdit = mk(L"EDIT", L"", WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | ES_PASSWORD,
                  pad, S(192), w, S(28), ID_AI_EDIT);
    SendMessageW(g_aiEdit, EM_SETPASSWORDCHAR, (WPARAM)0x25CF, 0);
    mk(L"BUTTON", L"Show key", WS_TABSTOP | BS_AUTOCHECKBOX, pad, S(226), S(140), S(22), ID_AI_SHOW);

    g_aiStatus = mk(L"STATIC", L"", 0, pad, S(254), w, S(44), 0);

    int by = DH - S(96), bh = S(32);
    mk(L"BUTTON", L"Test key", WS_TABSTOP, pad, by, S(100), bh, ID_AI_TEST);
    mk(L"BUTTON", L"Save", WS_TABSTOP | BS_DEFPUSHBUTTON, pad + S(110), by, S(90), bh, ID_AI_SAVE);
    HWND rm = mk(L"BUTTON", L"Remove saved key", WS_TABSTOP, pad + S(210), by, S(140), bh, ID_AI_REMOVE);
    EnableWindow(rm, !saved.empty());
    mk(L"BUTTON", L"Cancel", WS_TABSTOP, DW - S(36) - S(90), by, S(90), bh, ID_AI_CANCEL);
    SecureZeroMemory(&saved[0], saved.size());

    EnableWindow(g_hMainWindow, FALSE);
    ShowWindow(d, SW_SHOW);
    SetFocus(g_aiEdit);
    g_aiDlgOpen = true;

    MSG msg;
    while (g_aiDlgOpen && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(d, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(g_hMainWindow, TRUE);
    SetForegroundWindow(g_hMainWindow);
    g_aiDlg = nullptr;
    return g_aiChanged;
}

void openAiSettingsDialog(HWND parent) {
    bool hadKey = aiKeyConfigured();
    if (!showAiSettingsDialog(parent)) return;

    invalidateAiKeyState();
    bool hasKey = aiKeyConfigured();
    // Newly set up → turn AI on; removed → off and disabled.
    syncAiSummaryCheckbox(hasKey && !hadKey);
    audit_log(hasKey ? "AI KEY SAVED" : "AI KEY REMOVED",
              "Gemini API key stored in Windows Credential Manager (key not logged).");
    showToast(hasKey ? L"AI key saved — AI summary is on for your next analysis."
                     : L"AI key removed — AI summary is off.",
              hasKey ? TOAST_SUCCESS : TOAST_INFO);
    if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
}

// Startup: move any legacy plaintext key/token files into Credential
// Manager and tell the user exactly what happened (files deleted, or
// kept because they couldn't be imported). Split in two because the
// import has to happen before the first window is created — the
// sidebar asks aiKeyConfigured() while it's being built, and that path
// would import silently — while the notice needs the window to exist.
static LegacySecretMigration g_startupMigration;

void migrateLegacySecretsEarly() {
    g_startupMigration = migrateLegacySecretFiles();
    invalidateAiKeyState();
}

void showLegacySecretMigrationNotice() {
    const LegacySecretMigration& m = g_startupMigration;
    if (!m.anything()) return;

    std::wstring body;
    if (m.geminiMigrated)
        body += L"• Your Gemini API key was moved from api_key.txt into Windows "
                L"Credential Manager.\n";
    if (m.tokenMigrated)
        body += L"• Your Google Classroom sign-in was moved from "
                L"calss_gc_token.json into Windows Credential Manager.\n";
    if (!m.deletedFiles.empty()) {
        body += L"\nThese plain-text files were deleted:\n";
        for (const auto& p : m.deletedFiles) body += L"   " + p + L"\n";
    }
    if (!m.failedFiles.empty()) {
        body += L"\nThese files could NOT be imported and were left in place:\n";
        for (const auto& p : m.failedFiles) body += L"   " + p + L"\n";
    }
    body += L"\nNothing else changes — AI summaries and Classroom sync work as before.";

    std::string logBody;
    for (const auto& p : m.deletedFiles) logBody += "Deleted: " + toNarrowGC(p) + "\n";
    for (const auto& p : m.failedFiles)  logBody += "Kept (not imported): " + toNarrowGC(p) + "\n";
    audit_log("SECRETS MOVED TO CREDENTIAL MANAGER", logBody);

    showAppModal(L"Saved keys moved to Windows Credential Manager", body,
                 MODAL_OK, INFO_, L"OK");
}

#endif // _WIN32
