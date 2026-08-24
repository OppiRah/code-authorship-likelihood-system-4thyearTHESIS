// ─────────────────────────────────────────────────────────────
// gui_assign_dialog.cpp
// Unclassified-file assignment dialog — the modal that lets an
// instructor manually attach a student, category and period to a
// file that couldn't be auto-categorized, plus the file operations
// (rename / delete) that follow from it.
//
// Split out of gui.cpp unchanged. All 8 of its globals stay private
// to this file; handleUnclassifiedAction is the single entry point
// the rest of the app calls, already declared in gui_common.h.
//
// NOTE: extractStudentNameGC() used to sit between applyFileAssignment
// and deleteUnclassifiedFile in gui.cpp, but its only caller is
// exportPairsToCsv (Flagged Pairs), so it stayed behind rather than
// moving here. See the checkpoint 3 report.
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"

#ifdef _WIN32

// ═════════════════════════════════════════════════════════════
// Unclassified file assignment — dialog + file operations
// ═════════════════════════════════════════════════════════════
//
// When a file can't be auto-categorized (unknown type, unknown
// student, or both — see main.cpp's categorizeFile() priority
// hierarchy), it lands in the Unclassified bucket. This lets the
// instructor manually fix it: pick a real student from the known
// roster, pick a category and period, and the file gets renamed to
// match the exact same "{period}{type}__{name}_ASGN{fingerprint}.c"
// convention Classroom sync already uses — making it indistinguishable
// from a normal synced file to every downstream function. No special
// casing needed anywhere else in the pipeline.

struct AssignChoice {
    bool        confirmed;
    std::string studentName;
    std::string category; // "exam", "quiz", "activity"
    std::string period;   // "midterm", "semifinal", "final"
};

static AssignChoice        g_assignResult;
static HWND                g_assignListBox = nullptr;
static HWND                g_assignRadioExam = nullptr, g_assignRadioQuiz = nullptr, g_assignRadioActivity = nullptr;
static HWND                g_assignRadioMid = nullptr, g_assignRadioSemi = nullptr, g_assignRadioFinal = nullptr;
static HWND                g_assignErrorLabel = nullptr;
static bool                g_assignDialogOpen = false;
static std::vector<std::string> g_assignStudentList;
static bool                g_assignClassRegistered = false;

static LRESULT CALLBACK AssignDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CTLCOLORSTATIC: {
            // Render the inline validation label in red; everything
            // else on this native-styled dialog stays default.
            if ((HWND)lParam == g_assignErrorLabel) {
                HDC hdcCtl = (HDC)wParam;
                SetTextColor(hdcCtl, RGB(180, 30, 30));
                SetBkMode(hdcCtl, TRANSPARENT);
                return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == 1001) { // Assign
                int sel = (int)SendMessageW(g_assignListBox, LB_GETCURSEL, 0, 0);
                if (sel == LB_ERR) {
                    // Compliance fix item 5: inline validation label
                    // instead of a native MessageBox — this dialog
                    // already uses plain system controls, so a static
                    // text control fits its own visual language rather
                    // than pulling in the toast/modal system built for
                    // the main canvas.
                    if (g_assignErrorLabel) {
                        SetWindowTextW(g_assignErrorLabel, L"Please select a student.");
                        ShowWindow(g_assignErrorLabel, SW_SHOW);
                    }
                    return 0;
                }
                g_assignResult.studentName = g_assignStudentList[sel];
                g_assignResult.category =
                    (SendMessageW(g_assignRadioExam, BM_GETCHECK, 0, 0) == BST_CHECKED) ? "exam" :
                    (SendMessageW(g_assignRadioQuiz, BM_GETCHECK, 0, 0) == BST_CHECKED) ? "quiz" : "activity";
                g_assignResult.period =
                    (SendMessageW(g_assignRadioMid, BM_GETCHECK, 0, 0) == BST_CHECKED) ? "midterm" :
                    (SendMessageW(g_assignRadioSemi, BM_GETCHECK, 0, 0) == BST_CHECKED) ? "semifinal" : "final";
                g_assignResult.confirmed = true;
                g_assignDialogOpen = false;
                DestroyWindow(hwnd);
                return 0;
            } else if (id == 1002 || id == IDCANCEL) {
                // IDCANCEL: compliance fix item 2 — IsDialogMessageW
                // (used in this dialog's modal loop) intercepts Esc
                // itself and sends WM_COMMAND(IDCANCEL) directly,
                // never reaching a WM_KEYDOWN handler at all. The
                // Cancel button's own ID (1002) isn't IDCANCEL, so
                // without this branch Esc was silently swallowed.
                g_assignResult.confirmed = false;
                g_assignDialogOpen = false;
                DestroyWindow(hwnd);
                return 0;
            }
            return 0;
        }
        case WM_CLOSE:
            g_assignResult.confirmed = false;
            g_assignDialogOpen = false;
            DestroyWindow(hwnd);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// Shows a modal assignment dialog. Uses default system control
