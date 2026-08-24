// ─────────────────────────────────────────────────────────────
// main.cpp - Code Authorship Likelihood Scoring System
// Generates an HTML report of similarity + authorship results
// ─────────────────────────────────────────────────────────────

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <functional>
#include <iomanip>

#include "tokenizer.h"
#include "features.h"
#include "knn_model.h"
#include "similarity.h"
#include "gemini.h"
#include "gui.h"

#ifdef _WIN32
  #include <windows.h>
#endif

// ═════════════════════════════════════════════════════════════
// FILE UTILITIES
// ═════════════════════════════════════════════════════════════

static std::string extractStudentName(const std::string& filename) {
    std::string name = filename;
    size_t dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot);

    // New-style Classroom-synced files: "{period}{type}__{studentPortion}".
    // Everything after the LAST "__" is the student-identifying portion —
    // split here first so the category tag (which may itself contain
    // digits or category words) never leaks into the extracted name.
    {
        size_t delimPos = name.rfind("__");
        if (delimPos != std::string::npos) {
            name = name.substr(delimPos + 2);
        }
    }

    // Strip a trailing "_ASGNXXXXXX" assignment-disambiguation fingerprint
    // if present (added by gc_downloadSubmissions() in google_classroom.cpp
    // to keep Classroom-synced filenames unique across different
    // assignments in the same category — e.g. Midterm Exam vs Final Exam
    // both categorize as "exam" and would otherwise collide on disk).
    // "_ASGN" is deliberately distinctive (uppercase, unlikely to occur
    // in any real student name) so this can never misfire on legitimate
    // filenames. This must run BEFORE prefix stripping since the
    // fingerprint sits at the very end, after the student name.
    {
        size_t fpPos = name.rfind("_ASGN");
        if (fpPos != std::string::npos) {
            name = name.substr(0, fpPos);
        }
    }

    // Also strip a numbered multi-attachment suffix like "_2", "_3"
    // (added when a single submission has multiple .c attachments).
    {
        size_t lastUnderscore = name.rfind('_');
        if (lastUnderscore != std::string::npos) {
            std::string tail = name.substr(lastUnderscore + 1);
            bool allDigits = !tail.empty();
            for (char c : tail) {
                if (!isdigit((unsigned char)c)) { allDigits = false; break; }
            }
            if (allDigits) {
                name = name.substr(0, lastUnderscore);
            }
        }
    }

    const std::string prefixes[] = {
        "finalexam","finals","final","semis","prelims","midterms","semi",
        "prelim","midterm","mid","activity","act","quiz","hw","exam"
    };
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& pfx : prefixes) {
            if (name.size() > pfx.size()) {
                std::string lower = name.substr(0, pfx.size());
                std::transform(lower.begin(), lower.end(),
                               lower.begin(), ::tolower);
                if (lower == pfx) {
                    std::string rest = name.substr(pfx.size());
                    size_t i = 0;
                    while (i < rest.size() &&
                           isdigit((unsigned char)rest[i])) ++i;
                    name = rest.substr(i);
                    changed = true;
                    break;
                }
            }
        }
    }
    size_t start = name.find_first_not_of("_- ");
    return (start != std::string::npos) ? name.substr(start) : filename;
}

// ─────────────────────────────────────────────────────────────
// File categorization — priority hierarchy
//
// Design rationale: rather than guessing a submission's category by
// executing student code and comparing output (unreliable, and a
// security risk — see Chapter 3 methodology notes), CALSS uses a
// deterministic priority hierarchy over signals that are already
// available and instructor-controlled:
//
//   Priority 1 — Classroom assignment title, embedded by
//                gc_downloadSubmissions() as a filename prefix
//                (e.g. "finalexamCarlos.c"). This is the most
//                reliable signal because it comes directly from
//                what the instructor named the assignment in
//                Google Classroom.
//
//   Priority 2 — Filename keyword pattern, for manually imported
//                files that never passed through Classroom sync.
//                Checked in most-likely-first order: activities
//                are far more common than exams in a typical
//                semester, so activity/quiz patterns are checked
//                before exam patterns purely as a minor
//                short-circuit optimization — the categories
//                themselves are mutually exclusive regardless
//                of check order.
//
//   Priority 3 — Unclassified. Rather than silently treating an
//                unrecognized file as "just another activity"
//                (which could corrupt a student's style profile
//                with unrelated data), unclassified files are
//                excluded from both profile-building and pairwise
//                comparison, and reported separately so the
//                instructor can review them manually.
// ─────────────────────────────────────────────────────────────

enum class FileCategory {
    Exam,
    Quiz,
    Activity,
    Unclassified
};

static FileCategory categorizeFile(const std::string& filename) {
    std::string name = filename;
    size_t dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot);

    // New-style Classroom-synced files encode "{period}{type}__{name}".
    // If the "__" delimiter is present, only examine the category tag
    // portion (before the delimiter) — this avoids accidentally matching
    // a type keyword that happens to appear inside a real student's name.
    size_t delimPos = name.find("__");
    std::string checkPortion = (delimPos != std::string::npos)
        ? name.substr(0, delimPos)
        : name;

    std::string lower = checkPortion;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (delimPos != std::string::npos) {
        // New-style: search within the category tag only.
        if (lower.find("exam") != std::string::npos)     return FileCategory::Exam;
        if (lower.find("quiz") != std::string::npos)     return FileCategory::Quiz;
        if (lower.find("activity") != std::string::npos) return FileCategory::Activity;
        // Tag present but no recognizable type keyword — fall through
        // to the legacy checks below as a last resort.
    }

    // Priority 1 — legacy Classroom-embedded prefix (older sync format,
    // or manually imported files using the same naming convention)
    if (lower.rfind("finalexam", 0) == 0 || lower.rfind("exam", 0) == 0)
        return FileCategory::Exam;
    if (lower.rfind("quiz", 0) == 0)
        return FileCategory::Quiz;
    if (lower.rfind("activity", 0) == 0)
        return FileCategory::Activity;

    // Priority 2 — filename keyword fallback (manually imported files)
    if (lower.find("activity") != std::string::npos ||
        lower.find("seatwork") != std::string::npos ||
        lower.find("lab")      != std::string::npos)
        return FileCategory::Activity;
    if (lower.find("quiz") != std::string::npos)
        return FileCategory::Quiz;
    if (lower.find("exam")   != std::string::npos ||
        lower.find("finals") != std::string::npos)
        return FileCategory::Exam;

    // Priority 3 — no reliable signal found
    return FileCategory::Unclassified;
}

// Extracts which grading period (Prelim/Midterm/Semifinal/Final) a file
// belongs to, based on the "{period}{type}__{name}" tag embedded by
// gc_downloadSubmissions() during Classroom sync. Files with no such tag
// (manually imported, or locally organized into per-period folders)
// fall into a single "general" bucket — this preserves prior behavior
// for datasets that already achieve period separation via folder
// structure rather than filename encoding.
//
// This exists specifically so pairwise comparison can avoid comparing
// exams from different periods against each other (e.g. a Midterm Exam
// covers different material than a Final Exam — comparing them produces
// meaningless similarity noise, not useful authorship evidence).
static std::string extractPeriod(const std::string& filename) {
    std::string name = filename;
    size_t dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot);

    size_t delimPos = name.find("__");
    if (delimPos == std::string::npos) return "general";

    std::string tag = name.substr(0, delimPos);
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);

    // Check "semifinal" before "final" since it contains "final" as a substring.
    if (tag.find("semifinal") != std::string::npos) return "semifinal";
    if (tag.find("midterm")   != std::string::npos) return "midterm";
    if (tag.find("final")     != std::string::npos) return "final";
    if (tag.find("prelim")    != std::string::npos) return "prelim";
    return "general";
}

// Legacy helper retained for compatibility — treats Exam category
// as the "exam" bucket used throughout the pairwise-comparison logic.
static bool isExamFile(const std::string& filename) {
    return categorizeFile(filename) == FileCategory::Exam;
}

// ─────────────────────────────────────────────────────────────
// Language validation
//
// The tokenizer and feature extractor don't verify a file is actually
// C — they just extract surface-level text patterns from whatever is
// there. A student who submits Java (or any other language) as a .c
// file will still tokenize, still produce 14 feature values, and
// still show up in results — just with meaningless numbers, since
// none of it reflects genuine C authorship style.
//
// This matters most for exam files: comparing a Java submission
// against a student's real C-based profile would likely produce a
// LOW authorship match — which is technically correct (it doesn't
// match their style) but for the wrong underlying reason. An
// instructor seeing "Low Likelihood" could easily misread that as a
// ghostwriting signal rather than "wrong language entirely."
//
// This check is intentionally a lightweight text scan, not a parser
// or compiler — consistent with the earlier decision not to execute
// student code (see Chapter 3 methodology notes on why AI-execution-
// based categorization was rejected). It looks for structural markers
// that are simply impossible in valid C, so false positives are rare.
// ─────────────────────────────────────────────────────────────

struct LanguageCheck {
    bool        isPlausibleC;
    std::string suspectedLanguage; // empty if isPlausibleC is true
};

static LanguageCheck checkLanguage(const std::string& filepath) {
    LanguageCheck result{true, ""};

    std::ifstream f(filepath);
    if (!f.is_open()) return result; // can't read it — don't block on this

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    // Java — public class / System.out / import java.*
    if (content.find("public class ") != std::string::npos ||
        content.find("System.out.println") != std::string::npos ||
        content.find("System.out.print(") != std::string::npos ||
        content.find("import java.") != std::string::npos ||
        content.find("public static void main(String") != std::string::npos) {
        result.isPlausibleC = false;
        result.suspectedLanguage = "Java";
        return result;
    }

    // Python — def/elif, f-strings, or no #include with Python-style print
    if ((content.find("def ") != std::string::npos &&
         content.find("#include") == std::string::npos) ||
        content.find("elif ") != std::string::npos ||
        content.find("print(f\"") != std::string::npos ||
        content.find("import numpy") != std::string::npos) {
        result.isPlausibleC = false;
        result.suspectedLanguage = "Python";
        return result;
    }

    // JavaScript
    if (content.find("console.log(") != std::string::npos) {
        result.isPlausibleC = false;
        result.suspectedLanguage = "JavaScript";
        return result;
    }

    // C++ — out of scope per Chapter 1 (C only), even though it's a
    // "close" language, mixing it in would still corrupt style
    // comparisons since C++ idioms differ meaningfully from C.
    if (content.find("using namespace std") != std::string::npos ||
        content.find("cout <<") != std::string::npos ||
        content.find("cin >>") != std::string::npos) {
        result.isPlausibleC = false;
        result.suspectedLanguage = "C++";
        return result;
    }

    return result; // no disqualifying signal found — treat as plausible C
}

// Whether a file should be used to build a student's style profile.
// Only Activity and Quiz submissions build the profile — Exam files
// are reserved for comparison, and Unclassified files are excluded
// entirely so they cannot silently corrupt a profile.
static bool isProfileBuildingFile(const std::string& filename) {
    FileCategory cat = categorizeFile(filename);
    return cat == FileCategory::Activity || cat == FileCategory::Quiz;
}

// Records that a manually-placed file belongs to a given block, based
// purely on which subfolder it was found in — no dependency on
// google_classroom.h, kept self-contained since main.cpp doesn't (and
// shouldn't need to) know about Classroom internals. This writes to the
// SAME manifest file Classroom sync uses, so the Students tab's
// block-grouping logic works identically regardless of where a file's
// block information came from.
static void autoTagManualBlock(const std::string& rootFolder,
                                  const std::string& bareFilename,
                                  const std::string& blockName) {
    std::string manifestPath = rootFolder + "\\calss_sync_manifest.jsonl";

    // Skip if this file already has a manifest entry (e.g. it was
    // synced via Classroom directly into this subfolder, or analysis
    // already ran on this folder before) — avoids duplicate/stale lines.
    {
        std::ifstream check(manifestPath);
        if (check.is_open()) {
            std::string line;
            std::string searchKey = "\"file\":\"" + bareFilename + "\"";
            while (std::getline(check, line)) {
                if (line.find(searchKey) != std::string::npos) return;
            }
        }
    }

    std::ofstream f(manifestPath, std::ios::app);
    if (!f.is_open()) return;
    f << "{\"file\":\"" << bareFilename << "\","
      << "\"title\":\"Manually Imported\","
      << "\"block\":\"" << blockName << "\"}\n";
}

