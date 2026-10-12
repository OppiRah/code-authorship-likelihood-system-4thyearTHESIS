// ─────────────────────────────────────────────────────────────
// gui_workspace.cpp
// "Continue previous session" (1.3.0, professor feedback: "go back to
// previous work and continue where I left off"). Scope agreed with the
// team: only the ONE most recent workspace, no history list.
//
//   startup    workspaceInitAtStartup() reads workspace.json; if it is
//              usable, the empty-state screen shows a "Continue
//              previous session" card (drawResumeSessionCard) above the
//              three data-source cards. Nothing happens automatically.
//   Continue   workspaceContinue(): checks the saved folder still exists
//              and has .c files, loads it exactly like Select folder,
//              and runs the normal analysis WITH AI OFF (no Gemini quota
//              spent, works offline and without a key). Never touches
//              Google sign-in or Classroom sync.
//   complete   workspaceOnAnalysisComplete(): once the fresh results
//              are in, re-applies the saved page / class / student /
//              flagged pair, each looked up BY NAME in the new results;
//              anything that no longer exists is skipped and reported.
//   saving     workspaceSaveIfChanged(), called after every content
//              paint and on exit: rebuilds the current navigation and
//              writes the file only when it differs from the last write
//              — and only while the results on screen are a finished,
//              valid run for the folder that is loaded.
//
// Old scores are never shown: the file holds no results at all.
// File format, parsing and the pure decisions: workspace_state.h/.cpp.
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"
#include "../include/app_paths.h"
#include "../include/version.h"
#include "../include/workspace_state.h"

#ifdef _WIN32

static bool           s_offerAvailable = false;  // card shown on the empty state
static WorkspaceState s_offer;

static bool           s_restorePending = false;  // analysis running for a restore
static WorkspaceState s_restore;
static std::string    s_restoreFolderU8;

static bool           s_haveLastSaved = false;   // last state written (or read at startup)
static WorkspaceState s_lastSaved;

// The flagged pair the professor last opened (stable flaggedPairs index
// for the CURRENT results; reset on every new run).
static int            s_focusPairIdx = -1;

static std::string nowLocalString() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[32];
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buf;
}

// A path short enough for the modal: its body is word-wrapped, and one
// long unbreakable path is measured wider than it is drawn (the last
// line would be clipped).
static std::wstring shortPathForDisplay(const std::string& pathU8) {
    std::wstring p = widenUtf8(pathU8);
    const size_t maxChars = 60;
    if (p.size() <= maxChars) return p;
    return p.substr(0, 20) + L"…" + p.substr(p.size() - (maxChars - 21));
}

static std::string classroomDirU8() {
    // Same folder as appClassroomDirU8(), without creating it.
    return appDataDirU8() + "\\Classroom";
}

void workspaceInitAtStartup() {
    WorkspaceState ws;
    if (loadWorkspaceFile(workspaceFilePath(), ws)) {
        s_offer = ws;
        s_offerAvailable = true;
        s_lastSaved = ws;
        s_haveLastSaved = true;
    }
    // Missing, malformed, future-version or invalid: start normally. The
    // file is left alone; the next finished analysis replaces it.
}

void workspaceDismissOffer() {
    s_offerAvailable = false;
    if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
}

void workspaceNotePairFocus(int pairIdx, bool opened) {
    if (opened) s_focusPairIdx = pairIdx;
    else if (s_focusPairIdx == pairIdx) s_focusPairIdx = -1;
}

// ── Continue ─────────────────────────────────────────────────