// styling (not the dark UL theme) — this is a lightweight utility
// dialog, not main app chrome, so plain Windows styling keeps the
// implementation simple and reliable rather than fighting custom
// control colors for a rarely-used one-off action.
static AssignChoice showAssignDialog(HWND parent, const std::string& filename,
                                       const std::vector<std::string>& knownStudents)
{
    g_assignResult = {};
    g_assignStudentList = knownStudents;

    const int DW = S(380), DH = S(500);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    HINSTANCE hInst = GetModuleHandle(nullptr);

    if (!g_assignClassRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = AssignDialogProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = L"CALSSAssignDialog";
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        RegisterClassExW(&wc);
        g_assignClassRegistered = true;
    }

    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"CALSSAssignDialog", L"Assign Unclassified File",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        (screenW-DW)/2, (screenH-DH)/2, DW, DH,
        parent, nullptr, hInst, nullptr);

    std::wstring fileLabel = L"File: " + s2w(filename);
    CreateWindowW(L"STATIC", fileLabel.c_str(), WS_CHILD | WS_VISIBLE,
        S(20), S(15), DW-S(60), S(20), dlg, nullptr, hInst, nullptr);

    CreateWindowW(L"STATIC", L"Assign to student:", WS_CHILD | WS_VISIBLE,
        S(20), S(45), S(250), S(20), dlg, nullptr, hInst, nullptr);

    g_assignListBox = CreateWindowW(L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        S(20), S(68), DW-S(60), S(130), dlg, (HMENU)2001, hInst, nullptr);
    for (const auto& s : knownStudents)
        SendMessageW(g_assignListBox, LB_ADDSTRING, 0, (LPARAM)s2w(s).c_str());
    if (!knownStudents.empty())
        SendMessageW(g_assignListBox, LB_SETCURSEL, 0, 0);

    CreateWindowW(L"STATIC", L"Category:", WS_CHILD | WS_VISIBLE,
        S(20), S(212), S(200), S(20), dlg, nullptr, hInst, nullptr);
    g_assignRadioExam = CreateWindowW(L"BUTTON", L"Exam",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        S(20), S(236), S(100), S(22), dlg, (HMENU)2010, hInst, nullptr);
    g_assignRadioQuiz = CreateWindowW(L"BUTTON", L"Quiz",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        S(130), S(236), S(100), S(22), dlg, (HMENU)2011, hInst, nullptr);
    g_assignRadioActivity = CreateWindowW(L"BUTTON", L"Activity",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        S(240), S(236), S(110), S(22), dlg, (HMENU)2012, hInst, nullptr);
    SendMessageW(g_assignRadioExam, BM_SETCHECK, BST_CHECKED, 0);

    CreateWindowW(L"STATIC", L"Period:", WS_CHILD | WS_VISIBLE,
        S(20), S(276), S(200), S(20), dlg, nullptr, hInst, nullptr);
    g_assignRadioMid = CreateWindowW(L"BUTTON", L"Midterm",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        S(20), S(300), S(100), S(22), dlg, (HMENU)2020, hInst, nullptr);
    g_assignRadioSemi = CreateWindowW(L"BUTTON", L"Semifinal",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        S(130), S(300), S(100), S(22), dlg, (HMENU)2021, hInst, nullptr);
    g_assignRadioFinal = CreateWindowW(L"BUTTON", L"Final",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        S(240), S(300), S(110), S(22), dlg, (HMENU)2022, hInst, nullptr);
    SendMessageW(g_assignRadioMid, BM_SETCHECK, BST_CHECKED, 0);

    CreateWindowW(L"STATIC",
        L"Note: Prelim is intentionally not offered — Prelim submissions\n"
        L"are excluded from profiling and comparison by design.",
        WS_CHILD | WS_VISIBLE,
        S(20), S(335), DW-S(60), S(40), dlg, nullptr, hInst, nullptr);

    // Inline validation label (compliance fix item 5) — hidden until
    // the Assign click actually needs it.
    g_assignErrorLabel = CreateWindowW(L"STATIC", L"",
        WS_CHILD | SS_NOTIFY,
        S(20), S(385), DW-S(40), S(20), dlg, nullptr, hInst, nullptr);
    HFONT errFont = CreateFontW(S(-13), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SendMessageW(g_assignErrorLabel, WM_SETFONT, (WPARAM)errFont, TRUE);

    CreateWindowW(L"BUTTON", L"Assign", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        S(70), S(430), S(100), S(32), dlg, (HMENU)1001, hInst, nullptr);
    CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE,
        S(210), S(430), S(100), S(32), dlg, (HMENU)1002, hInst, nullptr);

    EnableWindow(g_hMainWindow, FALSE);
    ShowWindow(dlg, SW_SHOW);
    g_assignDialogOpen = true;

    MSG msg;
    while (g_assignDialogOpen && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(g_hMainWindow, TRUE);
    SetForegroundWindow(g_hMainWindow);

    return g_assignResult;
}