static std::map<std::string,std::string> loadFolder(
    const std::string& folder)
{
    std::map<std::string,std::string> files;
#ifdef _WIN32
    // Level 1: files directly in the root data folder. This is the
    // existing behavior — Classroom-synced files land here, tagged
    // with their block via the manifest (not via physical folders).
    {
        WIN32_FIND_DATAA fd;
        std::string pattern = folder + "\\*.c";
        HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string fname = fd.cFileName;
                std::string key   = fname.substr(0, fname.rfind('.'));
                files[key] = folder + "\\" + fname;
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
    }

    // Level 2: one level of subfolders (e.g. "BSCS", "BSIT_Block1").
    // This is specifically for manually-organized submissions — an
    // instructor can create a subfolder per course/block and drop
    // downloaded .c files in directly. The subfolder name automatically
    // becomes that file's block for grouping purposes, with zero extra
    // configuration required.
    //
    // Note: this does NOT recurse further than one level, and files
    // sharing an identical bare filename across two different
    // subfolders will collide in this map (last one found wins). This
    // is a known, low-probability edge case — instructors should
    // include a distinguishing student identifier (full name) in
    // manually-assigned filenames, which is already good practice and
    // avoids the collision entirely.
    {
        WIN32_FIND_DATAA fd;
        std::string dirPattern = folder + "\\*";
        HANDLE hFind = FindFirstFileA(dirPattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                std::string subName = fd.cFileName;
                if (subName == "." || subName == "..") continue;

                std::string subFolder = folder + "\\" + subName;
                std::string subPattern = subFolder + "\\*.c";

                WIN32_FIND_DATAA fd2;
                HANDLE hFind2 = FindFirstFileA(subPattern.c_str(), &fd2);
                if (hFind2 != INVALID_HANDLE_VALUE) {
                    do {
                        std::string fname = fd2.cFileName;
                        std::string key   = fname.substr(0, fname.rfind('.'));
                        files[key] = subFolder + "\\" + fname;
                        autoTagManualBlock(folder, fname, subName);
                    } while (FindNextFileA(hFind2, &fd2));
                    FindClose(hFind2);
                }
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
    }
#else
    // Linux/Mac fallback (flat only — development/testing environment)
    FILE* pipe = popen(("ls " + folder + "/*.c 2>/dev/null").c_str(), "r");
    if (!pipe) return files;
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe)) {
        std::string path = buf;
        while (!path.empty() && (path.back()=='\n'||path.back()=='\r'))
            path.pop_back();
        size_t slash = path.rfind('/');
        std::string fname = (slash==std::string::npos) ? path : path.substr(slash+1);
        std::string key   = fname.substr(0, fname.rfind('.'));
        files[key] = path;
    }
    pclose(pipe);
#endif
    return files;
}

// ═════════════════════════════════════════════════════════════
// HTML GENERATION
// ═════════════════════════════════════════════════════════════

static std::string scoreColor(double pct) {
    if (pct >= 80) return "#4caf7d";
    if (pct >= 60) return "#f0a500";
    return "#e05c5c";
}

static std::string simColor(double pct) {
    if (pct >= 80) return "#e05c5c";
    if (pct >= 60) return "#f0a500";
    return "#4caf7d";
}