void workspaceContinue(HWND hwnd) {
    if (!s_offerAvailable || g_state.analysisRunning) return;
    const WorkspaceState ws = s_offer;
    s_offerAvailable = false;

    std::wstring folderW = widenUtf8(ws.dataset);
    DWORD attr = GetFileAttributesW(folderW.c_str());
    bool exists = attr != INVALID_FILE_ATTRIBUTES;
    bool isDir  = exists && (attr & FILE_ATTRIBUTE_DIRECTORY);
    int  count  = isDir ? countCFiles(folderW) : 0;
    bool isClassroom = pathIsInside(ws.dataset, classroomDirU8());

    DatasetCheck check = classifyDataset(exists, isDir, count, isClassroom);
    if (check != DatasetCheck::Ok) {
        std::wstring name = s2w(datasetDisplayName(ws.dataset));
        if (check == DatasetCheck::MissingClassroom) {
            showAppModal(L"Previous session unavailable",
                L"The Google Classroom submissions for “" + name + L"” are no "
                L"longer on this computer. CALSS removes its Classroom downloads "
                L"when it is uninstalled, and they may also have been deleted.\n\n"
                L"Sync the class again from Google Classroom, then run the analysis.",
                MODAL_CONFIRM, INFO_, L"Sync Classroom", L"Close",
                [](bool sync) {
                    if (sync) PostMessageW(g_hMainWindow, WM_COMMAND,
                                           MAKEWPARAM(ID_BTN_GCLASSROOM, 0), 0);
                });
        } else {
            std::wstring why =
                check == DatasetCheck::NoCFiles
                    ? L"The folder no longer contains any .c files."
                : check == DatasetCheck::NotAFolder
                    ? L"The saved location is no longer a folder."
                    : L"The folder can’t be found. It may have been moved, renamed "
                      L"or deleted, or it is on a drive that isn’t connected.";
            showAppModal(L"Previous session unavailable",
                why + L"\n\n" + shortPathForDisplay(ws.dataset) +
                L"\n\nSelect the folder again to start a new analysis.",
                MODAL_CONFIRM, INFO_, L"Select folder…", L"Close",
                [](bool pick) {
                    if (pick) PostMessageW(g_hMainWindow, WM_COMMAND,
                                           MAKEWPARAM(ID_BTN_SELECT_FOLDER, 0), 0);
                });
        }
        if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
        return;
    }

    g_state.dataFolder       = folderW;
    g_state.fileCount        = count;
    g_state.hasData          = true;
    g_state.analysisComplete = false;

    s_restore          = ws;
    s_restoreFolderU8  = toNarrowGC(folderW);
    s_restorePending   = true;
    startAnalysisRun(hwnd, /*useAI=*/false, /*restoringSession=*/true);
}

// ── After the analysis ───────────────────────────────────────

void workspaceOnAnalysisComplete(HWND content) {
    s_focusPairIdx = -1;  // pair indices belong to the previous results
    if (!s_restorePending) return;
    s_restorePending = false;

    const AnalysisResults& r = g_analysisResults;
    if (!r.valid || r.dataFolder != s_restoreFolderU8) {
        showToast(L"The previous session was not restored because the analysis "
                  L"did not finish. Run the analysis again from the sidebar.",
                  TOAST_WARNING);
        return;
    }

    const WorkspaceState& ws = s_restore;
    std::vector<std::wstring> missing;

    g_selectedStudent = -1;
    g_currentBlockTab = -1;
    g_scrollY = 0;

    if (ws.page == WS_PAGE_CLASSES) {
        g_currentPage = PAGE_STUDENTS;
        if (!ws.block.empty()) {
            int t = blockTabIndexForName(ws.block);
            if (t >= 0) g_currentBlockTab = t;
            else missing.push_back(L"the class “" + s2w(ws.block) + L"”");
        }
        if (!ws.student.empty()) {
            std::vector<std::string> names;
            names.reserve(r.profiles.size());
            for (const auto& p : r.profiles) names.push_back(p.name);
            int s = findByName(names, ws.student);
            if (s >= 0) g_selectedStudent = s;
            else missing.push_back(L"the student “" + s2w(ws.student) + L"”");
        }
    } else if (ws.page == WS_PAGE_FLAGGED) {
        g_currentPage = PAGE_FLAGGED;
        if (!ws.pairFileA.empty()) {
            std::vector<std::pair<std::string, std::string>> files;
            files.reserve(r.flaggedPairs.size());
            for (const auto& p : r.flaggedPairs) files.emplace_back(p.filenameA, p.filenameB);
            int idx = findPair(files, ws.pairFileA, ws.pairFileB);
            if (idx >= 0) {
                jumpToFlaggedPair(content, idx);  // expands it and scrolls to it
                s_focusPairIdx = idx;
            } else {
                missing.push_back(L"the flagged pair " +
                    s2w(ws.pairLabel.empty() ? ws.pairFileA + " vs " + ws.pairFileB
                                             : ws.pairLabel) +
                    L" (it is no longer flagged)");
            }
        }
    } else {
        g_currentPage = PAGE_OVERVIEW;
    }

    g_contentFadeAlpha = 60;
    SetTimer(content, TIMER_FADE, 15, nullptr);
    SetTimer(content, TIMER_TAB_UNDERLINE, 12, nullptr);
    InvalidateRect(content, nullptr, FALSE);

    std::wstring changed;
    if (ws.fileCount > 0 && ws.fileCount != g_state.fileCount)
        changed = L" The folder now has " + std::to_wstring(g_state.fileCount) +
                  L" files (it had " + std::to_wstring(ws.fileCount) + L").";

    if (missing.empty()) {
        showToast(L"Previous session restored. The analysis was re-run on the current "
                  L"files with the AI summary off." + changed, TOAST_SUCCESS);
    } else {
        std::wstring body = L"The analysis was re-run on the current files (AI summary "
                            L"off), but these are no longer in the results:\n";
        for (const auto& m : missing) body += L"\n  •  " + m;
        if (!changed.empty()) body += L"\n\n" + changed.substr(1);
        showAppModal(L"Session partly restored", body, MODAL_OK, INFO_, L"OK");
    }
}

