#pragma once
// ─────────────────────────────────────────────────────────────
// CALSS version — the single source of truth.
//
// Read by the Help > About text (gui.cpp), the exe's Windows version
// resource (res/calss.rc) and, through that resource, the installer
// (installer/calss.iss reads the version back out of authorship.exe).
// Change it here only. Plain C preprocessor so windres can include it.
//
// 1.0.0 = first installer release of the thesis build (Phase 5,
// 2026-10-07). Bump the last number for a fix-only rebuild, the middle
// one for a release that adds or changes features.
// ─────────────────────────────────────────────────────────────

#define CALSS_VERSION_MAJOR 1
#define CALSS_VERSION_MINOR 0
#define CALSS_VERSION_PATCH 0

#define CALSS_STR2_(x) #x
#define CALSS_STR_(x)  CALSS_STR2_(x)

// "1.0.0"
#define CALSS_VERSION_STR \
    CALSS_STR_(CALSS_VERSION_MAJOR) "." CALSS_STR_(CALSS_VERSION_MINOR) "." \
    CALSS_STR_(CALSS_VERSION_PATCH)

#define CALSS_RELEASE_LABEL "Thesis Release"

// Resource ID of the application icon (res/calss.rc).
#define IDI_CALSS 101
