#ifndef GUI_H
#define GUI_H
// ─────────────────────────────────────────────────────────────
// gui.h
// Win32 GUI for Code Authorship Likelihood Scoring System
// ─────────────────────────────────────────────────────────────

#ifdef _WIN32

#include <windows.h>
#include <string>

// Initialize and run the GUI application
// Returns exit code from message loop
int runGUI(HINSTANCE hInstance);

#endif // _WIN32

#endif // GUI_H