// ── Saving ───────────────────────────────────────────────────

void workspaceSaveIfChanged() {
    if (s_restorePending || g_state.analysisRunning || !g_state.analysisComplete) return;
    const AnalysisResults& r = g_analysisResults;
    if (!r.valid) return;
    std::string folderU8 = toNarrowGC(g_state.dataFolder);
    if (folderU8.empty() || folderU8 != r.dataFolder) return;  // results are for another folder

    WorkspaceState ws;
    ws.dataset   = folderU8;
    ws.fileCount = g_state.fileCount;
    if (g_currentPage == PAGE_STUDENTS) {
        ws.page = WS_PAGE_CLASSES;
        if (g_currentBlockTab >= 0) ws.block = blockTabNameForIndex(g_currentBlockTab);
        if (g_selectedStudent >= 0 && g_selectedStudent < (int)r.profiles.size())
            ws.student = r.profiles[g_selectedStudent].name;
    } else if (g_currentPage == PAGE_FLAGGED) {
        ws.page = WS_PAGE_FLAGGED;
        if (s_focusPairIdx >= 0 && s_focusPairIdx < (int)r.flaggedPairs.size() &&
            g_expandedPairs.count(s_focusPairIdx)) {
            const auto& p = r.flaggedPairs[s_focusPairIdx];
            ws.pairFileA = p.filenameA;
            ws.pairFileB = p.filenameB;
            ws.pairLabel = p.studentA.studentName + " vs " + p.studentB.studentName;
        }
    } else {
        ws.page = WS_PAGE_OVERVIEW;
    }
    // Names longer than the reader accepts are not saved (the file must
    // always be readable back).
    for (std::string* s : {&ws.block, &ws.student, &ws.pairFileA, &ws.pairFileB, &ws.pairLabel})
        if (s->size() > WORKSPACE_MAX_NAME_CHARS) s->clear();
    if (ws.pairFileA.empty() || ws.pairFileB.empty()) {
        ws.pairFileA.clear(); ws.pairFileB.clear(); ws.pairLabel.clear();
    }
    if (ws.dataset.size() > WORKSPACE_MAX_PATH_CHARS) return;

    if (s_haveLastSaved && sameWorkspaceContent(ws, s_lastSaved)) return;

    ws.appVersion = CALSS_VERSION_STR;
    ws.savedAt    = nowLocalString();
    // Recorded even if the write fails, so a read-only profile can't
    // turn every repaint into another write attempt.
    saveWorkspaceFile(workspaceFilePath(), ws);
    s_lastSaved = ws;
    s_haveLastSaved = true;
}

