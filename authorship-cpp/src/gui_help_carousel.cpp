// ─────────────────────────────────────────────────────────────
// gui_help_carousel.cpp
// Help Guide image carousel — the multi-step walkthrough overlay:
// the HELP_STEPS content table, lazy PNG loading from assets\help\,
// step navigation, the annotation-ring sweep/pulse animation, and
// the main draw.
//
// Split out of gui.cpp unchanged. RingSpec, CarouselStep, the
// HELP_STEPS table and the image cache all stay private to this
// file; only the eight globals and seven functions already declared
// in gui_common.h are visible to the rest of the app.
//
// The two source regions were separated in gui.cpp by the focus
// system and the modal/toast drawing code; they are contiguous here.
// Order within each region is unchanged.
// ─────────────────────────────────────────────────────────────

#include "../include/gui_common.h"

#ifdef _WIN32

// ── Help Guide image carousel ──────────────────────────────────────
// A separate system from AppModal above, not a variant of it — see
// the planning discussion: AppModal is shaped for one title/body/
// yes-no answer, and the carousel needs multi-step position, a
// per-step image+ring, and Skip/Next/Done semantics that don't map
// onto primary/secondary accept/cancel. It reuses AppModal's visual
// shell conventions (gray-900 card, 12px radius, gold-500 title, left
// gold accent bar, scrim, "draw on hdcScreen after the main blit")
// and gets an equivalent top-of-handler gate so the two can never be
// open at once by construction, same as every other mutual-exclusion
// pattern in this file.

// One annotation ring over the loaded screenshot. Coordinates are
// design-space, relative to the image frame's own box (not the whole
// modal) — scaled via S() at draw time like everything else. `tag`
// empty = no leader/label (per spec §5: only add a tag when the
// target is small or off to the side; at most one tag per screenshot).
struct RingSpec {
    int cx, cy, w, h;
    std::wstring tag;
    bool dimmed = false; // for the future multi-ring-with-emphasis case; unused today (§5's "otherwise all full opacity, no tags" branch applies to every current step)
};

struct CarouselStep {
    std::wstring title;
    std::wstring caption;
    std::wstring imageFile;   // filename only; resolved against the
                               // cached exe-relative assets\help\ dir
    std::vector<RingSpec> rings; // empty for the closing card
    bool isClosingCard = false;  // explicit flag rather than an
                                  // empty-imageFile sentinel — a
                                  // missing/unloadable image (the
                                  // pre-screenshot state right now)
                                  // is a different condition and
                                  // shouldn't look like a closing card
};

// Design-space size of the 16:10 image frame (fits within the
// modal's 680px max width per spec §7, with room for the card's own
// padding on both sides).
static const int HELP_IMG_FRAME_W_DESIGN = 584;
static const int HELP_IMG_FRAME_H_DESIGN = 365;

