// ─────────────────────────────────────────────────────────────
// main.cpp - Code Authorship Likelihood Scoring System
// Entry point: launches the GUI. The analysis pipeline lives in
// analysis_pipeline.cpp, the per-student report in student_report.cpp.
// ─────────────────────────────────────────────────────────────

#include "gui.h"

#include <windows.h>

int main() {
    return runGUI(GetModuleHandle(nullptr));
}