// ── Startup card ─────────────────────────────────────────────

static void drawResumeButton(HDC hdc, const RECT& r, const wchar_t* label,
                             bool primary, bool hovered) {
    COLORREF bg = primary ? (hovered ? MAROON_600 : MAROON_700) : (hovered ? GRAY_800 : GRAY_850);
    COLORREF bd = primary ? (hovered ? GOLD_400 : MAROON_600) : (hovered ? GOLD_400 : GRAY_700);
    drawCard(hdc, r, bg, bd, 6);
    HFONT of = (HFONT)SelectObject(hdc, g_hFontButton);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, primary ? WHITE_ : GRAY_200);
    RECT tr = r;
    DrawTextW(hdc, label, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, of);
}

int drawResumeSessionCard(HDC hdc, int x, int y, int width, int cardW) {
    if (!s_offerAvailable || g_state.hasData || g_state.analysisRunning) return 0;

    const int cardH = S(132);
    int left = x + (width - cardW) / 2;
    RECT card = {left, y, left + cardW, y + cardH};
    drawCard(hdc, card, GRAY_850, GRAY_700, 10, GOLD_500);

    const int padL = S(24), btnW = S(150), btnH = S(36), btnGap = S(10);
    int btnX = card.right - S(20) - btnW;
    int textR = btnX - S(20);

    SetBkMode(hdc, TRANSPARENT);
    HFONT of = (HFONT)SelectObject(hdc, g_hFontLabel);
    drawEyebrow(hdc, left + padL, y + S(16), textR - left - padL, L"CONTINUE PREVIOUS SESSION");

    struct Row { const wchar_t* label; std::wstring value; };
    std::wstring dataset = s2w(datasetDisplayName(s_offer.dataset));
    if (s_offer.fileCount > 0)
        dataset += L"  ·  " + std::to_wstring(s_offer.fileCount) + L" files";
    Row rows[3] = {
        { L"Dataset",       dataset },
        { L"Last location", s2w(describeLocation(s_offer)) },
        { L"Last used",     s2w(s_offer.savedAt) },
    };
    int labelW = S(110);
    int ry = y + S(42);
    for (const Row& row : rows) {
        if (row.value.empty()) continue;
        SelectObject(hdc, g_hFontLabel);
        SetTextColor(hdc, GRAY_400);
        RECT lr = {left + padL, ry, left + padL + labelW, ry + S(22)};
        DrawTextW(hdc, row.label, -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, g_hFontBodyNew);
        SetTextColor(hdc, GRAY_200);
        RECT vr = {lr.right, ry, textR, ry + S(22)};
        DrawTextW(hdc, row.value.c_str(), -1, &vr,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        ry += S(24);
    }

    // Buttons: hit-tested and keyboard-focused as empty-state cards
    // (EmptyCardRect → WM_COMMAND), like the three source cards below.
    int by = y + (cardH - (btnH * 2 + btnGap)) / 2;
    struct Btn { const wchar_t* label; int cmd; bool primary; };
    const Btn btns[2] = {
        { L"Continue",  ID_BTN_RESUME_SESSION, true  },
        { L"Start new", ID_BTN_RESUME_DISMISS, false },
    };
    for (const Btn& b : btns) {
        int idx = (int)g_emptyCardRects.size();
        RECT br = {btnX, by, btnX + btnW, by + btnH};
        drawResumeButton(hdc, br, b.label, b.primary, g_hoveredEmptyCard == idx);
        EmptyCardRect hit { br.left, br.top + g_scrollY, btnW, btnH, b.cmd };
        g_emptyCardRects.push_back(hit);
        addFocusable(g_contentFocusables, FocusKind::EmptyStateCard,
                     RECT{hit.x, hit.y, hit.x + hit.w, hit.y + hit.h}, hit.cmdId, true);
        by += btnH + btnGap;
    }

    SelectObject(hdc, of);
    return cardH;
}

#endif // _WIN32