// Ring coordinates below are placeholder/illustrative — real captures
// don't exist yet (per this session's status), so these are
// reasonable guesses at roughly where each control sits within its
// own screenshot, meant to be tuned once actual screenshots exist.
// Nothing else about the carousel depends on these being exactly
// right; that's the point of keeping this data-driven.
static const CarouselStep HELP_STEPS[10] = {
    // 1 — Load your submissions
    { L"Load your submissions",
      L"Point CALSS at student code three ways: a folder, individual "
      L"files, or a live Google Classroom sync.",
      L"calss_help_01_load_submissions.png",
      { {110, 300, 160, 60, L"", false},
        {290, 300, 160, 60, L"", false},
        {470, 300, 160, 60, L"", false} },
      false },
    // 2 — Run the analysis
    { L"Run the analysis",
      L"Decide on an AI summary up front, then run. No mid-flow "
      L"prompts to interrupt you.",
      L"calss_help_02_run_analysis.png",
      { {120, 260, 220, 30, L"", false},
        {120, 310, 220, 44, L"", false} },
      false },
    // 3 — Overview: the big picture
    { L"Overview \u2014 the big picture",
      L"Every analysis opens here. Pairs flagged, high-similarity "
      L"count, and max score \u2014 the three numbers worth a glance "
      L"before anything else.",
      L"calss_help_03_overview_summary.png",
      { {292, 90, 520, 110, L"", false} },
      false },
    // 4 — Overview: jump straight to a finding
    { L"Overview \u2014 jump straight to a finding",
      L"The AI briefing references specific pairs by name \u2014 click "
      L"one to open it directly in Flagged Pairs.",
      L"calss_help_04_overview_ai_briefing.png",
      { {420, 220, 140, 22, L"Click here", false} },
      false },
    // 5 — Classes: every student's fingerprint
    { L"Classes \u2014 every student's fingerprint",
      L"Each card carries a Style DNA strip \u2014 a glanceable "
      L"barcode of that student's coding habits.",
      L"calss_help_05_classes_grid.png",
      { {90, 150, 130, 20, L"", false} },
      false },
    // 6 — Classes: a student's full profile
    { L"Classes \u2014 a student's full profile",
      L"Open any card for the full picture, including every pair "
      L"that student has appeared in as a flagged match.",
      L"calss_help_06_classes_student_detail.png",
      { {430, 60, 140, 300, L"", false} },
      false },
    // 7 — Flagged Pairs: what needs a look
    { L"Flagged Pairs \u2014 what needs a look",
      L"Filter by severity to triage fast \u2014 High first, Moderate "
      L"when you have time.",
      L"calss_help_07_flagged_pairs_list.png",
      { {60, 60, 360, 36, L"", false} },
      false },
    // 8 — Flagged Pairs: the side-by-side
    { L"Flagged Pairs \u2014 the side-by-side",
      L"Expand any pair for the full comparison \u2014 the Deviations "
      L"block puts the features that actually mismatched front and "
      L"center.",
      L"calss_help_08_flagged_pairs_detail.png",
      { {60, 180, 240, 140, L"", false} },
      false },
    // 9 — Generate a report
    { L"Generate a report",
      L"Turn any finding into a document you can hand a student or "
      L"attach to a case file.",
      L"calss_help_09_generate_report.png",
      { {380, 20, 200, 34, L"", false} },
      false },
    // 10 — closing card (no screenshot — see the draw function)
    { L"You're set",
      L"That's the full loop \u2014 load, analyze, review. Google "
      L"Classroom sync is a faster way to do step 1, and every run is "
      L"logged under View \u2192 View Audit Log.",
      L"",
      {},
      true },
};

// ── Runtime state ──────────────────────────────────────────────────
HelpCarouselState g_helpCarouselState = HelpCarouselState::Closed;
int g_helpCarouselStep = 0; // 0-based index into HELP_STEPS
// Keyboard focus parity with AppModal (item 2): Tab toggles between
// Skip and Next/Done, same shape as g_modalFocusOnPrimary.
bool g_helpCarouselFocusOnNext = true;
// Elapsed-time base for the current step's entrance sweep + pulse —
// a single stored tick, not a counter, so the pulse is a pure
// function of elapsed time and can't drift (see g_pairAnimating for
// the precedent this follows).
static DWORD g_helpCarouselStepShownTick = 0;
// Direction of the most recent step change, for the cross-fade +
// slide transition (+1 = advancing, -1 = going back).
static int g_helpCarouselTransitionDir = 1;

// This is the first indefinite-duration timer in the app — every
// other one (TIMER_PAIR_EXPAND, TIMER_FADE, TIMER_TOAST, etc.) runs
// until its own completion condition, then KillTimers itself. This
// one runs continuously for as long as the carousel is open (driving
// the per-step pulse loop) and only stops when the carousel closes.

