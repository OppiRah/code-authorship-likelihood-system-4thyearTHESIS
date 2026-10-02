// ─────────────────────────────────────────────────────────────
// main.cpp - Code Authorship Likelihood Scoring System
// Analysis pipeline + entry point (launches the GUI)
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
#include "app_paths.h"

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

// Non-static export of extractStudentName for the GUI (Classes cache,
// student detail, CSV export), so it shows files under exactly the
// same student identity the pipeline used to build profiles and
// pairs. Display-only use; does not participate in scoring.
std::string studentNameForFile(const std::string& filename) {
    return extractStudentName(filename);
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

// Non-static export of extractPeriod for the GUI's period labels
// (Flagged Pairs rows, student detail). Display-only use.
std::string periodForFile(const std::string& filename) {
    return extractPeriod(filename);
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
// ANALYSIS PIPELINE — called by the GUI's analysis thread
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
    // Documents\CALSS Reports, where the professor can find it again
    // (not the working folder, which file dialogs can change).
    std::string outputPath = appReportsDirA() + "\\similarity_report_" +
                             safeName + ".html";

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
// runAnalysisPipeline(); nullptr checks keep it optional.
// ─────────────────────────────────────────────────────────────
static AnalysisProgressCallback g_analysisProgressCb = nullptr;

void setAnalysisProgressCallback(AnalysisProgressCallback cb) {
    g_analysisProgressCb = cb;
}

static bool reportProgress(AnalysisPhase phase, int current, int total) {
    if (g_analysisProgressCb) return g_analysisProgressCb(phase, current, total);
    return true; // no callback registered — never cancel
}

std::string runAnalysisPipeline(const std::string& dataFolder,
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
        for (const auto& sub : p->submissions) {    // for View source style-change highlight
            if (!sub.success || sub.raw.size() < FEATURE_COUNT) continue;
            summary.submissionPaths.push_back(sub.filepath);
            summary.submissionRaw.push_back(sub.raw);
        }
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

    // Build flagged pair displays
    for (const auto& pair : flagged) {
        PairAnalysisDisplay disp;
        disp.filenameA      = pair.studentA;
        disp.filenameB      = pair.studentB;
        disp.pathA          = pair.pathA;
        disp.pathB          = pair.pathB;
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

    // AI outputs: per-pair narratives (Tier 1, Flagged Pairs tab) and
    // cross-pair patterns (Tier 2, Overview). Gated by the pre-run
    // useAI toggle; no separate mid-flow prompt.
    if (useAI && !flagged.empty()) {
        reportProgress(AnalysisPhase::GeneratingSummary, 0, 1);
        g_analysisResults.aiPairNarratives =
            generatePairNarratives(g_analysisResults.flaggedPairs);
        g_analysisResults.aiBatchPatterns =
            generateCrossPairPatterns(g_analysisResults.flaggedPairs);
        reportProgress(AnalysisPhase::GeneratingSummary, 1, 1);
    } else {
        g_analysisResults.aiBatchPatterns.skipped = true;
        g_analysisResults.aiBatchPatterns.success = false;
        g_analysisResults.aiBatchPatterns.error   = "AI summary not requested.";
        // aiPairNarratives left as an empty vector -- the GUI treats
        // "empty vector" the same as "not requested," same as it would
        // for zero flagged pairs.
    }

    // Timestamp
    time_t now = time(nullptr);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%B %d, %Y  %H:%M", localtime(&now));
    g_analysisResults.timestamp = timebuf;

    g_analysisResults.valid = true;

    std::ostringstream resultMsg;
    resultMsg << "Analysis complete. " << stats.totalPairs
              << " pairs analyzed, " << stats.flaggedCount << " flagged.";
    return resultMsg.str();
}

int main() {
    return runGUI(GetModuleHandle(nullptr));
}