// Renames an unclassified file to match the standard sync-generated
// naming convention, based on the instructor's dialog choices. This
// makes the file indistinguishable from a normal Classroom-synced
// file to every downstream categorization/parsing function.
static bool applyFileAssignment(const std::string& fullPath,
                                  const AssignChoice& choice) {
    std::string categoryTag = choice.period + choice.category;

    std::string safeName;
    for (char c : choice.studentName) {
        if (isalnum((unsigned char)c) || c == '_' || c == '-') safeName += c;
        else if (c == ' ') safeName += '_';
    }

    char fingerprint[16];
    snprintf(fingerprint, sizeof(fingerprint), "%06X",
             (unsigned)(GetTickCount() & 0xFFFFFF));

    // Preserve whatever folder (root or block subfolder) the file was
    // already sitting in — assignment shouldn't silently move a file
    // between blocks.
    size_t lastSlash = fullPath.find_last_of("\\/");
    std::string folder = (lastSlash != std::string::npos)
        ? fullPath.substr(0, lastSlash) : ".";

    std::string newPath = folder + "\\" + categoryTag + "__" +
        safeName + "_ASGN" + fingerprint + ".c";

    return MoveFileA(fullPath.c_str(), newPath.c_str()) != 0;
}

// Deletes a manually-imported / unclassified file from disk. Scoped
// intentionally to unclassified files only — files with a real
// Classroom-synced manifest entry are never offered a delete option
// through the app.
static bool deleteUnclassifiedFile(const std::string& fullPath) {
    return DeleteFileA(fullPath.c_str()) != 0;
}

// Shared by the mouse click handler (WM_LBUTTONDOWN) and
// activateFocused() (Enter key) — same business logic either way, so
// the two input paths can never drift apart (item 2 design principle).
// Factored out of what used to be an inline block only the mouse
// handler reached.
void handleUnclassifiedAction(HWND hwnd, int fileIdx, bool isDelete) {
    if (fileIdx < 0 || fileIdx >= (int)g_analysisResults.unclassifiedFiles.size())
        return;

    const auto& uf = g_analysisResults.unclassifiedFiles[fileIdx];
    // Copy out before any potential vector mutation below (erasing
    // invalidates the uf reference).
    std::string ufKey = uf.key;
    std::string ufFullPath = uf.fullPath;
    int ufIdx = fileIdx;

    if (isDelete) {
        std::wstring confirmMsg =
            L"Delete this file?\n\n" + s2w(ufKey) +
            L"\n\nThis cannot be undone.";
        showAppModal(L"Confirm delete", confirmMsg,
            MODAL_CONFIRM, COLOR_ERROR, L"Delete", L"Cancel",
            [ufFullPath, ufIdx](bool confirmed) {
                if (!confirmed) return;
                if (deleteUnclassifiedFile(ufFullPath)) {
                    if (ufIdx >= 0 && ufIdx <
                        (int)g_analysisResults.unclassifiedFiles.size())
                    {
                        g_analysisResults.unclassifiedFiles.erase(
                            g_analysisResults.unclassifiedFiles.begin() + ufIdx);
                    }
                    InvalidateRect(g_hContent, nullptr, FALSE);
                } else {
                    showToast(L"Could not delete the file. It may be open elsewhere.",
                               TOAST_ERROR);
                }
            });
    } else {
        std::vector<std::string> knownStudents;
        for (const auto& p : g_analysisResults.profiles)
            knownStudents.push_back(p.name);

        if (knownStudents.empty()) {
            showToast(L"No known students yet. Run an analysis with at "
                       L"least one recognized student first, then assign "
                       L"this file.", TOAST_WARNING);
            return;
        }

        AssignChoice choice = showAssignDialog(hwnd, ufKey, knownStudents);
        if (choice.confirmed) {
            if (applyFileAssignment(ufFullPath, choice)) {
                audit_log("MANUAL ASSIGNMENT",
                    "File: " + ufKey + "\n" +
                    "Assigned to: " + choice.studentName + "\n" +
                    "Category: " + choice.category + "\n" +
                    "Period: " + choice.period);

                // Remove immediately from the visible Unclassified
                // list. Note: this does NOT make the file appear in
                // the assigned student's profile yet — that still
                // requires Run Analysis, since building profiles and
                // pairwise comparisons is a full pipeline pass, not
                // something we can safely patch into live results.
                g_analysisResults.unclassifiedFiles.erase(
                    g_analysisResults.unclassifiedFiles.begin() + ufIdx);

                showToast(L"Assigned to " + s2w(choice.studentName) +
                    L". Click 'Run Analysis' to have this submission "
                    L"appear in their profile and be included in scoring.",
                    TOAST_SUCCESS);
                InvalidateRect(hwnd, nullptr, FALSE);
            } else {
                showToast(L"Could not rename the file. It may be open elsewhere.",
                           TOAST_ERROR);
            }
        }
    }
}

#endif // _WIN32