// ── Image cache ─────────────────────────────────────────────────────
// Lazily loaded on first carousel open, not at app startup — a
// missing assets\help folder shouldn't affect anything before Help is
// ever opened. Cached for the process lifetime once loaded (9 small
// PNGs). A null entry after a load attempt means that file is
// missing/unloadable; the draw function falls back to a placeholder
// frame for that step rather than failing, which is what lets the
// whole layout be verified today, before any real screenshots exist.
static Gdiplus::Bitmap* g_helpStepBitmaps[HELP_STEP_COUNT] = { nullptr };
static bool g_helpStepBitmapLoadAttempted[HELP_STEP_COUNT] = { false };
static std::wstring g_helpAssetsDir; // exe-relative assets\help\, resolved once

// Resolves (once) the exe's own directory via GetModuleFileNameW —
// not the working directory, which shifts depending on how the app
// was launched (double-click vs. shortcut vs. IDE) and would silently
// break a relative path. Loose-folder dev-mode loading only, per
// spec §4; the embedded-RCDATA path for final builds is explicitly
// out of scope for this pass.
static const std::wstring& helpAssetsDir() {
    if (g_helpAssetsDir.empty()) {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring path = exePath;
        size_t lastSlash = path.find_last_of(L"\\/");
        std::wstring exeDir = (lastSlash != std::wstring::npos)
            ? path.substr(0, lastSlash) : L".";
        g_helpAssetsDir = exeDir + L"\\assets\\help\\";
    }
    return g_helpAssetsDir;
}

// Lazy-loads (and caches) the bitmap for one step. Returns nullptr if
// the step has no image (closing card) or the file is missing/
// unloadable — callers must handle that as "draw a placeholder", not
// an error.
static Gdiplus::Bitmap* helpStepBitmap(int step) {
    if (step < 0 || step >= HELP_STEP_COUNT) return nullptr;
    if (HELP_STEPS[step].isClosingCard) return nullptr;
    if (g_helpStepBitmapLoadAttempted[step]) return g_helpStepBitmaps[step];

    g_helpStepBitmapLoadAttempted[step] = true;
    std::wstring fullPath = helpAssetsDir() + HELP_STEPS[step].imageFile;
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(fullPath.c_str(), FALSE);
    if (bmp && bmp->GetLastStatus() == Gdiplus::Ok) {
        g_helpStepBitmaps[step] = bmp;
    } else {
        delete bmp; // FromFile can return a non-null Bitmap in an Ok!=status state
        g_helpStepBitmaps[step] = nullptr;
    }
    return g_helpStepBitmaps[step];
}

// Click-target rects, rebuilt each paint — same per-item pattern as
// every other hit-rect vector in this file. Also doubles as the
// keyboard focus ring's target (Next/Skip) for Tab parity with
// AppModal.
RECT g_helpCarouselCloseRect = {};
RECT g_helpCarouselPrevRect = {};
RECT g_helpCarouselNextRect = {};
RECT g_helpCarouselSkipRect = {};
RECT g_helpCarouselDotRects[HELP_STEP_COUNT] = {};

// ── Open / close / navigation ──────────────────────────────────────
void openHelpCarousel() {
    // Mutual exclusion with AppModal, by construction rather than
    // coincidence — see the planning discussion.
    if (g_appModal.kind != MODAL_NONE) return;
    g_helpCarouselState = HelpCarouselState::Open;
    g_helpCarouselStep = 0;
    g_helpCarouselFocusOnNext = true;
    g_helpCarouselStepShownTick = GetTickCount();
    g_helpCarouselTransitionDir = 1;
    if (g_hContent) {
        SetTimer(g_hContent, TIMER_HELP_CAROUSEL, 16, nullptr);
        InvalidateRect(g_hContent, nullptr, FALSE);
    }
}

void closeHelpCarousel() {
    g_helpCarouselState = HelpCarouselState::Closed;
    if (g_hContent) {
        KillTimer(g_hContent, TIMER_HELP_CAROUSEL);
        InvalidateRect(g_hContent, nullptr, FALSE);
    }
}

