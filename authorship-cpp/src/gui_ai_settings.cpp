// ─────────────────────────────────────────────────────────────
// gui_ai_settings.cpp
// "AI settings" dialog — where each installation enters its OWN Gemini
// API key (instead of a shared key shipping next to the exe). The key
// is stored in Windows Credential Manager for the current Windows user
// (see secret_store.h / gemini.h), never in a file.
//
// Since 1.0.1 it also shows which Gemini model CALSS will use for this
// key (found with Google's models.list, see gemini.cpp) and lets the
// user override Automatic with one of the compatible models. Every
// check with Google runs on a worker thread and reports back with
// WM_AI_CHECK_DONE, so the dialog never freezes on a slow network.
//
// Same lightweight native-dialog pattern as gui_assign_dialog.cpp:
// plain system controls, its own small modal loop.
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"
#include "../include/secret_store.h"

#include <functional>
#include <memory>

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
    ID_AI_REMOVE, ID_AI_CANCEL, ID_AI_MODEL
};

static const UINT WM_AI_CHECK_DONE = WM_APP + 41;

static HWND     g_aiDlg = nullptr, g_aiEdit = nullptr, g_aiStatus = nullptr,
                g_aiLink = nullptr, g_aiSavedLabel = nullptr,
                g_aiModelCombo = nullptr, g_aiModelInfo = nullptr,
                g_aiBtnTest = nullptr, g_aiBtnSave = nullptr, g_aiBtnRemove = nullptr;
static bool     g_aiDlgOpen = false;
static bool     g_aiChanged = false;        // key added / replaced / removed
static bool     g_aiPrefChanged = false;    // model preference changed
static bool     g_aiNoModelWarning = false; // saved a valid key that has no usable model
static bool     g_aiClassRegistered = false;
static bool     g_aiHasSavedKey = false;
static COLORREF g_aiStatusColor = RGB(60, 60, 60);
static HFONT    g_aiLinkFont = nullptr;
// Key the user was told "couldn't check — Save again to keep it anyway"
// about. A second Save with the same text saves it unverified.
static std::string g_aiUnverifiedKey;
// Worker bookkeeping: only the newest check's result is used.
static int      g_aiJobId = 0;
static bool     g_aiBusy = false;
// Combo contents: index 0 = Automatic, index i = g_aiComboIds[i].
static std::vector<std::string> g_aiComboIds;
static std::string g_aiPrefAtOpen;

static const wchar_t* AI_KEY_URL = L"https://aistudio.google.com/apikey";

enum AiCheckPurpose { AI_CHECK_TEST, AI_CHECK_SAVE, AI_CHECK_OPEN };

// What the worker hands back. It never carries the key itself: Save
// re-reads the box and compares this hash, so no copy of the key can be
// left behind in a message that is never delivered.
struct AiCheckResult {
    int            jobId;
    AiCheckPurpose purpose;
    bool           usingSaved;
    size_t         keyHash;
    GeminiKeyCheck check;
};

static size_t aiHash(const std::string& k) { return std::hash<std::string>{}(k); }