static std::string htmlReport(
    const std::vector<PairResult>& allPairs,
    const SummaryStats& stats,
    const std::vector<PairResult>& flagged,
    AuthorshipScorer& scorer,
    const std::string& dataFolder,
    const AISummary& aiSummary)
{
    std::ostringstream h;
    time_t now = time(nullptr);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%B %d, %Y  %H:%M", localtime(&now));

    h << R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Code Authorship Likelihood Report</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:'Segoe UI',Arial,sans-serif;background:#0f0f1a;color:#e0e0f0;min-height:100vh}
  .header{background:linear-gradient(135deg,#7c6af7,#4c5af0);padding:32px 40px;border-bottom:3px solid #5a4fd0}
  .header h1{font-size:1.7rem;font-weight:700;letter-spacing:.5px}
  .header p{opacity:.85;margin-top:6px;font-size:.95rem}
  .meta{display:flex;gap:16px;margin-top:14px;flex-wrap:wrap}
  .meta span{background:rgba(255,255,255,.15);padding:4px 12px;border-radius:20px;font-size:.82rem}
  .container{max-width:1100px;margin:0 auto;padding:32px 20px}
  .stats-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:14px;margin-bottom:32px}
  .stat-card{background:#1e1e2e;border:1px solid #2a2a4a;border-radius:10px;padding:20px;text-align:center}
  .stat-card .val{font-size:2rem;font-weight:700}
  .stat-card .lbl{font-size:.78rem;color:#9090b0;margin-top:4px;text-transform:uppercase;letter-spacing:.5px}
  .section-title{font-size:1.1rem;font-weight:600;color:#a0a0e0;margin:28px 0 14px;padding-bottom:8px;border-bottom:1px solid #2a2a4a}
  .pair-card{background:#1a1a2e;border:1px solid #2a2a4a;border-radius:10px;margin-bottom:16px;overflow:hidden}
  .pair-header{display:flex;align-items:center;justify-content:space-between;padding:14px 20px;background:#1e1e3e;flex-wrap:wrap;gap:8px}
  .pair-names{font-weight:600;font-size:1rem}
  .pair-badge{padding:4px 14px;border-radius:20px;font-size:.82rem;font-weight:600;color:#fff}
  .score-row{display:flex;gap:16px;padding:10px 20px;background:#161626;flex-wrap:wrap}
  .score-item{font-size:.82rem;color:#9090b0}
  .score-item span{font-weight:600;color:#c0c0e0}
  .pair-body{padding:16px 20px}
  .students-grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}
  @media(max-width:600px){.students-grid{grid-template-columns:1fr}}
  .student-card{background:#12121f;border:1px solid #2a2a4a;border-radius:8px;padding:14px}
  .student-name{font-weight:600;margin-bottom:6px;font-size:.95rem}
  .auth-score{font-size:1.5rem;font-weight:700;margin-bottom:4px}
  .auth-label{font-size:.78rem;color:#9090b0;margin-bottom:10px}
  .progress-bg{background:#0a0a18;border-radius:4px;height:8px;margin-bottom:12px}
  .progress-fill{height:8px;border-radius:4px;transition:width .3s}
  .style-list{font-size:.78rem;line-height:1.8}
  .match{color:#4caf7d}
  .mismatch{color:#e05c5c}
  .verdict-box{background:#0e0e1c;border:1px solid #3a2a6a;border-radius:8px;padding:14px 18px;margin-top:14px}
  .verdict-title{font-size:.78rem;color:#9090b0;text-transform:uppercase;letter-spacing:.5px;margin-bottom:8px}
  .verdict-row{display:flex;gap:10px;align-items:center;margin-bottom:4px;font-size:.88rem}
  .verdict-row .vl{font-weight:600}
  .orig{color:#4caf7d}
  .copy{color:#e05c5c}
  .note{font-size:.75rem;color:#6060a0;margin-top:8px;font-style:italic}
  .profile-section{margin-bottom:32px}
  .profile-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px}
  .profile-card{background:#1a1a2e;border:1px solid #2a2a4a;border-radius:8px;padding:14px}
  .profile-card h4{font-size:.9rem;color:#a0a0e0;margin-bottom:8px}
  .profile-card .sub-count{font-size:.78rem;color:#6060a0;margin-bottom:8px}
  .style-note{font-size:.78rem;color:#c0c0d0;padding:3px 0;border-bottom:1px solid #1e1e3e}
  .style-note:last-child{border-bottom:none}
  .tag{display:inline-block;background:#2a2a4a;border-radius:4px;padding:1px 6px;font-size:.7rem;color:#8080b0;margin-right:4px}
  .no-flag{color:#4caf7d;text-align:center;padding:20px;background:#121f12;border:1px solid #2a4a2a;border-radius:8px}
  footer{text-align:center;padding:24px;color:#404060;font-size:.78rem;border-top:1px solid #1e1e2e;margin-top:32px}
</style>
</head>
<body>
<div class="header">
  <h1>&#128196; Code Authorship Likelihood Report</h1>
  <p>An AI-Assisted Code Authorship Likelihood Scoring System Using Student-Specific Programming Style Fingerprinting</p>
  <div class="meta">
    <span>&#128197; )"; h << timebuf; h << R"(</span>
    <span>&#128193; )"; h << dataFolder; h << R"(</span>
    <span>University of Luzon &mdash; College of Computer Studies</span>
  </div>
</div>
<div class="container">)";

    // ── AI Generated Summary ──────────────────────────────────
    if (aiSummary.success && !aiSummary.text.empty()) {
        h << "<div style='background:linear-gradient(135deg,#1e1b3a,#2a1e4a);"
          << "border:1px solid #5a4fd0;border-radius:10px;padding:20px 24px;"
          << "margin-bottom:24px;position:relative'>"
          << "<div style='display:flex;align-items:center;gap:8px;margin-bottom:12px'>"
          << "<span style='font-size:1.3rem'>&#129302;</span>"
          << "<span style='font-size:.85rem;font-weight:600;color:#a0a0e0;"
          << "text-transform:uppercase;letter-spacing:1px'>"
          << "AI-Generated Instructor Summary</span>"
          << "<span style='font-size:.7rem;color:#7070a0;margin-left:auto;"
          << "padding:3px 10px;background:#0a0a18;border-radius:12px'>"
          << "Powered by Gemini</span>"
          << "</div>"
          << "<div style='font-size:.9rem;line-height:1.7;color:#d0d0e8;"
          << "white-space:pre-wrap'>"
          << aiSummary.text
          << "</div>"
          << "<div style='margin-top:14px;padding-top:12px;border-top:1px solid #3a2a6a;"
          << "font-size:.72rem;color:#6060a0;font-style:italic'>"
          << "This summary was generated by an AI model based on the statistical "
          << "analysis below. All academic integrity determinations rest with the instructor."
          << "</div>"
          << "</div>";
    } else if (aiSummary.skipped) {
        h << "<div style='background:#1a1a2e;border:1px dashed #3a3a5a;"
          << "border-radius:10px;padding:14px 20px;margin-bottom:24px;"
          << "font-size:.82rem;color:#7070a0;text-align:center'>"
          << "&#9432; AI summary unavailable: " << aiSummary.error
          << "</div>";
    } else if (!aiSummary.success && !aiSummary.error.empty()) {
        h << "<div style='background:#1a1a2e;border:1px dashed #5a3a3a;"
          << "border-radius:10px;padding:14px 20px;margin-bottom:24px;"
          << "font-size:.82rem;color:#a07070'>"
          << "&#9888; AI summary generation failed: " << aiSummary.error
          << "</div>";
    }

    // ── Summary Stats ─────────────────────────────────────────
    h << R"(<div class="stats-grid">)";
    auto statCard = [&](const std::string& val, const std::string& lbl,
                         const std::string& color) {
        h << "<div class='stat-card'>"
          << "<div class='val' style='color:" << color << "'>" << val << "</div>"
          << "<div class='lbl'>" << lbl << "</div></div>";
    };
    statCard(std::to_string(stats.totalPairs),   "Total Pairs",   "#a0a0e0");
    statCard(std::to_string(stats.flaggedCount),  "Flagged",       "#e05c5c");
    statCard(std::to_string(stats.exactCount),    "Exact Dupes",   "#e05c5c");
    statCard(std::to_string(stats.highCount),     "High Sim",      "#f0a500");

    std::ostringstream avgss, maxss;
    avgss.precision(1); avgss << std::fixed << stats.avgScore;
    maxss.precision(1); maxss << std::fixed << stats.maxScore;
    statCard(avgss.str() + "%", "Avg Score",     "#a0a0e0");
    statCard(maxss.str() + "%", "Max Score",     "#e05c5c");
    h << "</div>";

    // ── Student Profiles ──────────────────────────────────────
    h << "<div class='section-title'>&#128100; Student Style Profiles</div>";
    h << "<div class='profile-grid'>";
    for (const auto& name : scorer.readyStudents()) {
        const StudentProfile* p = scorer.getProfile(name);
        if (!p) continue;
        h << "<div class='profile-card'>"
          << "<h4>" << name << "</h4>"
          << "<div class='sub-count'>"
          << p->submissions.size() << " prior submission(s) used</div>";
        for (const auto& note : p->styleNotes) {
            h << "<div class='style-note'>"
              << "<span class='tag'>" << note.feature << "</span>"
              << note.observation << "</div>";
        }
        h << "</div>";
    }
    h << "</div>";

    // ── Flagged Pairs ─────────────────────────────────────────
    h << "<div class='section-title'>&#128680; Flagged Pairs &mdash; Authorship Analysis</div>";

    if (flagged.empty()) {
        h << "<div class='no-flag'>&#10003; No flagged pairs. All submissions appear unique.</div>";
    } else {
        for (const auto& pair : flagged) {
            std::string sA = extractStudentName(pair.studentA);
            std::string sB = extractStudentName(pair.studentB);
            PairVerdict verdict = scorer.scorePair(
                sA, pair.pathA, sB, pair.pathB);

            std::ostringstream css;
            css.precision(1); css << std::fixed << pair.combinedScore;

            h << "<div class='pair-card'>"
              << "<div class='pair-header'>"
              << "<div class='pair-names'>&#128196; "
              << pair.studentA << " &harr; " << pair.studentB << "</div>"
              << "<span class='pair-badge' style='background:"
              << simColor(pair.combinedScore) << "'>"
              << css.str() << "% &mdash; " << pair.label << "</span>"
              << "</div>"
              << "<div class='score-row'>"
              << "<div class='score-item'>Token: <span>";

            std::ostringstream ts, ss2;
            ts.precision(1); ts << std::fixed << pair.tokenScore;
            ss2.precision(1); ss2 << std::fixed << pair.styleScore;
            h << ts.str() << "%</span></div>"
              << "<div class='score-item'>Style: <span>"
              << ss2.str() << "%</span></div>"
              << "<div class='score-item'>Combined: <span>"
              << css.str() << "%</span></div>"
              << "</div><div class='pair-body'>"
              << "<div class='students-grid'>";

            // Student cards
            auto studentCard = [&](const AuthorshipResult& r,
                                    const std::string& filename) {
                std::string col = scoreColor(r.scorePct);
                std::ostringstream sp;
                sp.precision(1); sp << std::fixed << r.scorePct;
                int fillW = (int)r.scorePct;

                h << "<div class='student-card'>"
                  << "<div class='student-name'>" << filename << "</div>"
                  << "<div class='auth-score' style='color:" << col << "'>"
                  << sp.str() << "%</div>"
                  << "<div class='auth-label'>" << r.label << "</div>"
                  << "<div class='progress-bg'><div class='progress-fill' style='width:"
                  << fillW << "%;background:" << col << "'></div></div>";

                // Profile Reliability Indicator
                // Based on profile size - matches Chapter 2 (Brocardo et al. 2014)
                std::string relLabel, relColor, relNote;
                if (r.profileSize >= 3) {
                    relLabel = "High";
                    relColor = "#4caf7d";
                    relNote  = "";
                } else if (r.profileSize == 2) {
                    relLabel = "Moderate";
                    relColor = "#f0a500";
                    relNote  = "Score reliability may be limited by small profile size.";
                } else {
                    relLabel = "Low";
                    relColor = "#e05c5c";
                    relNote  = "Score may be less reliable due to limited profile data. "
                               "Additional submissions recommended for higher confidence.";
                }

                h << "<div style='margin-top:8px;padding:8px 10px;background:#0a0a18;"
                  << "border-left:3px solid " << relColor << ";border-radius:4px;"
                  << "font-size:.75rem'>"
                  << "<div style='color:#9090b0'>"
                  << "Profile Reliability: "
                  << "<span style='color:" << relColor << ";font-weight:600'>"
                  << relLabel << "</span>"
                  << " <span style='color:#6060a0'>(" << r.profileSize
                  << " prior submission" << (r.profileSize == 1 ? "" : "s") << ")</span>"
                  << "</div>";
                if (!relNote.empty()) {
                    h << "<div style='color:#7070a0;font-style:italic;margin-top:4px;"
                      << "font-size:.7rem'>" << relNote << "</div>";
                }
                h << "</div>";

                h << "<div class='style-list'>";

                // Categorize features into Lexical, Layout, Syntactic
                // based on what each feature measures (matches Chapter 2)
                std::vector<std::string> lexical, layout, syntactic;
                std::string summary;

                for (const auto& note : r.styleMatch) {
                    if (note.find("matched /") != std::string::npos) {
                        summary = note;
                        continue;
                    }
                    // Categorize by feature name in the note
                    if (note.find("Variable name length") != std::string::npos ||
                        note.find("Single-letter variables") != std::string::npos ||
                        note.find("camelCase naming") != std::string::npos) {
                        lexical.push_back(note);
                    }
                    else if (note.find("Comment frequency") != std::string::npos ||
                             note.find("Inline comment style") != std::string::npos ||
                             note.find("Bracket placement") != std::string::npos ||
                             note.find("Blank line usage") != std::string::npos) {
                        layout.push_back(note);
                    }
                    else if (note.find("Loop preference") != std::string::npos ||
                             note.find("Operator spacing") != std::string::npos) {
                        syntactic.push_back(note);
                    }
                }

                auto renderGroup = [&](const std::string& title,
                                        const std::vector<std::string>& items) {
                    if (items.empty()) return;
                    h << "<div style='margin-top:8px;font-size:.7rem;color:#7070a0;"
                      << "text-transform:uppercase;letter-spacing:.5px;font-weight:600;"
                      << "padding-bottom:3px;border-bottom:1px solid #2a2a4a'>"
                      << title << "</div>";
                    for (const auto& note : items) {
                        bool isMatch = (note.find("[MATCH]") != std::string::npos);
                        h << "<div class='" << (isMatch ? "match" : "mismatch") << "'>"
                          << (isMatch ? "&#10003;" : "&#10007;")
                          << note.substr(note.find(']') + 1)
                          << "</div>";
                    }
                };

                renderGroup("Lexical Features",   lexical);
                renderGroup("Layout Features",    layout);
                renderGroup("Syntactic Features", syntactic);

                if (!summary.empty()) {
                    h << "<div style='color:#6060a0;margin-top:8px;font-size:.75rem;"
                      << "padding-top:6px;border-top:1px solid #1e1e3e'>"
                      << summary << "</div>";
                }

                h << "</div></div>";
            };

            if (verdict.success) {
                studentCard(verdict.resultA, pair.studentA);
                studentCard(verdict.resultB, pair.studentB);
            }

            h << "</div>";

            // Comparative Authorship Analysis (no verdict)
            if (verdict.success) {
                std::ostringstream spA, spB;
                spA.precision(1); spA << std::fixed << verdict.resultA.scorePct;
                spB.precision(1); spB << std::fixed << verdict.resultB.scorePct;

                double diff = std::abs(verdict.resultA.scorePct -
                                        verdict.resultB.scorePct);
                std::string interpretation;
                if (diff < 5.0) {
                    interpretation =
                        "Both submissions show comparable style consistency with their "
                        "respective profiles. No significant authorship inconsistency "
                        "detected in either submission.";
                } else {
                    const AuthorshipResult& lower =
                        (verdict.resultA.scorePct < verdict.resultB.scorePct)
                        ? verdict.resultA : verdict.resultB;
                    interpretation =
                        "The submission by " + lower.student +
                        " shows substantially lower style consistency with their "
                        "established profile. The instructor is advised to examine "
                        "this submission for potential authorship inconsistency.";
                }

                h << "<div class='verdict-box'>"
                  << "<div class='verdict-title'>&#128202; Comparative Authorship Analysis</div>"
                  << "<div class='verdict-row'>"
                  << "<span style='color:#9090b0'>Submission A &mdash; " << verdict.studentA << ":</span>"
                  << "<span class='vl' style='color:" << scoreColor(verdict.resultA.scorePct)
                  << "'>&nbsp;" << spA.str() << "% (" << verdict.resultA.label << ")</span></div>"
                  << "<div class='verdict-row'>"
                  << "<span style='color:#9090b0'>Submission B &mdash; " << verdict.studentB << ":</span>"
                  << "<span class='vl' style='color:" << scoreColor(verdict.resultB.scorePct)
                  << "'>&nbsp;" << spB.str() << "% (" << verdict.resultB.label << ")</span></div>"
                  << "<div style='margin-top:10px;padding:10px;background:#0a0a18;border-radius:6px;font-size:.82rem;color:#c0c0d0;line-height:1.5'>"
                  << "<strong style='color:#a0a0e0'>Interpretation:</strong> " << interpretation
                  << "</div>"
                  << "<div class='note'>This report provides statistical evidence only. "
                  << "All academic integrity determinations rest with the instructor.</div>"
                  << "</div>";
            }
            h << "</div></div>";
        }
    }

    // ── All Pairs Table ───────────────────────────────────────
    h << "<div class='section-title'>&#128202; All Pair Scores</div>";
    h << "<table style='width:100%;border-collapse:collapse;font-size:.82rem'>"
      << "<tr style='background:#1e1e3e;color:#9090b0'>"
      << "<th style='padding:10px;text-align:left'>Pair</th>"
      << "<th style='padding:10px'>Token</th>"
      << "<th style='padding:10px'>Style</th>"
      << "<th style='padding:10px'>Combined</th>"
      << "<th style='padding:10px'>Result</th></tr>";

    for (const auto& r : allPairs) {
        if (!r.success) continue;
        std::string bg = r.flagged ? "#1a0a0a" : "#12121f";
        std::ostringstream ts, ss2, cs;
        ts.precision(1); ts << std::fixed << r.tokenScore;
        ss2.precision(1); ss2 << std::fixed << r.styleScore;
        cs.precision(1); cs << std::fixed << r.combinedScore;

        h << "<tr style='background:" << bg << ";border-bottom:1px solid #1e1e2e'>"
          << "<td style='padding:9px 10px'>"
          << (r.flagged ? "&#128680; " : "")
          << r.studentA << " &harr; " << r.studentB << "</td>"
          << "<td style='padding:9px;text-align:center'>" << ts.str() << "%</td>"
          << "<td style='padding:9px;text-align:center'>" << ss2.str() << "%</td>"
          << "<td style='padding:9px;text-align:center;font-weight:600;color:"
          << simColor(r.combinedScore) << "'>" << cs.str() << "%</td>"
          << "<td style='padding:9px;text-align:center;color:"
          << simColor(r.combinedScore) << "'>" << r.label << "</td></tr>";
    }
    h << "</table>";

    h << R"(</div>
<footer>
  Code Authorship Likelihood Scoring System &mdash; CS Thesis Prototype<br>
  University of Luzon, College of Computer Studies &mdash; Academic Integrity Support Tool<br>
  <span style="color:#303050">This system provides statistical evidence only. All final judgments rest with the instructor.</span>
</footer>
</body></html>)";

    return h.str();
}

// ═════════════════════════════════════════════════════════════
// MAIN
// ═════════════════════════════════════════════════════════════

// ═════════════════════════════════════════════════════════════
// ANALYSIS PIPELINE — Callable function for both CLI and GUI
// Returns AnalysisResults struct with everything needed for display
// ═════════════════════════════════════════════════════════════

#include "results_data.h"

// Global results storage accessible from GUI
AnalysisResults g_analysisResults = {};

// Helper: categorize a feature name
static std::string categorizeFeature(const std::string& feature) {
    if (feature.find("Variable name length") != std::string::npos ||
        feature.find("Single-letter") != std::string::npos ||
        feature.find("camelCase") != std::string::npos ||
        feature.find("snake_case") != std::string::npos ||
        feature.find("ALL_CAPS") != std::string::npos) {
        return "Lexical";
    }
    if (feature.find("Comment") != std::string::npos ||
        feature.find("Bracket") != std::string::npos ||
        feature.find("Blank line") != std::string::npos ||
        feature.find("Line length") != std::string::npos ||
        feature.find("line length") != std::string::npos ||
        feature.find("Inline comment") != std::string::npos) {
        return "Layout";
    }
    return "Syntactic";
}

// Helper: parse a style note string like
// "[MATCH]    Variable name length (profile:0.21 | submission:0.22)"
static StyleNoteDisplay parseStyleNote(const std::string& noteStr) {
    StyleNoteDisplay note;
    note.isMatch = (noteStr.find("[MATCH]") != std::string::npos);

    // Extract feature name (between ] and ()
    size_t bracket = noteStr.find(']');
    size_t paren = noteStr.find('(');
    if (bracket != std::string::npos && paren != std::string::npos) {
        std::string feat = noteStr.substr(bracket + 1, paren - bracket - 1);
        // Trim whitespace
        while (!feat.empty() && feat.front() == ' ') feat.erase(feat.begin());
        while (!feat.empty() && feat.back() == ' ') feat.pop_back();
        note.feature = feat;
    }

    // Extract profile and submission values
    size_t pcolon = noteStr.find("profile:");
    size_t scolon = noteStr.find("submission:");
    note.profileValue = 0.0;
    note.submissionValue = 0.0;

    if (pcolon != std::string::npos) {
        std::string pstr = noteStr.substr(pcolon + 8);
        try { note.profileValue = std::stod(pstr); } catch(...) {}
    }
    if (scolon != std::string::npos) {
        std::string sstr = noteStr.substr(scolon + 11);
        try { note.submissionValue = std::stod(sstr); } catch(...) {}
    }

    note.category = categorizeFeature(note.feature);
    return note;
}

// Helper: classify reliability
static void getReliability(int profileSize,
                            std::string& label,
                            std::string& note) {
    if (profileSize >= 3) {
        label = "High";
        note  = "";
    } else if (profileSize == 2) {
        label = "Moderate";
        note  = "Score reliability may be limited by small profile size.";
    } else {
        label = "Low";
        note  = "Score may be less reliable due to limited profile data.";
    }
}

// ═════════════════════════════════════════════════════════════
// PER-STUDENT SUMMARY REPORTS
// Generated for students in flagged pairs ≥90% or exact duplicates
// ═════════════════════════════════════════════════════════════

static void generateStudentReports(
    const std::vector<PairResult>& flagged,
    AuthorshipScorer& scorer,
    const std::string& dataFolder)
{
    // Identify students who appear in severe flagged pairs (≥90%)
    // For each, collect all their flagged pair appearances
    struct StudentFlag {
        std::string name;
        std::vector<const PairResult*> pairs;
        std::vector<AuthorshipResult> results;  // their result in each pair
        std::vector<double> pairScores;          // combined score of each pair
        std::vector<std::string> pairLabels;     // label of each pair
        std::vector<std::string> otherStudent;   // who they were paired with
    };

    std::map<std::string, StudentFlag> studentFlags;

    for (const auto& pair : flagged) {
        if (pair.combinedScore < 90.0) continue;  // Only ≥90% or exact duplicates

        // Process student A
        AuthorshipResult rA = scorer.score(pair.studentA, pair.pathA);
        if (rA.success) {
            auto& sf = studentFlags[pair.studentA];
            sf.name = pair.studentA;
            sf.pairs.push_back(&pair);
            sf.results.push_back(rA);
            sf.pairScores.push_back(pair.combinedScore);
            sf.pairLabels.push_back(pair.label);
            sf.otherStudent.push_back(pair.studentB);
        }

        // Process student B
        AuthorshipResult rB = scorer.score(pair.studentB, pair.pathB);
        if (rB.success) {
            auto& sf = studentFlags[pair.studentB];
            sf.name = pair.studentB;
            sf.pairs.push_back(&pair);
            sf.results.push_back(rB);
            sf.pairScores.push_back(pair.combinedScore);
            sf.pairLabels.push_back(pair.label);
            sf.otherStudent.push_back(pair.studentA);
        }
    }

    if (studentFlags.empty()) return;

    // Create reports directory
    std::string reportsDir = dataFolder + "/student_reports";
#ifdef _WIN32
    CreateDirectoryA(reportsDir.c_str(), nullptr);
#endif

    // Generate one HTML report per flagged student
    for (const auto& [studentName, sf] : studentFlags) {
        std::ostringstream h;

        // Find the most severe appearance (lowest authorship score)
        int worstIdx = 0;
        double worstScore = 999.0;
        for (size_t i = 0; i < sf.results.size(); ++i) {
            if (sf.results[i].scorePct < worstScore) {
                worstScore = sf.results[i].scorePct;
                worstIdx = (int)i;
            }
        }

        const AuthorshipResult& primary = sf.results[worstIdx];

        // Count matches/mismatches
        int totalFeatures = 0, matchedFeatures = 0;
        for (const auto& note : primary.styleMatch) {
            if (note.find("matched /") != std::string::npos) continue;
            ++totalFeatures;
            if (note.find("[MATCH]") != std::string::npos &&
                note.find("[MISMATCH]") == std::string::npos) ++matchedFeatures;
        }

        h << R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Student Summary Report - )" << studentName << R"(</title>
<style>
  * { margin:0; padding:0; box-sizing:border-box; }
  body {
    font-family: 'Segoe UI', Calibri, sans-serif;
    background: #1E2026; color: #E8E6E0;
    padding: 40px; line-height: 1.6;
  }
  .header {
    background: #7B1F2A; padding: 30px 40px;
    border-bottom: 4px solid #D4AF37;
    margin: -40px -40px 30px -40px;
  }
  .header h1 { color: #D4AF37; font-size: 28px; margin-bottom: 4px; }
  .header .subtitle { color: #E8E6E0; font-size: 16px; }
  .header .meta { color: #C0A060; font-size: 13px; margin-top: 8px; }

  .section { margin-bottom: 30px; }
  .section-title {
    color: #D4AF37; font-size: 18px; font-weight: bold;
    border-bottom: 2px solid #D4AF37; padding-bottom: 6px;
    margin-bottom: 16px;
  }

  .card {
    background: #2A2D34; border: 1px solid #3A3D44;
    padding: 20px; border-radius: 4px; margin-bottom: 16px;
  }

  .score-box {
    display: inline-block; padding: 8px 20px;
    font-size: 22px; font-weight: bold; border-radius: 4px;
    margin: 8px 0;
  }
  .score-high { background: rgba(76,175,80,0.2); color: #4CAF50; }
  .score-mod  { background: rgba(212,175,55,0.2); color: #D4AF37; }
  .score-low  { background: rgba(255,152,0,0.2); color: #FF9800; }
  .score-vlow { background: rgba(244,67,54,0.2); color: #F44336; }

  .feature-table { width: 100%; border-collapse: collapse; margin-top: 12px; }
  .feature-table th {
    text-align: left; padding: 8px 12px;
    background: #55141C; color: #D4AF37; font-size: 13px;
  }
  .feature-table td { padding: 8px 12px; font-size: 14px; }
  .row-match { background: rgba(76,175,80,0.1); }
  .row-mismatch { background: rgba(244,67,54,0.1); }
  .badge {
    display: inline-block; padding: 2px 8px; border-radius: 3px;
    font-size: 12px; font-weight: bold;
  }
  .badge-match { background: rgba(76,175,80,0.3); color: #4CAF50; }
  .badge-mismatch { background: rgba(244,67,54,0.3); color: #F44336; }

  .pair-row {
    background: #2A2D34; border: 1px solid #3A3D44;
    padding: 14px 20px; margin-bottom: 8px; border-radius: 4px;
    display: flex; justify-content: space-between; align-items: center;
  }
  .pair-score { font-weight: bold; font-size: 18px; }

  .justification {
    background: #37232A; border-left: 4px solid #D4AF37;
    padding: 20px 24px; margin-top: 16px; border-radius: 0 4px 4px 0;
  }
  .justification h3 { color: #D4AF37; margin-bottom: 10px; }

  .disclaimer {
    background: #2A2D34; border: 1px solid #3A3D44;
    padding: 16px 20px; margin-top: 30px; border-radius: 4px;
    color: #9A9AA0; font-size: 13px; text-align: center;
  }

  .footer {
    text-align: center; color: #6A6A70; font-size: 12px;
    margin-top: 30px; padding-top: 16px;
    border-top: 1px solid #3A3D44;
  }
</style>
</head>
<body>

<div class="header">
  <h1>Student Authorship Summary Report</h1>
  <div class="subtitle">)" << studentName << R"(</div>
  <div class="meta">Generated by CALSS — Code Authorship Likelihood Scoring System</div>
</div>

<div class="section">
  <div class="section-title">OVERVIEW</div>
  <div class="card">
    <p>This report summarizes the authorship verification findings for <strong>)"
      << studentName << R"(</strong>'s exam submission, as analyzed by the
    Code Authorship Likelihood Scoring System.</p>
    <p style="margin-top:12px">The submission was compared against
    <strong>)" << studentName << R"(</strong>'s established style profile,
    built from <strong>)" << primary.profileSize << R"(</strong> prior supervised
    submission)" << (primary.profileSize != 1 ? "s" : "") << R"(.</p>
  </div>
</div>

<div class="section">
  <div class="section-title">AUTHORSHIP LIKELIHOOD SCORE</div>
  <div class="card">
    <div class="score-box )"
     << (primary.scorePct >= 80 ? "score-high" :
         primary.scorePct >= 60 ? "score-mod" :
         primary.scorePct >= 40 ? "score-low" : "score-vlow")
     << R"(">)" << std::fixed << std::setprecision(1) << primary.scorePct
     << R"(% — )" << primary.label << R"(</div>
    <p style="margin-top:12px; color:#9A9AA0">Features matched: <strong>)"
     << matchedFeatures << " / " << totalFeatures
     << R"(</strong> stylometric features consistent with established profile.</p>
  </div>
</div>

<div class="section">
  <div class="section-title">FLAGGED PAIR APPEARANCES</div>
)";

        // List all flagged pairs this student appears in
        for (size_t i = 0; i < sf.pairs.size(); ++i) {
            std::string scoreClass =
                sf.pairScores[i] >= 95 ? "#F44336" :
                sf.pairScores[i] >= 90 ? "#FF9800" : "#D4AF37";

            h << "  <div class='pair-row'>\n"
              << "    <div><strong>" << sf.name << "</strong> vs <strong>"
              << sf.otherStudent[i] << "</strong>"
              << "<br><span style='color:#9A9AA0;font-size:13px'>"
              << sf.pairLabels[i] << "</span></div>\n"
              << "    <div class='pair-score' style='color:" << scoreClass
              << "'>" << std::fixed << std::setprecision(1)
              << sf.pairScores[i] << "%</div>\n"
              << "  </div>\n";
        }

        h << R"(</div>

<div class="section">
  <div class="section-title">PER-FEATURE STYLE ANALYSIS</div>
  <p style="margin-bottom:12px; color:#9A9AA0">Each feature below compares this
  submission against the student's established style profile. A <span class="badge badge-match">MATCH</span>
  indicates consistency. A <span class="badge badge-mismatch">MISMATCH</span>
  indicates a deviation from typical patterns.</p>

  <table class="feature-table">
    <tr>
      <th>STATUS</th>
      <th>FEATURE</th>
      <th>PROFILE VALUE</th>
      <th>SUBMISSION VALUE</th>
      <th>CATEGORY</th>
    </tr>
)";

        // Parse and display all feature notes
        std::string currentCategory = "";
        for (const auto& note : primary.styleMatch) {
            if (note.find("matched /") != std::string::npos) continue;

            bool isMatch = (note.find("[MATCH]") != std::string::npos &&
                           note.find("[MISMATCH]") == std::string::npos);

            // Extract feature name
            std::string feature = "Unknown";
            size_t bracket = note.find(']');
            size_t paren = note.find('(');
            if (bracket != std::string::npos && paren != std::string::npos) {
                feature = note.substr(bracket + 1, paren - bracket - 1);
                while (!feature.empty() && feature.front() == ' ')
                    feature.erase(feature.begin());
                while (!feature.empty() && feature.back() == ' ')
                    feature.pop_back();
            }

            // Extract values
            double profVal = 0, subVal = 0;
            size_t pcolon = note.find("profile:");
            size_t scolon = note.find("submission:");
            if (pcolon != std::string::npos) {
                try { profVal = std::stod(note.substr(pcolon + 8)); } catch(...) {}
            }
            if (scolon != std::string::npos) {
                try { subVal = std::stod(note.substr(scolon + 11)); } catch(...) {}
            }

            // Categorize
            std::string cat = "Syntactic";
            if (feature.find("Variable") != std::string::npos ||
                feature.find("Single") != std::string::npos ||
                feature.find("camel") != std::string::npos ||
                feature.find("snake") != std::string::npos ||
                feature.find("ALL_CAPS") != std::string::npos)
                cat = "Lexical";
            else if (feature.find("Comment") != std::string::npos ||
                     feature.find("Inline") != std::string::npos ||
                     feature.find("Bracket") != std::string::npos ||
                     feature.find("Line length") != std::string::npos ||
                     feature.find("line length") != std::string::npos ||
                     feature.find("Blank") != std::string::npos)
                cat = "Layout";

            h << "    <tr class='" << (isMatch ? "row-match" : "row-mismatch") << "'>\n"
              << "      <td><span class='badge "
              << (isMatch ? "badge-match'>✓ MATCH" : "badge-mismatch'>✗ MISMATCH")
              << "</span></td>\n"
              << "      <td>" << feature << "</td>\n"
              << "      <td>" << std::fixed << std::setprecision(2) << profVal << "</td>\n"
              << "      <td>" << std::fixed << std::setprecision(2) << subVal << "</td>\n"
              << "      <td>" << cat << "</td>\n"
              << "    </tr>\n";
        }

        h << R"(  </table>
</div>

<div class="section">
  <div class="section-title">JUSTIFICATION</div>
  <div class="justification">
    <h3>Why This Submission Was Flagged</h3>
)";

        // Build justification text based on the data
        h << "    <p>The exam submission by <strong>" << studentName
          << "</strong> was flagged because it appeared in "
          << sf.pairs.size() << " high-similarity pair"
          << (sf.pairs.size() != 1 ? "s" : "")
          << " (≥90% combined similarity score).</p>\n\n";

        h << "    <p style='margin-top:12px'>The authorship likelihood score of <strong>"
          << std::fixed << std::setprecision(1) << primary.scorePct
          << "%</strong> (" << primary.label
          << ") indicates that <strong>" << matchedFeatures << " out of "
          << totalFeatures << "</strong> measured stylometric features "
          << "are consistent with the student's established coding style profile.</p>\n\n";

        // List specific mismatches
        int mismatchCount = totalFeatures - matchedFeatures;
        if (mismatchCount > 0) {
            h << "    <p style='margin-top:12px'>The following "
              << mismatchCount << " feature"
              << (mismatchCount != 1 ? "s" : "")
              << " deviate from the student's typical coding patterns:</p>\n"
              << "    <ul style='margin:8px 0 8px 24px; color:#E8E6E0'>\n";

            for (const auto& note : primary.styleMatch) {
                if (note.find("[MISMATCH]") == std::string::npos) continue;
                if (note.find("matched /") != std::string::npos) continue;

                std::string feature = "Unknown";
                size_t bracket = note.find(']');
                size_t paren = note.find('(');
                if (bracket != std::string::npos && paren != std::string::npos) {
                    feature = note.substr(bracket + 1, paren - bracket - 1);
                    while (!feature.empty() && feature.front() == ' ')
                        feature.erase(feature.begin());
                    while (!feature.empty() && feature.back() == ' ')
                        feature.pop_back();
                }

                double profVal = 0, subVal = 0;
                size_t pcolon = note.find("profile:");
                size_t scolon = note.find("submission:");
                if (pcolon != std::string::npos) {
                    try { profVal = std::stod(note.substr(pcolon + 8)); } catch(...) {}
                }
                if (scolon != std::string::npos) {
                    try { subVal = std::stod(note.substr(scolon + 11)); } catch(...) {}
                }

                h << "      <li><strong>" << feature << "</strong> — "
                  << "profile value: " << std::fixed << std::setprecision(2)
                  << profVal << ", submission value: " << subVal
                  << " (deviation detected)</li>\n";
            }
            h << "    </ul>\n";

            h << "    <p style='margin-top:12px'>These deviations suggest that the "
              << "submitted code may not fully reflect the student's typical "
              << "coding habits as established through prior supervised submissions.</p>\n";
        } else {
            h << "    <p style='margin-top:12px'>Although the submission is consistent "
              << "with the student's style profile, it was flagged due to high "
              << "similarity with another student's submission. The instructor "
              << "should review both submissions together.</p>\n";
        }

        h << R"(  </div>
</div>

<div class="disclaimer">
  <strong>Important:</strong> This report provides statistical evidence only.
  The authorship likelihood score is not a verdict of academic dishonesty.
  All academic integrity determinations rest with the instructor and must
  follow the institution's formal processes.<br><br>
  The style profile is built from prior supervised submissions and represents
  the student's typical coding patterns. Deviations may have legitimate
  explanations including natural style evolution, collaborative learning,
  or assignment-specific requirements.
</div>

<div class="footer">
  Code Authorship Likelihood Scoring System (CALSS)<br>
  University of Luzon | College of Computer Studies<br>
  This document is confidential and intended for academic use only.
</div>

</body>
</html>)";

        // Write file
        std::string filename = reportsDir + "/report_" + studentName + ".html";
        std::ofstream out(filename);
        if (out.is_open()) {
            out << h.str();
            out.close();
        }
    }
}

// ═════════════════════════════════════════════════════════════
// PER-STUDENT SIMILARITY REPORT
// Generates a printable HTML report documenting a specific
// student's flagged appearances with full evidence.
// Intended for presentation to the student during a formal
// academic integrity discussion.
// ═════════════════════════════════════════════════════════════

// Sanitize a name for use as a filename
static std::string sanitizeFilename(const std::string& name) {
    std::string out;
    for (char c : name) {
        if (isalnum((unsigned char)c)) out += c;
        else if (c == ' ' || c == '_' || c == '-') out += '_';
    }
    if (out.empty()) out = "student";
    return out;
}

// Format double as percent string
static std::string pctStr(double v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f%%", v);
    return std::string(buf);
}

// Format double with 2 decimals
static std::string twoStr(double v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", v);
    return std::string(buf);
}

// Build the per-feature rows for a given submission
static std::string buildFeatureRowsHTML(
    const std::vector<StyleNoteDisplay>& features)
{
    std::ostringstream h;
    for (const auto& f : features) {
        std::string statusClass = f.isMatch ? "match" : "mismatch";
        std::string statusText  = f.isMatch ? "MATCH" : "MISMATCH";
        h << "<tr class=\"" << statusClass << "\">"
          << "<td class=\"status\">" << statusText << "</td>"
          << "<td class=\"feature\">" << f.feature << "</td>"
          << "<td class=\"value\">" << twoStr(f.profileValue) << "</td>"
          << "<td class=\"value\">" << twoStr(f.submissionValue) << "</td>"
          << "</tr>\n";
    }
    return h.str();
}

// Helper: find a student in flaggedPairs and return their AuthorshipDisplay
// (returns nullptr if not in this pair)
static const AuthorshipDisplay* findStudentInPair(
    const PairAnalysisDisplay& pair, const std::string& studentName)
{
    if (pair.studentA.studentName == studentName) return &pair.studentA;
    if (pair.studentB.studentName == studentName) return &pair.studentB;
    return nullptr;
}

// Helper: find the "other" student (the one being compared against)
static const AuthorshipDisplay* findOtherInPair(
    const PairAnalysisDisplay& pair, const std::string& studentName)
{
    if (pair.studentA.studentName == studentName) return &pair.studentB;
    if (pair.studentB.studentName == studentName) return &pair.studentA;
    return nullptr;
}

// Generate the per-student similarity report
// Returns path to the generated file, or empty string on failure
std::string generateStudentSimilarityReport(const std::string& studentName)
{
    if (!g_analysisResults.valid) return "";

    // Find the student's profile summary
    const StudentProfileSummary* profile = nullptr;
    for (const auto& p : g_analysisResults.profiles) {
        if (p.name == studentName) { profile = &p; break; }
    }
    if (!profile) return "";

    // Collect all flagged pairs where this student appears
    std::vector<const PairAnalysisDisplay*> appearances;
    for (const auto& fp : g_analysisResults.flaggedPairs) {
        if (findStudentInPair(fp, studentName) != nullptr) {
            appearances.push_back(&fp);
        }
    }

    // Determine output path
    std::string safeName = sanitizeFilename(studentName);
    std::string outputPath = "similarity_report_" + safeName + ".html";

    std::ofstream out(outputPath);
    if (!out.is_open()) return "";

    // Get current date string
    std::time_t now = std::time(nullptr);
    char dateBuf[64];
    std::strftime(dateBuf, sizeof(dateBuf), "%B %d, %Y", std::localtime(&now));

    // ── HTML HEAD + STYLES ──────────────────────────────────────
    out << R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Code Similarity Report - )" << studentName << R"(</title>
<style>
    * { box-sizing: border-box; }
    body {
        font-family: Georgia, 'Times New Roman', serif;
        max-width: 8.5in;
        margin: 0 auto;
        padding: 0.6in 0.7in;
        background: #ffffff;
        color: #1a1a1a;
        line-height: 1.5;
    }
    .header {
        border-bottom: 3px solid #7B1F2A;
        padding-bottom: 16px;
        margin-bottom: 28px;
    }
    .institution {
        font-size: 11pt;
        color: #7B1F2A;
        font-weight: bold;
        letter-spacing: 2px;
        margin-bottom: 4px;
    }
    .college {
        font-size: 10pt;
        color: #666;
        margin-bottom: 14px;
    }
    .title {
        font-size: 22pt;
        color: #1a1a1a;
        margin: 0 0 6px 0;
        font-weight: bold;
    }
    .subtitle {
        font-size: 11pt;
        color: #555;
        font-style: italic;
    }
    .meta {
        margin-top: 14px;
        font-size: 10pt;
        color: #666;
    }
    .meta strong { color: #1a1a1a; }
    h2 {
        font-size: 14pt;
        color: #7B1F2A;
        border-bottom: 1px solid #d4af37;
        padding-bottom: 4px;
        margin-top: 28px;
        margin-bottom: 12px;
    }
    h3 {
        font-size: 12pt;
        color: #1a1a1a;
        margin-top: 18px;
        margin-bottom: 8px;
    }
    p { font-size: 11pt; margin: 8px 0; }
    .summary-box {
        background: #fdf6e3;
        border-left: 4px solid #d4af37;
        padding: 14px 18px;
        margin: 14px 0;
        font-size: 11pt;
    }
    .profile-card {
        background: #f7f7f7;
        border: 1px solid #ddd;
        padding: 14px 18px;
        margin: 12px 0;
    }
    .profile-card ul {
        margin: 6px 0 0 0;
        padding-left: 20px;
        font-size: 10.5pt;
    }
    .pair-block {
        border: 1px solid #ccc;
        margin: 16px 0;
        page-break-inside: avoid;
    }
    .pair-header {
        background: #7B1F2A;
        color: #ffffff;
        padding: 10px 16px;
        font-weight: bold;
        font-size: 12pt;
    }
    .pair-header .score-right {
        float: right;
        color: #d4af37;
    }
    .pair-body {
        padding: 14px 18px;
    }
    .key-finding {
        background: #fff5f5;
        border-left: 4px solid #c62828;
        padding: 12px 14px;
        margin: 10px 0;
        font-size: 10.5pt;
    }
    .key-finding strong { color: #c62828; }
    table.features {
        width: 100%;
        border-collapse: collapse;
        margin-top: 10px;
        font-size: 10pt;
    }
    table.features th {
        background: #1a1a1a;
        color: #fff;
        text-align: left;
        padding: 8px 10px;
        font-weight: bold;
    }
    table.features td {
        padding: 7px 10px;
        border-bottom: 1px solid #e0e0e0;
    }
    table.features tr.match td.status     { color: #2e7d32; font-weight: bold; }
    table.features tr.mismatch td.status  { color: #c62828; font-weight: bold; }
    table.features tr.match              { background: #f1f8f4; }
    table.features tr.mismatch           { background: #fdf2f2; }
    table.features td.value {
        text-align: right;
        font-family: 'Courier New', monospace;
        color: #555;
    }
    code {
        background: #f4f4f4;
        padding: 1px 5px;
        border-radius: 3px;
        font-family: 'Courier New', monospace;
        font-size: 9pt;
        color: #333;
    }
    .category-label {
        font-size: 10pt;
        font-weight: bold;
        color: #7B1F2A;
        letter-spacing: 1px;
        margin-top: 10px;
        margin-bottom: 4px;
    }
    .closing-statement {
        margin-top: 28px;
        padding: 14px 18px;
        background: #f7f7f7;
        border-left: 4px solid #1a1a1a;
        font-size: 10.5pt;
        font-style: italic;
    }
    .signatures {
        margin-top: 40px;
        page-break-inside: avoid;
    }
    .sig-row {
        display: flex;
        justify-content: space-between;
        margin-top: 30px;
    }
    .sig-block {
        width: 45%;
    }
    .sig-line {
        border-top: 1px solid #1a1a1a;
        padding-top: 4px;
        font-size: 10pt;
        color: #555;
    }
    .footer {
        margin-top: 40px;
        padding-top: 12px;
        border-top: 1px solid #ddd;
        text-align: center;
        font-size: 9pt;
        color: #999;
    }
    .print-button {
        position: fixed;
        top: 20px;
        right: 20px;
        background: #7B1F2A;
        color: #fff;
        border: none;
        padding: 10px 20px;
        font-size: 11pt;
        cursor: pointer;
        border-radius: 4px;
        font-family: Georgia, serif;
    }
    .print-button:hover { background: #5a161f; }
    @media print {
        body { padding: 0.4in; }
        .print-button { display: none; }
        .pair-block { page-break-inside: avoid; }
        h2 { page-break-after: avoid; }
    }
</style>
</head>
<body>
<button class="print-button" onclick='window.print()'>Print Report</button>

<div class="header">
    <div class="institution">UNIVERSITY OF LUZON</div>
    <div class="college">College of Computer Studies</div>
    <h1 class="title">Code Similarity Report</h1>
    <div class="subtitle">Authorship Verification Analysis</div>
    <div class="meta">
        <strong>Student:</strong> )" << studentName << R"(<br>
        <strong>Date Generated:</strong> )" << dateBuf << R"(<br>
        <strong>Instructor:</strong> __________________________<br>
        <strong>Course:</strong> __________________________
    </div>
</div>

<h2>1. Purpose of This Report</h2>
<p>This report documents stylistic inconsistencies identified between )"
       << studentName << R"('s submitted code and their established style profile.
The findings below are produced by the Code Authorship Likelihood Scoring System
(CALSS) using token-based similarity detection and K-Nearest Neighbors stylometric
profiling on 14 measurable features. This document is intended to support a formal
discussion of the findings between the instructor and the student.</p>

<h2>2. Student Style Profile</h2>
<p>The following style fingerprint was built from )"
       << profile->submissionCount
       << R"( prior supervised submission)"
       << (profile->submissionCount == 1 ? "" : "s")
       << R"(. This represents )" << studentName
       << R"('s typical coding patterns:</p>
<div class="profile-card">
    <ul>
)";
    for (const auto& note : profile->styleNotes) {
        out << "        <li>" << note << "</li>\n";
    }
    out << R"(    </ul>
</div>

<h2>3. Understanding the 14 Stylometric Features</h2>
<p>The system analyzes 14 measurable coding habits grouped into three categories. Below is a brief explanation of each feature, along with an example. These features are largely unconscious — most programmers do not actively think about them while coding.</p>

<h3>Lexical Features (How You Name Things)</h3>
<table class="features">
<thead><tr><th>Feature</th><th>What It Measures</th><th>Example</th></tr></thead>
<tbody>
<tr><td><strong>Variable Name Length</strong></td>
<td>How long your variable names are on average.</td>
<td><code>int x = 5;</code> (short) vs <code>int studentAge = 5;</code> (long)</td></tr>
<tr><td><strong>Single-Letter Variables</strong></td>
<td>How often you use one-letter names like x, i, n.</td>
<td><code>int x = a + b;</code> (high) vs <code>int sum = price + tax;</code> (low)</td></tr>
<tr><td><strong>camelCase Naming</strong></td>
<td>How often you name things likeThis.</td>
<td><code>int myVariable = 10;</code></td></tr>
<tr><td><strong>snake_case Naming</strong></td>
<td>How often you name things like_this.</td>
<td><code>int student_count = 5;</code></td></tr>
<tr><td><strong>ALL_CAPS Variables</strong></td>
<td>How often you use FULL UPPERCASE names.</td>
<td><code>int MAX_SIZE = 100;</code></td></tr>
</tbody>
</table>

<h3>Layout Features (How Your Code Looks)</h3>
<table class="features">
<thead><tr><th>Feature</th><th>What It Measures</th><th>Example</th></tr></thead>
<tbody>
<tr><td><strong>Comment Frequency</strong></td>
<td>How much of your code is comments vs actual code.</td>
<td>Some students comment every line; others rarely comment at all.</td></tr>
<tr><td><strong>Inline Comment Style</strong></td>
<td>Comments on the same line as code vs their own line.</td>
<td><code>int x = 5; // set x</code> (inline) vs a comment on its own line above</td></tr>
<tr><td><strong>Bracket Placement</strong></td>
<td>Whether you place the opening brace on the same line or a new line.</td>
<td><code>if (x) {</code> (same-line) vs <code>if (x)\n{</code> (new-line)</td></tr>
<tr><td><strong>Line Length</strong></td>
<td>How long your lines of code tend to be.</td>
<td>Short, compact lines vs long, descriptive lines.</td></tr>
<tr><td><strong>Blank Line Usage</strong></td>
<td>How much whitespace you leave between blocks of code.</td>
<td>Compact code with no gaps vs spacious code with blank lines between sections.</td></tr>
</tbody>
</table>

<h3>Syntactic Features (How You Structure Code)</h3>
<table class="features">
<thead><tr><th>Feature</th><th>What It Measures</th><th>Example</th></tr></thead>
<tbody>
<tr><td><strong>Loop Preference</strong></td>
<td>Whether you prefer for-loops or while-loops.</td>
<td><code>for (int i=0; i&lt;10; i++)</code> vs <code>while (i &lt; 10)</code></td></tr>
<tr><td><strong>Operator Spacing</strong></td>
<td>Whether you put spaces around operators.</td>
<td><code>x = a + b;</code> (spaced) vs <code>x=a+b;</code> (compact)</td></tr>
<tr><td><strong>Function Length</strong></td>
<td>Whether you write long functions or break code into smaller ones.</td>
<td>One 50-line function vs five 10-line functions.</td></tr>
<tr><td><strong>Return Placement</strong></td>
<td>Whether you always return at the end or return early from functions.</td>
<td><code>if (x > 0) return 1; return 0;</code> (early) vs building a result variable and returning once at the end.</td></tr>
</tbody>
</table>

<p><em>These features form your unique &ldquo;style fingerprint.&rdquo; The system compares each new submission against your established fingerprint to assess authorship consistency.</em></p>

<h2>4. Flagged Appearances ()" << appearances.size() << R"()</h2>
)";

    if (appearances.empty()) {
        out << R"(<p><em>No flagged appearances. This student's submissions appear consistent
with their established style profile across all analyzed work.</em></p>
)";
    } else {
        out << R"(<p>The following submission)" << (appearances.size() == 1 ? " was" : "s were")
            << R"( flagged for review. Each entry documents which features deviated
from the student's established style profile.</p>
)";

        int idx = 1;
        for (const auto* pair : appearances) {
            const AuthorshipDisplay* self  = findStudentInPair(*pair, studentName);
            const AuthorshipDisplay* other = findOtherInPair(*pair, studentName);
            if (!self || !other) { ++idx; continue; }

            out << R"(<div class="pair-block">
    <div class="pair-header">
        Finding #)" << idx << R"( — )" << pair->similarityLabel
                << R"(<span class="score-right">)" << pctStr(pair->combinedScore)
                << R"( similarity</span>
    </div>
    <div class="pair-body">
        <p><strong>Submission examined:</strong> )" << self->filename << R"(<br>
        <strong>Compared against (matching submission):</strong> )" << other->filename << R"(<br>
        <strong>Token similarity:</strong> )" << pctStr(pair->tokenScore)
                << R"(  |  <strong>Style similarity:</strong> )" << pctStr(pair->styleScore)
                << R"(  |  <strong>Combined:</strong> )" << pctStr(pair->combinedScore)
                << R"(</p>

        <h3>Authorship Likelihood for )" << studentName << R"(</h3>
        <p><strong>Score:</strong> )" << pctStr(self->scorePct)
                << R"( — )" << self->label << R"(<br>
        <strong>Features matched:</strong> )" << self->matchedCount
                << R"( out of )" << self->totalFeatures
                << R"(<br>
        <strong>Reliability:</strong> )" << self->reliabilityLabel << R"(</p>
)";

            // Key finding box - explain why this is concerning
            if (self->scorePct < other->scorePct - 5.0) {
                out << R"(        <div class="key-finding">
            <strong>Key Finding:</strong> )" << studentName
                    << R"('s submission shows substantially lower style consistency ("
                    << pctStr(self->scorePct) << ") than the compared submission by "
                    << other->studentName << " (" << pctStr(other->scorePct)
                    << R"(). Both submissions are highly similar in content, but only one matches
            its submitter's established style profile. This pattern suggests that )"
                    << studentName << R"('s submission may not reflect their authentic work.
        </div>
)";
            } else if (other->scorePct < self->scorePct - 5.0) {
                out << R"(        <div class="key-finding" style="border-left-color:#2e7d32;background:#f1f8f4;">
            <strong>Key Finding:</strong> )" << studentName
                    << R"('s submission shows higher style consistency ()"
                    << pctStr(self->scorePct) << ") than the matching submission by "
                    << other->studentName << " (" << pctStr(other->scorePct)
                    << R"(). The content is highly similar between both, but )" << studentName
                    << R"('s submission better matches their established style profile.
        </div>
)";
            } else {
                out << R"(        <div class="key-finding" style="border-left-color:#f9a825;background:#fffbf0;">
            <strong>Key Finding:</strong> Both submissions show comparable style consistency
            with their respective profiles. The similarity in content warrants review but
            no single submission shows a clear stylistic deviation.
        </div>
)";
            }

            // Feature breakdown table
            out << R"(        <h3>Per-Feature Analysis</h3>
        <table class="features">
            <thead>
                <tr>
                    <th style="width:90px;">Status</th>
                    <th>Feature</th>
                    <th style="width:90px;text-align:right;">Profile</th>
                    <th style="width:110px;text-align:right;">Submitted</th>
                </tr>
            </thead>
            <tbody>
)";
            out << "                <tr><td colspan=\"4\" class=\"category-label\">LEXICAL</td></tr>\n";
            out << buildFeatureRowsHTML(self->lexicalFeatures);
            out << "                <tr><td colspan=\"4\" class=\"category-label\">LAYOUT</td></tr>\n";
            out << buildFeatureRowsHTML(self->layoutFeatures);
            out << "                <tr><td colspan=\"4\" class=\"category-label\">SYNTACTIC</td></tr>\n";
            out << buildFeatureRowsHTML(self->syntacticFeatures);
            out << R"(            </tbody>
        </table>
    </div>
</div>
)";
            ++idx;
        }
    }

    // Closing statement and signatures
    out << R"(
<h2>5. Important Note</h2>
<div class="closing-statement">
This report provides <strong>statistical evidence only</strong>, generated by an automated
authorship verification system. The scores represent the degree of stylistic consistency
between a submission and the student's established profile — they do not constitute a
verdict of academic dishonesty. All final determinations regarding academic integrity
rest with the instructor based on this evidence and other relevant considerations,
in accordance with the institutional policies of the University of Luzon, College of
Computer Studies.
</div>

<div class="signatures">
    <h2>6. Acknowledgment</h2>
    <p>By signing below, the student acknowledges that this report has been presented and
    discussed, and that they have had the opportunity to respond to its findings.</p>

    <div class="sig-row">
        <div class="sig-block">
            <div class="sig-line">Student Signature  |  Date</div>
        </div>
        <div class="sig-block">
            <div class="sig-line">Instructor Signature  |  Date</div>
        </div>
    </div>
</div>

<div class="footer">
    Generated by CALSS — Code Authorship Likelihood Scoring System<br>
    University of Luzon  |  College of Computer Studies  |  )" << dateBuf << R"(
</div>
</body>
</html>
)";
    out.close();
    return outputPath;
}

// ─────────────────────────────────────────────────────────────
// Progress instrumentation (spec §5.5)
//
// AnalysisPhase / AnalysisProgressCallback are declared once in
// results_data.h (shared with gui.cpp). This is purely a reporting
// hook — it does not change what gets computed, in what order, or
// with what weights. The GUI sets a callback before calling
// runAnalysisPipeline(); CLI mode never sets one, so nullptr checks
// make this fully optional and backward compatible.
// ─────────────────────────────────────────────────────────────
static AnalysisProgressCallback g_analysisProgressCb = nullptr;

void setAnalysisProgressCallback(AnalysisProgressCallback cb) {
    g_analysisProgressCb = cb;
}

static bool reportProgress(AnalysisPhase phase, int current, int total) {
    if (g_analysisProgressCb) return g_analysisProgressCb(phase, current, total);
    return true; // no callback registered (CLI mode) — never cancel
}

std::string runAnalysisPipeline(const std::string& dataFolder,
                                  const std::string& outputFile,
                                  bool useAI)
{
    // Reset global results
    g_analysisResults = AnalysisResults{};
    g_analysisResults.valid = false;

    auto allFiles = loadFolder(dataFolder);
    if (allFiles.empty()) {
        return "ERROR: No .c files found in: " + dataFolder;
    }
    reportProgress(AnalysisPhase::LoadingFiles, (int)allFiles.size(), (int)allFiles.size());

    // ── Language validation — run BEFORE any categorization, profile-
    // building, or comparison touches these files. A .c file whose
    // content is actually Java/Python/C++ would otherwise silently
    // pollute results with meaningless feature values (see checkLanguage()
    // above for the full rationale). Flagged files are pulled out of
    // allFiles entirely so every downstream step — profile building,
    // pairwise comparison, categorization — simply never sees them.
    {
        std::vector<std::string> toRemove;
        for (const auto& kv : allFiles) {
            LanguageCheck check = checkLanguage(kv.second);
            if (!check.isPlausibleC) {
                g_analysisResults.languageMismatchFiles.push_back(
                    kv.first + " (suspected: " + check.suspectedLanguage + ")");
                toRemove.push_back(kv.first);
            }
        }
        for (const auto& key : toRemove) allFiles.erase(key);
    }

    // Build profiles
    AuthorshipScorer scorer(3);
    std::map<std::string,bool> hasProfile;

    reportProgress(AnalysisPhase::BuildingProfiles, 0, (int)allFiles.size());
    int profileFilesDone = 0;

    // Priority 1 & 2 combined: build profiles only from files that
    // clearly categorize as Activity/Quiz (Classroom title signal or
    // filename pattern — see categorizeFile() for the full hierarchy).
    for (const auto& kv : allFiles) {
        if (isProfileBuildingFile(kv.first)) {
            std::string student = extractStudentName(kv.first);
            if (scorer.loadSubmission(student, kv.second))
                hasProfile[student] = true;
        }
        reportProgress(AnalysisPhase::BuildingProfiles,
                       ++profileFilesDone, (int)allFiles.size());
    }

    // Cold-start fallback: a student with zero Activity/Quiz submissions
    // (e.g. only an exam on file) still gets a profile built from
    // whatever they have, rather than being excluded entirely. This
    // preserves the existing cold-start handling documented in Chapter 1.
    for (const auto& kv : allFiles) {
        std::string student = extractStudentName(kv.first);
        if (!hasProfile.count(student))
            if (scorer.loadSubmission(student, kv.second))
                hasProfile[student] = true;
    }

    // Priority 3: track files that matched no category at all, so they
    // are visible to the instructor rather than silently discarded.
    for (const auto& kv : allFiles) {
        if (categorizeFile(kv.first) == FileCategory::Unclassified) {
            AnalysisResults::UnclassifiedFile uf;
            uf.key = kv.first;
            uf.fullPath = kv.second;
            g_analysisResults.unclassifiedFiles.push_back(uf);
        }
    }

    // Capture profiles for display
    for (const auto& name : scorer.readyStudents()) {
        const StudentProfile* p = scorer.getProfile(name);
        if (!p) continue;
        StudentProfileSummary summary;
        summary.name = name;
        summary.submissionCount = (int)p->submissions.size();
        summary.featureVector = p->profileVector;  // for Style DNA strip
        for (const auto& sn : p->styleNotes) {
            std::string note = "[" + sn.feature + "] " + sn.observation;
            summary.styleNotes.push_back(note);
        }
        g_analysisResults.profiles.push_back(summary);
    }

    // Run pairwise analysis (only exam files)
    std::map<std::string,std::string> examOnly;
    for (const auto& kv : allFiles)
        if (isExamFile(kv.first))
            examOnly[kv.first] = kv.second;

    auto& compareFiles = examOnly.empty() ? allFiles : examOnly;

    // Files are grouped by extractPeriod() BEFORE comparison so that,
    // for example, a Midterm Exam is only ever compared against other
    // Midterm Exams — never against a Semifinal or Final Exam, which
    // cover entirely different material. Comparing across periods would
    // produce statistically meaningless similarity noise.
    std::vector<PairResult> allPairs;
    {
        std::map<std::string, std::vector<std::string>> byPeriod;
        for (const auto& kv : compareFiles) {
            byPeriod[extractPeriod(kv.first)].push_back(kv.first);
        }

        // Precompute the total comparison count so the progress bar
        // can show a real "X of Y" rather than an indeterminate spinner.
        int totalPairsEstimate = 0;
        for (auto& periodGroup : byPeriod) {
            int n = (int)periodGroup.second.size();
            totalPairsEstimate += (n * (n - 1)) / 2;
        }
        reportProgress(AnalysisPhase::ComparingPairs, 0, totalPairsEstimate);
        int pairsDone = 0;
        bool cancelled = false;

        for (auto& periodGroup : byPeriod) {
            if (cancelled) break;
            auto& names = periodGroup.second;
            for (size_t i = 0; i < names.size() && !cancelled; ++i) {
                for (size_t j = i + 1; j < names.size(); ++j) {
                    const std::string& nA = names[i];
                    const std::string& nB = names[j];
                    ++pairsDone;

                    if (extractStudentName(nA) != extractStudentName(nB)) {
                        PairResult r = analyzePair(
                            nA, compareFiles.at(nA),
                            nB, compareFiles.at(nB));
                        allPairs.push_back(r);
                    }

                    if (!reportProgress(AnalysisPhase::ComparingPairs,
                                        pairsDone, totalPairsEstimate)) {
                        cancelled = true;
                        break;
                    }
                }
            }
        }

        if (cancelled) {
            return "CANCELLED: Analysis cancelled by user.";
        }

        std::sort(allPairs.begin(), allPairs.end(),
            [](const PairResult& a, const PairResult& b){
                if (a.flagged != b.flagged) return a.flagged > b.flagged;
                return a.combinedScore > b.combinedScore;
            });
    }

    auto stats   = summaryStats(allPairs);
    auto flagged = flaggedPairs(allPairs);

    // Populate basic stats
    g_analysisResults.totalPairs      = stats.totalPairs;
    g_analysisResults.flaggedCount    = stats.flaggedCount;
    g_analysisResults.exactDuplicates = stats.exactCount;
    g_analysisResults.highSimilarity  = stats.highCount;
    g_analysisResults.averageScore    = stats.avgScore;
    g_analysisResults.maxScore        = stats.maxScore;
    g_analysisResults.dataFolder      = dataFolder;
    g_analysisResults.reportPath      = outputFile;

    // Build flagged pair displays
    for (const auto& pair : flagged) {
        PairAnalysisDisplay disp;
        disp.filenameA      = pair.studentA;
        disp.filenameB      = pair.studentB;
        disp.tokenScore     = pair.tokenScore;
        disp.styleScore     = pair.styleScore;
        disp.combinedScore  = pair.combinedScore;
        disp.similarityLabel = pair.label;

        std::string sA = extractStudentName(pair.studentA);
        std::string sB = extractStudentName(pair.studentB);
        PairVerdict verdict = scorer.scorePair(sA, pair.pathA,
                                                 sB, pair.pathB);

        auto fillStudent = [&](AuthorshipDisplay& d,
                                const AuthorshipResult& r,
                                const std::string& filename,
                                const std::string& student) {
            d.filename     = filename;
            d.studentName  = student;
            d.scorePct     = r.scorePct;
            d.label        = r.label;
            d.profileSize  = r.profileSize;
            getReliability(r.profileSize, d.reliabilityLabel,
                            d.reliabilityNote);

            int total = 0, matched = 0;
            for (const auto& noteStr : r.styleMatch) {
                if (noteStr.find("matched /") != std::string::npos) continue;
                StyleNoteDisplay note = parseStyleNote(noteStr);
                if (note.isMatch) ++matched;
                ++total;
                if (note.category == "Lexical")
                    d.lexicalFeatures.push_back(note);
                else if (note.category == "Layout")
                    d.layoutFeatures.push_back(note);
                else
                    d.syntacticFeatures.push_back(note);
            }
            d.matchedCount = matched;
            d.totalFeatures = total;
        };

        if (verdict.success) {
            fillStudent(disp.studentA, verdict.resultA, pair.studentA, sA);
            fillStudent(disp.studentB, verdict.resultB, pair.studentB, sB);

            double diff = std::abs(verdict.resultA.scorePct -
                                    verdict.resultB.scorePct);
            if (diff < 5.0) {
                disp.interpretation =
                    "Both submissions show comparable style consistency "
                    "with their respective profiles. No significant authorship "
                    "inconsistency detected in either submission.";
            } else {
                const AuthorshipDisplay& lower =
                    (verdict.resultA.scorePct < verdict.resultB.scorePct)
                    ? disp.studentA : disp.studentB;
                disp.interpretation =
                    "The submission by " + lower.studentName +
                    " shows substantially lower style consistency with their "
                    "established profile. The instructor is advised to "
                    "examine this submission for potential authorship "
                    "inconsistency.";
            }
        }

        g_analysisResults.flaggedPairs.push_back(disp);
    }

    // Build all pairs table
    for (const auto& r : allPairs) {
        if (!r.success) continue;
        PairRow row;
        row.filenameA     = r.studentA;
        row.filenameB     = r.studentB;
        row.tokenScore    = r.tokenScore;
        row.styleScore    = r.styleScore;
        row.combinedScore = r.combinedScore;
        row.label         = r.label;
        row.flagged       = r.flagged;
        g_analysisResults.allPairs.push_back(row);
    }

    // AI Summary
    AISummary aiSummary;
    aiSummary.success = false;
    aiSummary.skipped = false;

    if (useAI && !flagged.empty()) {
        reportProgress(AnalysisPhase::GeneratingSummary, 0, 1);
        aiSummary = generateAISummary(flagged, scorer);
        reportProgress(AnalysisPhase::GeneratingSummary, 1, 1);
    } else {
        aiSummary.skipped = true;
        aiSummary.error = "AI summary not requested.";
    }

    g_analysisResults.aiSummarySuccess = aiSummary.success;
    g_analysisResults.aiSummarySkipped = aiSummary.skipped;
    g_analysisResults.aiSummaryText    = aiSummary.text;
    g_analysisResults.aiSummaryError   = aiSummary.error;

    // Tiered AI outputs (restructure) — additive alongside the legacy
    // aiSummary block above, which is left untouched and still feeds
    // htmlReport() below exactly as it did before this change (approved:
    // run both, revisit the extra API cost later if it becomes a problem).
    // Gated by the same pre-run useAI toggle; no separate mid-flow prompt.
    if (useAI && !flagged.empty()) {
        g_analysisResults.aiPairNarratives =
            generatePairNarratives(g_analysisResults.flaggedPairs);
        g_analysisResults.aiBatchPatterns =
            generateCrossPairPatterns(g_analysisResults.flaggedPairs);
    } else {
        g_analysisResults.aiBatchPatterns.skipped = true;
        g_analysisResults.aiBatchPatterns.success = false;
        g_analysisResults.aiBatchPatterns.error   = "AI summary not requested.";
        // aiPairNarratives left as an empty vector -- gui.cpp should treat
        // "empty vector" the same as "not requested," same as it would for
        // zero flagged pairs.
    }

    // Timestamp
    time_t now = time(nullptr);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%B %d, %Y  %H:%M", localtime(&now));
    g_analysisResults.timestamp = timebuf;

    // Generate HTML
    std::string html = htmlReport(allPairs, stats, flagged,
                                   scorer, dataFolder, aiSummary);

    std::ofstream out(outputFile);
    if (!out.is_open()) {
        return "ERROR: Cannot write to " + outputFile;
    }
    out << html;
    out.close();

    // Generate per-student summary reports for severe flagged pairs (≥90%)
    generateStudentReports(flagged, scorer, dataFolder);

    g_analysisResults.valid = true;

    std::ostringstream resultMsg;
    resultMsg << "Analysis complete. " << stats.totalPairs
              << " pairs analyzed, " << stats.flaggedCount << " flagged.";
    return resultMsg.str();
}

int main(int argc, char* argv[]) {
    // If no arguments given, launch the GUI
    // If "data" folder path given as first argument, run CLI mode (legacy)
#ifdef _WIN32
    if (argc < 2) {
        // GUI mode
        return runGUI(GetModuleHandle(nullptr));
    }
#endif

    // CLI mode (kept for testing and scripting)
    std::string dataFolder  = "data";
    std::string outputFile  = "report.html";
    if (argc >= 2) dataFolder = argv[1];
    if (argc >= 3) outputFile = argv[2];

    std::cout << "Code Authorship Likelihood Scoring System\n";
    std::cout << "==========================================\n";
    std::cout << "Loading from: " << dataFolder << "\n";

    auto allFiles = loadFolder(dataFolder);
    if (allFiles.empty()) {
        std::cerr << "ERROR: No .c files found in: " << dataFolder << "\n";
        std::cerr << "Usage: authorship.exe [data_folder] [output.html]\n";
        return 1;
    }
    std::cout << "Found " << allFiles.size() << " file(s)\n";

    // Language validation — same check as the GUI pipeline. See
    // checkLanguage() definition above for full rationale.
    {
        std::vector<std::string> toRemove;
        for (const auto& kv : allFiles) {
            LanguageCheck check = checkLanguage(kv.second);
            if (!check.isPlausibleC) {
                std::cout << "WARNING: " << kv.first
                          << " does not look like C (suspected: "
                          << check.suspectedLanguage
                          << ") - excluded from analysis\n";
                toRemove.push_back(kv.first);
            }
        }
        for (const auto& key : toRemove) allFiles.erase(key);
    }

    // Build profiles
    AuthorshipScorer scorer(3);
    std::map<std::string,bool> hasProfile;

    for (const auto& kv : allFiles) {
        if (!isExamFile(kv.first)) {
            std::string student = extractStudentName(kv.first);
            if (scorer.loadSubmission(student, kv.second))
                hasProfile[student] = true;
        }
    }
    for (const auto& kv : allFiles) {
        std::string student = extractStudentName(kv.first);
        if (!hasProfile.count(student))
            if (scorer.loadSubmission(student, kv.second))
                hasProfile[student] = true;
    }

    std::cout << "Profiles built for: ";
    for (const auto& n : scorer.readyStudents()) std::cout << n << " ";
    std::cout << "\n";

    // Run analysis
    // Only compare exam files against each other
    // Activity files are profile sources only — never compared
    std::cout << "Running similarity analysis...\n";

    std::map<std::string,std::string> examOnly;
    for (const auto& kv : allFiles)
        if (isExamFile(kv.first))
            examOnly[kv.first] = kv.second;

    // If no exam files found, fall back to all files
    auto& compareFiles = examOnly.empty() ? allFiles : examOnly;

    // Run pairwise comparison — grouped by grading period, skipping
    // same-student pairs.
    //
    // Files are grouped by extractPeriod() BEFORE comparison so that,
    // for example, a Midterm Exam is only ever compared against other
    // Midterm Exams — never against a Semifinal or Final Exam, which
    // cover entirely different material. Comparing across periods would
    // produce statistically meaningless similarity noise (inflating
    // "Total Pairs Analyzed" and dragging down "Average Score" with
    // comparisons that were never meant to be equivalent).
    //
    // Files with no period signal (extractPeriod() returns "general" —
    // manually imported files, or datasets that already achieve period
    // separation via folder organization rather than filename encoding)
    // all fall into one group together, preserving prior behavior for
    // such datasets exactly as before this change.
    std::vector<PairResult> allPairs;
    {
        std::map<std::string, std::vector<std::string>> byPeriod;
        for (const auto& kv : compareFiles) {
            byPeriod[extractPeriod(kv.first)].push_back(kv.first);
        }

        // Note: no progress/cancellation here — this is the legacy CLI
        // entry point (kept for scripting/testing), which never
        // registers a callback, so reportProgress() would be a
        // guaranteed-true no-op anyway. Cancellation only makes sense
        // where main() can actually return the resulting std::string,
        // which is runAnalysisPipeline() above, not this int-returning
        // CLI path.
        for (auto& periodGroup : byPeriod) {
            auto& names = periodGroup.second;
            for (size_t i = 0; i < names.size(); ++i) {
                for (size_t j = i + 1; j < names.size(); ++j) {
                    const std::string& nA = names[i];
                    const std::string& nB = names[j];
                    // Skip if both files belong to the same student
                    if (extractStudentName(nA) == extractStudentName(nB))
                        continue;
                    PairResult r = analyzePair(
                        nA, compareFiles.at(nA),
                        nB, compareFiles.at(nB));
                    allPairs.push_back(r);
                }
            }
        }

        // Sort: flagged first, then by score descending
        std::sort(allPairs.begin(), allPairs.end(),
            [](const PairResult& a, const PairResult& b){
                if (a.flagged != b.flagged) return a.flagged > b.flagged;
                return a.combinedScore > b.combinedScore;
            });
    }

    auto stats   = summaryStats(allPairs);
    auto flagged = flaggedPairs(allPairs);

    std::cout << "Total pairs : " << stats.totalPairs   << "\n";
    std::cout << "Flagged     : " << stats.flaggedCount  << "\n";
    std::cout << "Max score   : " << stats.maxScore      << "%\n";

    // Ask user if they want AI summary (opt-in)
    AISummary aiSummary;
    aiSummary.success = false;
    aiSummary.skipped = false;

    // Only prompt if there are flagged pairs to summarize
    if (flagged.empty()) {
        aiSummary.skipped = true;
        aiSummary.error = "No flagged pairs to summarize.";
        std::cout << "\nNo flagged pairs found. Skipping AI summary.\n";
    } else {
        // Check if API key exists before asking
        std::string keyCheck = loadApiKey();
        if (keyCheck.empty()) {
            aiSummary.skipped = true;
            aiSummary.error = "No API key configured. AI summary unavailable.";
            std::cout << "\nNo API key configured. Skipping AI summary.\n";
            std::cout << "To enable: create api_key.txt with your Gemini API key.\n";
        } else {
            std::cout << "\n============================================================\n";
            std::cout << "Generate AI-powered instructor summary? (y/n): ";
            std::string choice;
            std::getline(std::cin, choice);

            // Trim whitespace and lowercase the first char
            while (!choice.empty() && (choice.front() == ' ' || choice.front() == '\t'))
                choice.erase(choice.begin());

            char first = choice.empty() ? 'n' : (char)tolower((unsigned char)choice[0]);

            if (first == 'y') {
                std::cout << "  Calling Gemini API...\n";
                aiSummary = generateAISummary(flagged, scorer);
                if (aiSummary.success) {
                    std::cout << "  AI summary generated successfully.\n";
                } else if (aiSummary.skipped) {
                    std::cout << "  AI summary skipped: " << aiSummary.error << "\n";
                } else {
                    std::cout << "  AI summary failed: " << aiSummary.error << "\n";
                }
            } else {
                aiSummary.skipped = true;
                aiSummary.error = "AI summary not requested for this run.";
                std::cout << "  AI summary skipped by user.\n";
            }
        }
    }

    // Generate HTML
    std::string html = htmlReport(allPairs, stats, flagged,
                                   scorer, dataFolder, aiSummary);

    std::ofstream out(outputFile);
    if (!out.is_open()) {
        std::cerr << "ERROR: Cannot write to " << outputFile << "\n";
        return 1;
    }
    out << html;
    out.close();

    // Generate per-student summary reports
    generateStudentReports(flagged, scorer, dataFolder);

    std::cout << "\nReport saved to: " << outputFile << "\n";
    std::cout << "Open it in your browser to view results.\n";

#ifdef _WIN32
    // Auto-open in browser on Windows
    ShellExecuteA(nullptr, "open", outputFile.c_str(),
                  nullptr, nullptr, SW_SHOWNORMAL);
#endif

    return 0;
}