// dir: +1 advancing, -1 going back — drives the cross-fade/slide
// transition direction (drawn in the next piece).
void helpCarouselGoToStep(int step, int dir) {
    if (step < 0) step = 0;
    if (step >= HELP_STEP_COUNT) step = HELP_STEP_COUNT - 1;
    if (step == g_helpCarouselStep) return;
    g_helpCarouselTransitionDir = dir;
    g_helpCarouselStep = step;
    g_helpCarouselStepShownTick = GetTickCount();
    if (g_hContent) InvalidateRect(g_hContent, nullptr, FALSE);
}

void helpCarouselNext() {
    if (g_helpCarouselStep >= HELP_STEP_COUNT - 1) {
        closeHelpCarousel(); // "Done" on the last step
    } else {
        helpCarouselGoToStep(g_helpCarouselStep + 1, 1);
    }
}

void helpCarouselPrev() {
    helpCarouselGoToStep(g_helpCarouselStep - 1, -1);
}

// Spec §6: "Skip ... jumps straight to Close." Read as jumping to the
// closing card (step 10 — Done still closes from there), not closing
// outright, since that's the step carrying the Classroom-sync and
// audit-log mentions the spec explicitly wants surfaced even for
// someone skipping the walkthrough. Flagging this reading explicitly
// in case the intent was closing immediately instead.
void helpCarouselSkip() {
    helpCarouselGoToStep(HELP_STEP_COUNT - 1, 1);
}