static std::string aiEditText() {
    int n = GetWindowTextLengthW(g_aiEdit);
    std::wstring w(n, L'\0');
    if (n > 0) GetWindowTextW(g_aiEdit, &w[0], n + 1);
    std::string s = toNarrowGC(w);
    if (n > 0) SecureZeroMemory(&w[0], w.size() * sizeof(wchar_t));
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

// After a rejected key: put focus back in the box with the text
// selected, so the user can retype or paste over it straight away.
static void aiReselectKey() {
    SetFocus(g_aiEdit);
    SendMessageW(g_aiEdit, EM_SETSEL, 0, -1);
}

static void aiSetBusy(bool busy) {
    g_aiBusy = busy;
    EnableWindow(g_aiBtnTest, !busy);
    EnableWindow(g_aiBtnSave, !busy);
    EnableWindow(g_aiBtnRemove, !busy && g_aiHasSavedKey);
}

// ── Model dropdown ────────────────────────────────────────────
static std::string aiSelectedModel() {
    int i = (int)SendMessageW(g_aiModelCombo, CB_GETCURSEL, 0, 0);
    return (i > 0 && i < (int)g_aiComboIds.size()) ? g_aiComboIds[i] : "";
}

// Rebuilds the list: Automatic, then the compatible models (best first),
// plus the saved preference if this key doesn't offer it.
static void aiFillModels(const std::vector<GeminiModelInfo>& models, bool known) {
    std::string keep = g_aiComboIds.empty() ? g_aiPrefAtOpen : aiSelectedModel();
    SendMessageW(g_aiModelCombo, CB_RESETCONTENT, 0, 0);
    g_aiComboIds.assign(1, "");
    SendMessageW(g_aiModelCombo, CB_ADDSTRING, 0, (LPARAM)L"Automatic (recommended)");
    bool keepListed = keep.empty();
    for (const auto& m : models) {
        std::wstring label = s2w(m.displayName) + L"  (" + s2w(m.id) + L")";
        SendMessageW(g_aiModelCombo, CB_ADDSTRING, 0, (LPARAM)label.c_str());
        g_aiComboIds.push_back(m.id);
        if (m.id == keep) keepListed = true;
    }
    if (!keepListed) {
        std::wstring label = s2w(keep) + (known ? L"  (not available to this key)" : L"");
        SendMessageW(g_aiModelCombo, CB_ADDSTRING, 0, (LPARAM)label.c_str());
        g_aiComboIds.push_back(keep);
    }
    int sel = 0;
    for (size_t i = 1; i < g_aiComboIds.size(); ++i)
        if (g_aiComboIds[i] == keep) sel = (int)i;
    SendMessageW(g_aiModelCombo, CB_SETCURSEL, sel, 0);
}

static void aiShowModelInfo(const GeminiKeyCheck& c) {
    std::wstring t;
    if (!c.keyValid)
        t = L"Model: shown once Google accepts the key.";
    else if (c.autoModel.empty())
        t = L"✗ No compatible Gemini model is available to this key.";
    else {
        std::wstring name = s2w(c.autoModel);
        for (const auto& m : c.models)
            if (m.id == c.autoModel) name = s2w(m.displayName) + L" (" + s2w(m.id) + L")";
        t = L"✓ Compatible model available. Automatic uses " + name + L".";
    }
    SetWindowTextW(g_aiModelInfo, t.c_str());
}

// ── Background check ──────────────────────────────────────────
static void aiStartCheck(HWND hwnd, std::string key, AiCheckPurpose purpose, bool usingSaved) {
    int id = ++g_aiJobId;
    if (purpose != AI_CHECK_OPEN) aiSetBusy(true);
    size_t h = aiHash(key);
    std::thread([hwnd, id, purpose, usingSaved, h](std::string k) {
        auto* r = new AiCheckResult{id, purpose, usingSaved, h, gemini_checkKey(k)};
        if (!k.empty()) SecureZeroMemory(&k[0], k.size());
        // Fails if the dialog was closed meanwhile; the result is dropped.
        if (!PostMessageW(hwnd, WM_AI_CHECK_DONE, 0, (LPARAM)r)) delete r;
    }, std::move(key)).detach();
}

static void aiSaveAndClose(HWND hwnd, const std::string& key, const GeminiKeyCheck* check);

static void aiOnCheckDone(HWND hwnd, AiCheckResult& r) {
    if (r.jobId != g_aiJobId) return;          // superseded by a newer check
    if (r.purpose != AI_CHECK_OPEN) aiSetBusy(false);
    const GeminiKeyCheck& c = r.check;
    if (c.keyValid) aiFillModels(c.models, true);
    aiShowModelInfo(c);

    std::wstring modelNote = c.models.empty()
        ? L" But no compatible Gemini model is available to it right now."
        : L"";
    switch (r.purpose) {
        case AI_CHECK_OPEN:
            if (c.keyValid)
                aiSetStatus(L"✓ Your saved key works." + modelNote,
                            c.models.empty() ? RGB(150, 90, 0) : RGB(20, 120, 50));
            else
                aiSetStatus(L"Couldn't confirm your saved key: " + s2w(c.error), RGB(150, 90, 0));
            return;
        case AI_CHECK_TEST:
            if (c.keyValid)
                aiSetStatus((r.usingSaved ? L"✓ Your saved key works."
                                          : L"✓ This key works. Click Save to keep it.") + modelNote,
                            c.models.empty() ? RGB(150, 90, 0) : RGB(20, 120, 50));
            else {
                aiSetStatus(L"✗ " + s2w(c.error), RGB(160, 30, 30));
                if (c.rejected && !r.usingSaved) aiReselectKey();
            }
            return;
        case AI_CHECK_SAVE: {
            std::string key = aiEditText();
            if (key.empty() || aiHash(key) != r.keyHash) {
                aiSetStatus(L"The key changed while it was being checked. Click Save again.",
                            RGB(150, 90, 0));
                return;
            }
            // A key Google rejects is never saved — the dialog stays open
            // so it can be fixed. If Google can't be reached, a second
            // Save keeps it anyway.
            if (!c.keyValid && c.rejected) {
                aiSetStatus(L"✗ " + s2w(c.error) + L" The key was not saved.", RGB(160, 30, 30));
                aiReselectKey();
                return;
            }
            if (!c.keyValid) {
                g_aiUnverifiedKey = key;
                aiSetStatus(L"Couldn't check this key: " + s2w(c.error) +
                            L" Click Save again to save it anyway.", RGB(150, 90, 0));
                SecureZeroMemory(&key[0], key.size());
                return;
            }
            aiSaveAndClose(hwnd, key, &c);
            SecureZeroMemory(&key[0], key.size());
            return;
        }
    }
}

static void aiCloseDialog(HWND hwnd) {
    if (!g_aiUnverifiedKey.empty())
        SecureZeroMemory(&g_aiUnverifiedKey[0], g_aiUnverifiedKey.size());
    g_aiUnverifiedKey.clear();
    ++g_aiJobId;                 // any check still running is now stale
    g_aiDlgOpen = false;
    DestroyWindow(hwnd);
}

// Stores the model preference if it changed. False on a write error.
static bool aiSavePreference() {
    std::string sel = aiSelectedModel();
    if (sel == g_aiPrefAtOpen) return true;
    if (!gemini_setModelPreference(sel)) return false;
    g_aiPrefChanged = true;
    return true;
}

// key empty = keep the saved key, store only the model preference.
static void aiSaveAndClose(HWND hwnd, const std::string& key, const GeminiKeyCheck* check) {
    if (!key.empty() && !gemini_saveKey(key)) {
        // No file fallback — the key is simply not saved.
        aiSetStatus(L"✗ Couldn't save to Windows Credential Manager "
                    L"(Windows error " + std::to_wstring(secretLastError()) +
                    L"). The key was not saved anywhere.", RGB(160, 30, 30));
        return;
    }
    if (!key.empty()) g_aiChanged = true;
    if (check && check->keyValid && check->models.empty()) g_aiNoModelWarning = true;
    if (!aiSavePreference()) {
        aiSetStatus(L"✗ Couldn't save the model choice in %LOCALAPPDATA%\\CALSS.",
                    RGB(160, 30, 30));
        return;
    }
    aiCloseDialog(hwnd);
}

static LRESULT CALLBACK AiSettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_AI_CHECK_DONE) {
        std::unique_ptr<AiCheckResult> r((AiCheckResult*)lParam);
        if (g_aiDlgOpen && hwnd == g_aiDlg) aiOnCheckDone(hwnd, *r);
        return 0;
    }
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
            if (g_aiBusy) { SetCursor(LoadCursor(nullptr, IDC_APPSTARTING)); return TRUE; }
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
            if (g_aiBusy && (id == ID_AI_TEST || id == ID_AI_SAVE || id == IDOK || id == ID_AI_REMOVE))
                return 0;
            if (id == ID_AI_TEST) {
                std::string key = aiEditText();
                bool usingSaved = false;
                if (key.empty()) { key = loadApiKey(); usingSaved = true; }
                if (key.empty()) {
                    aiSetStatus(L"Paste a key into the box first.", RGB(160, 30, 30));
                    return 0;
                }
                aiSetStatus(L"Testing the key with Google…", RGB(60, 60, 60));
                aiStartCheck(hwnd, std::move(key), AI_CHECK_TEST, usingSaved);
                return 0;
            }
            // IDOK: Enter in the key box. This window isn't a real dialog
            // class, so IsDialogMessage sends IDOK rather than Save's ID.
            if (id == ID_AI_SAVE || id == IDOK) {
                std::string key = aiEditText();
                if (key.empty()) {
                    // Nothing typed: with a saved key, Save keeps that key
                    // and stores only the model choice.
                    if (g_aiHasSavedKey) aiSaveAndClose(hwnd, "", nullptr);
                    else aiSetStatus(L"Paste a key into the box first.", RGB(160, 30, 30));
                    return 0;
                }
                if (key == g_aiUnverifiedKey) {      // second Save, Google unreachable
                    aiSaveAndClose(hwnd, key, nullptr);
                    SecureZeroMemory(&key[0], key.size());
                    return 0;
                }
                aiSetStatus(L"Checking the key with Google…", RGB(60, 60, 60));
                aiStartCheck(hwnd, std::move(key), AI_CHECK_SAVE, false);
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
    g_aiPrefChanged = false;
    g_aiNoModelWarning = false;
    g_aiBusy = false;
    g_aiComboIds.clear();
    g_aiPrefAtOpen = gemini_getModelPreference();
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

    const int DW = S(500), DH = S(530);
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
    g_aiHasSavedKey = !saved.empty();
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

    g_aiStatus = mk(L"STATIC", L"", 0, pad, S(254), w, S(60), 0);

    mk(L"STATIC", L"Gemini model:", 0, pad, S(322), w, S(22), 0);
    g_aiModelCombo = mk(L"COMBOBOX", L"", WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                        pad, S(346), w, S(220), ID_AI_MODEL);
    g_aiModelInfo = mk(L"STATIC", L"", 0, pad, S(380), w, S(44), 0);
    aiFillModels({}, false);

    int by = DH - S(96), bh = S(32);
    g_aiBtnTest = mk(L"BUTTON", L"Test key", WS_TABSTOP, pad, by, S(100), bh, ID_AI_TEST);
    g_aiBtnSave = mk(L"BUTTON", L"Save", WS_TABSTOP | BS_DEFPUSHBUTTON, pad + S(110), by, S(90), bh, ID_AI_SAVE);
    g_aiBtnRemove = mk(L"BUTTON", L"Remove saved key", WS_TABSTOP, pad + S(210), by, S(140), bh, ID_AI_REMOVE);
    EnableWindow(g_aiBtnRemove, g_aiHasSavedKey);
    mk(L"BUTTON", L"Cancel", WS_TABSTOP, DW - S(36) - S(90), by, S(90), bh, ID_AI_CANCEL);

    EnableWindow(g_hMainWindow, FALSE);
    ShowWindow(d, SW_SHOW);
    SetFocus(g_aiEdit);
    g_aiDlgOpen = true;

    // With a key saved, find out right away (in the background) whether
    // it works and which model it gets, to fill the model list.
    if (g_aiHasSavedKey) {
        SetWindowTextW(g_aiModelInfo, L"Checking which Gemini models your saved key can use…");
        aiStartCheck(d, saved, AI_CHECK_OPEN, true);
    } else {
        SetWindowTextW(g_aiModelInfo, L"Automatic picks a stable, low-cost Gemini Flash-Lite "
                                      L"(or Flash) model your key can use.");
    }
    if (!saved.empty()) SecureZeroMemory(&saved[0], saved.size());

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
    bool keyChanged = showAiSettingsDialog(parent);
    if (!keyChanged) {
        if (g_aiPrefChanged) {
            std::string pref = gemini_getModelPreference();
            audit_log("AI MODEL SETTING", "Gemini model: " + (pref.empty() ? std::string("Automatic") : pref));
            showToast(L"Gemini model setting saved.", TOAST_SUCCESS);
        }
        return;
    }

    invalidateAiKeyState();
    bool hasKey = aiKeyConfigured();
    // Newly set up → turn AI on; removed → off and disabled.
    syncAiSummaryCheckbox(hasKey && !hadKey);
    audit_log(hasKey ? "AI KEY SAVED" : "AI KEY REMOVED",
              "Gemini API key stored in Windows Credential Manager (key not logged).");
    if (hasKey && g_aiNoModelWarning)
        showToast(L"AI key saved, but no compatible Gemini model is available to it right now.",
                  TOAST_WARNING);
    else
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