// Draws one emphasis ring over the loaded screenshot (spec §5). Two
// phases, both driven off a single elapsed-time value (see
// g_helpCarouselStepShownTick — a pure function of elapsed time, not
// a frame counter, so it can't drift and correctly resets whenever
// the step changes):
//  - 0-400ms: a clockwise "draw-in" sweep. GDI+ has no native partial-
//    path-stroke primitive (the CSS stroke-dashoffset equivalent), so
//    this flattens the squircle into line segments (GraphicsPath::
//    Flatten), walks them to find the cumulative length, and strokes
//    only the point prefix up to elapsed/400ms of that total length —
//    the practical GDI+ equivalent of a dash-offset sweep.
//  - After 400ms: full ring, alpha pulsing 100%->70%->100% as a
//    triangle wave over (elapsed-400) mod 1800ms — the one place in
//    the app allowed to loop indefinitely (see TIMER_HELP_CAROUSEL's
//    own comment for why).
// frameX/frameY are the image frame's top-left in screen space; ring
// coordinates are relative to that frame, matching how they're
// authored in HELP_STEPS.
static void drawHelpCarouselRing(HDC hdcScreen, const RingSpec& ring,
                                   int frameX, int frameY, DWORD elapsedMs)
{
    int rx = frameX + S(ring.cx) - S(ring.w) / 2 - S(6);
    int ry = frameY + S(ring.cy) - S(ring.h) / 2 - S(6);
    int rw = S(ring.w) + S(12);
    int rh = S(ring.h) + S(12);
    float radius = 0.4f * (float)(std::min)(rw, rh) / 2.0f;

    Graphics g(hdcScreen);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    BYTE alphaB;
    if (elapsedMs < 400) {
        alphaB = 255;
    } else {
        DWORD pulseT = (elapsedMs - 400) % 1800;
        float phase = (float)pulseT / 1800.0f; // 0..1 over the loop
        float amt = (phase < 0.5f) ? (phase * 2.0f) : (2.0f - phase * 2.0f); // 0..1..0
        float alphaF = 1.0f - amt * 0.30f; // 100% down to 70% and back
        alphaB = (BYTE)(alphaF * 255.0f);
    }

    Color goldColor(alphaB, GetRValue(GOLD_400), GetGValue(GOLD_400), GetBValue(GOLD_400));
    // 2.5px design-space stroke — computed directly from g_dpiScale
    // rather than routed through the integer-rounding S() helper,
    // since a stroke width benefits from the sub-pixel precision a
    // float Pen width allows.
    Pen ringPen(goldColor, (float)(2.5 * g_dpiScale));

    GraphicsPath* basePath = makeRoundRectPath((float)rx, (float)ry, (float)rw, (float)rh, radius);

    if (elapsedMs >= 400) {
        g.DrawPath(&ringPen, basePath);
    } else {
        GraphicsPath* flatPath = basePath->Clone();
        flatPath->Flatten(nullptr, 0.5f);
        int count = flatPath->GetPointCount();
        if (count > 1) {
            std::vector<PointF> pts(count);
            flatPath->GetPathPoints(pts.data(), count);

            std::vector<float> cum(count, 0.0f);
            for (int i = 1; i < count; ++i) {
                float dx = pts[i].X - pts[i-1].X;
                float dy = pts[i].Y - pts[i-1].Y;
                cum[i] = cum[i-1] + sqrtf(dx*dx + dy*dy);
            }
            float total = cum[count - 1];
            float progress = (float)elapsedMs / 400.0f;
            if (progress > 1.0f) progress = 1.0f;
            float cutoff = total * progress;

            int lastIdx = 0;
            while (lastIdx < count - 1 && cum[lastIdx + 1] <= cutoff) ++lastIdx;

            std::vector<PointF> drawPts(pts.begin(), pts.begin() + lastIdx + 1);
            if (lastIdx < count - 1 && cum[lastIdx + 1] > cum[lastIdx]) {
                float segFrac = (cutoff - cum[lastIdx]) / (cum[lastIdx + 1] - cum[lastIdx]);
                PointF interp(
                    pts[lastIdx].X + (pts[lastIdx+1].X - pts[lastIdx].X) * segFrac,
                    pts[lastIdx].Y + (pts[lastIdx+1].Y - pts[lastIdx].Y) * segFrac);
                drawPts.push_back(interp);
            }
            if (drawPts.size() >= 2) {
                g.DrawLines(&ringPen, drawPts.data(), (int)drawPts.size());
            }
        }
        delete flatPath;
    }
    delete basePath;

    // Leader + tag (spec §5) — only once the ring has fully swept in,
    // so the tag doesn't point at an incomplete ring mid-animation.
    if (!ring.tag.empty() && elapsedMs >= 400) {
        int tagX = rx + rw + S(14);
        int midY = ry + rh / 2;

        HPEN leaderPen = CreatePen(PS_SOLID, 1, GOLD_400);
        HPEN oldPen = (HPEN)SelectObject(hdcScreen, leaderPen);
        MoveToEx(hdcScreen, rx + rw, midY, nullptr);
        LineTo(hdcScreen, tagX, midY);
        SelectObject(hdcScreen, oldPen);
        DeleteObject(leaderPen);

        HFONT oldFont = (HFONT)SelectObject(hdcScreen, g_hFontMonoSm);
        SIZE tagSz;
        GetTextExtentPoint32W(hdcScreen, ring.tag.c_str(), (int)ring.tag.size(), &tagSz);
        int tagW = tagSz.cx + S(16);
        int tagH = S(22);
        RECT tagR = {tagX, midY - tagH/2, tagX + tagW, midY + tagH/2};

        SetBkMode(hdcScreen, TRANSPARENT);
        HBRUSH tagBg = CreateSolidBrush(GRAY_850);
        FillRect(hdcScreen, &tagR, tagBg);
        DeleteObject(tagBg);
        SetTextColor(hdcScreen, GOLD_400);
        DrawTextW(hdcScreen, ring.tag.c_str(), -1, &tagR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdcScreen, oldFont);
    }
}

// Dot-row grouping (spec §6): 1/1/2/2/2/1/1 — load, run, [overview
// x2], [classes x2], [flagged pairs x2], report, closing — with a
// slightly wider gap after each group so the row itself hints at the
// app's shape without a label. True at index i means "extra gap after
// this dot."
static const bool HELP_DOT_EXTRA_GAP_AFTER[HELP_STEP_COUNT] = {
    true, true, false, true, false, true, false, true, true, false
};

// Main carousel draw — scrim + card, same "draw on hdcScreen after
// the main blit" pattern as drawAppModal/drawToasts above. Called
// from ContentProc's WM_PAINT alongside those two.
void drawHelpCarousel(HDC hdcScreen, const RECT& viewport) {
    if (g_helpCarouselState != HelpCarouselState::Open) return;

    {
        Graphics g(hdcScreen);
        g.SetSmoothingMode(SmoothingModeNone);
        SolidBrush scrim(Color(170, 10, 10, 12));
        g.FillRectangle(&scrim, 0, 0, viewport.right, viewport.bottom);
    }

    SetBkMode(hdcScreen, TRANSPARENT);

    const CarouselStep& step = HELP_STEPS[g_helpCarouselStep];
    int pad = S(28);
    int frameW = S(HELP_IMG_FRAME_W_DESIGN);
    int frameH = S(HELP_IMG_FRAME_H_DESIGN);
    int titleH = S(28);
    int captionH = S(46);
    int navH = S(50);

    int modalW = frameW + pad * 2;
    if (modalW > S(680)) modalW = S(680);
    int modalH = pad + S(30) + S(26) + frameH + S(14) + titleH + S(8) +
                 captionH + S(10) + navH + pad;

    int mx = (viewport.right - modalW) / 2;
    int my = (viewport.bottom - modalH) / 2;
    if (my < S(20)) my = S(20);

    RECT modalR = {mx, my, mx + modalW, my + modalH};
    drawCard(hdcScreen, modalR, GRAY_900, GRAY_700, 12, GOLD_500);

    int cy = my + pad;

    // Fixed header: title + close X
    HFONT oldFont = (HFONT)SelectObject(hdcScreen, g_hFontH2);
    SetTextColor(hdcScreen, GOLD_500);
    RECT titleR = {mx + pad, cy, mx + modalW - pad - S(30), cy + S(26)};
    DrawTextW(hdcScreen, L"CALSS Quick Start Guide", -1, &titleR,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdcScreen, g_hFontBodyNew);
    RECT closeR = {mx + modalW - pad - S(24), cy, mx + modalW - pad, cy + S(24)};
    g_helpCarouselCloseRect = closeR;
    SetTextColor(hdcScreen, GRAY_400);
    DrawTextW(hdcScreen, L"\u2715", -1, &closeR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    cy += S(30);

    // "STEP N OF 10" eyebrow — reuses drawEyebrow, already S()-scaled
    // internally, same token as every other section label in the app.
    wchar_t stepBuf[32];
    swprintf(stepBuf, 32, L"STEP %d OF %d", g_helpCarouselStep + 1, HELP_STEP_COUNT);
    drawEyebrow(hdcScreen, mx + pad, cy, modalW - pad * 2, stepBuf);
    cy += S(26);

    // Image frame
    int frameX = mx + (modalW - frameW) / 2;
    int frameY = cy;
    RECT frameR = {frameX, frameY, frameX + frameW, frameY + frameH};
    DWORD elapsed = GetTickCount() - g_helpCarouselStepShownTick;

    HBRUSH letterBg = CreateSolidBrush(GRAY_950);
    FillRect(hdcScreen, &frameR, letterBg);
    DeleteObject(letterBg);

    if (step.isClosingCard) {
        // Centered UL crest + gold checkmark in the same slot so the
        // modal doesn't resize between steps.
        SelectObject(hdcScreen, g_hFontDisplay);
        SetTextColor(hdcScreen, GOLD_500);
        RECT crestR = {frameR.left, frameR.top, frameR.right, frameR.top + frameH / 2};
        DrawTextW(hdcScreen, L"UL", -1, &crestR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        int cx = (frameR.left + frameR.right) / 2;
        int checkY = frameR.top + frameH / 2 + S(40);
        HPEN checkPen = CreatePen(PS_SOLID, S(3), GOLD_500);
        HPEN oldCP = (HPEN)SelectObject(hdcScreen, checkPen);
        MoveToEx(hdcScreen, cx - S(30), checkY, nullptr);
        LineTo(hdcScreen, cx - S(8), checkY + S(22));
        LineTo(hdcScreen, cx + S(34), checkY - S(24));
        SelectObject(hdcScreen, oldCP);
        DeleteObject(checkPen);
    } else {
        Gdiplus::Bitmap* bmp = helpStepBitmap(g_helpCarouselStep);
        if (bmp) {
            Graphics g(hdcScreen);
            g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            // Fit within the frame preserving aspect; whatever's left
            // over is the letterbox already filled above.
            float bw = (float)bmp->GetWidth(), bh = (float)bmp->GetHeight();
            float frameAspect = (float)frameW / (float)frameH;
            float imgAspect = (bh > 0) ? (bw / bh) : frameAspect;
            RectF dest;
            if (imgAspect > frameAspect) {
                float dh = frameW / imgAspect;
                dest = RectF((float)frameX, (float)frameY + (frameH - dh) / 2.0f,
                             (float)frameW, dh);
            } else {
                float dw = frameH * imgAspect;
                dest = RectF((float)frameX + (frameW - dw) / 2.0f, (float)frameY,
                             dw, (float)frameH);
            }
            g.DrawImage(bmp, dest);
        } else {
            // No screenshot yet — placeholder text, exactly what lets
            // the whole layout be checked before any captures exist.
            SelectObject(hdcScreen, g_hFontBodyNew);
            SetTextColor(hdcScreen, GRAY_500);
            RECT phR = frameR;
            DrawTextW(hdcScreen, L"[ screenshot not yet captured ]", -1, &phR,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        // Ring overlay — on top of the image, drawn from data; never
        // baked into the bitmap (spec §5 / §7).
        for (const auto& ring : step.rings) {
            drawHelpCarouselRing(hdcScreen, ring, frameX, frameY, elapsed);
        }
    }

    HPEN borderPen = CreatePen(PS_SOLID, 1, GRAY_700);
    HPEN oldBP = (HPEN)SelectObject(hdcScreen, borderPen);
    HBRUSH nullBr = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldBB = (HBRUSH)SelectObject(hdcScreen, nullBr);
    Rectangle(hdcScreen, frameR.left, frameR.top, frameR.right, frameR.bottom);
    SelectObject(hdcScreen, oldBP); SelectObject(hdcScreen, oldBB);
    DeleteObject(borderPen);
    cy += frameH + S(14);

    // Per-step title
    SelectObject(hdcScreen, g_hFontH2);
    SetTextColor(hdcScreen, WHITE_);
    RECT stepTitleR = {mx + pad, cy, mx + modalW - pad, cy + titleH};
    DrawTextW(hdcScreen, step.title.c_str(), -1, &stepTitleR,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    cy += titleH + S(8);

    // Caption — 2 lines max per spec §6
    SelectObject(hdcScreen, g_hFontBodyNew);
    SetTextColor(hdcScreen, GRAY_200);
    RECT capR = {mx + pad, cy, mx + modalW - pad, cy + captionH};
    DrawTextW(hdcScreen, step.caption.c_str(), -1, &capR, DT_LEFT | DT_WORDBREAK);
    cy += captionH + S(10);

    // ── Nav row: ‹  dots  ...  Skip  Next/Done ──
    // Note: the spec's mockup shows "‹ ... Skip Next ›" with a
    // trailing chevron next to Next. Implemented here as "Next ›" /
    // "Done ›" being the primary button's own label (a single hit
    // target with a trailing arrow glyph, same convention as many
    // "Next →" buttons), with only the LEFT chevron as a separate
    // icon-only Previous button — this reads the same visually but
    // avoids two adjacent controls doing the identical "advance"
    // action. Flagging this reading explicitly in case a fully
    // separate icon-only Next chevron was intended instead.
    int navY = cy;
    int chevW = S(28);

    bool canGoPrev = (g_helpCarouselStep > 0);
    RECT prevR = {mx + pad, navY, mx + pad + chevW, navY + navH};
    g_helpCarouselPrevRect = canGoPrev ? prevR : RECT{0, 0, 0, 0};
    SetTextColor(hdcScreen, canGoPrev ? GRAY_200 : GRAY_700);
    DrawTextW(hdcScreen, L"\u2039", -1, &prevR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Dots — grouped per HELP_DOT_EXTRA_GAP_AFTER
    int dotX = prevR.right + S(10);
    int dotY = navY + navH / 2;
    {
        Graphics g(hdcScreen);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        for (int i = 0; i < HELP_STEP_COUNT; ++i) {
            bool cur = (i == g_helpCarouselStep);
            int dSize = cur ? S(8) : S(6);
            RECT dotHit = {dotX, dotY - S(9), dotX + S(14), dotY + S(9)};
            g_helpCarouselDotRects[i] = dotHit;

            COLORREF dc = cur ? GOLD_500 : GRAY_700;
            SolidBrush sb(Color(255, GetRValue(dc), GetGValue(dc), GetBValue(dc)));
            int dx = dotX + (S(14) - dSize) / 2, dy = dotY - dSize / 2;
            g.FillEllipse(&sb, dx, dy, dSize, dSize);

            dotX += S(14);
            if (HELP_DOT_EXTRA_GAP_AFTER[i]) dotX += S(8);
        }
    }

    // Skip — only visible before the last step
    RECT skipR = {0, 0, 0, 0};
    if (g_helpCarouselStep < HELP_STEP_COUNT - 1) {
        SelectObject(hdcScreen, g_hFontBodyNew);
        SIZE skipSz; GetTextExtentPoint32W(hdcScreen, L"Skip", 4, &skipSz);
        int skipX = mx + modalW - pad - S(150) - skipSz.cx - S(16);
        skipR = {skipX, navY, skipX + skipSz.cx + S(16), navY + navH};
        SetTextColor(hdcScreen, GRAY_400);
        DrawTextW(hdcScreen, L"Skip", -1, &skipR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    g_helpCarouselSkipRect = skipR;

    // Next / Done, primary maroon-700, trailing chevron in the label
    bool isLast = (g_helpCarouselStep == HELP_STEP_COUNT - 1);
    std::wstring nextLabel = isLast ? L"Done \u203A" : L"Next \u203A";
    SelectObject(hdcScreen, g_hFontBodyNew);
    SIZE nextSz;
    GetTextExtentPoint32W(hdcScreen, nextLabel.c_str(), (int)nextLabel.size(), &nextSz);
    int nextW = nextSz.cx + S(28);
    int nextX = mx + modalW - pad - nextW;
    int btnH = navH - S(10);
    RECT nextR = {nextX, navY + (navH - btnH) / 2, nextX + nextW, navY + (navH - btnH) / 2 + btnH};
    g_helpCarouselNextRect = nextR;
    drawCard(hdcScreen, nextR, MAROON_700, MAROON_700, 8);
    SetTextColor(hdcScreen, WHITE_);
    DrawTextW(hdcScreen, nextLabel.c_str(), -1, &nextR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Keyboard focus ring — Tab-toggle parity with AppModal, per
    // sign-off. Only Skip/Next participate (Prev/dots/close stay
    // mouse-only, matching AppModal's own primary/secondary-only scope).
    if (g_focusVisible) {
        bool skipExists = (skipR.right > skipR.left);
        bool onNext = g_helpCarouselFocusOnNext || !skipExists;
        drawFocusRing(hdcScreen, onNext ? nextR : skipR);
    }

    SelectObject(hdcScreen, oldFont);
}

#endif // _WIN32