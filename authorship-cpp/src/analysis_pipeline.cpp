// ─────────────────────────────────────────────────────────────
// analysis_pipeline.cpp - Code Authorship Likelihood Scoring System
// Loading a data folder (file identity, category and period parsing,
// language check) and runAnalysisPipeline(), which builds profiles,
// compares pairs and fills g_analysisResults for the GUI.
//
// Moved unchanged out of main.cpp (2026-10-02) so the golden-output
// harness can link the pipeline without the app's main().
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

    std::ifstream f(pathFromUtf8(filepath));
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
// google_classroom.h, kept self-contained since analysis_pipeline.cpp doesn't (and
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
        std::ifstream check(pathFromUtf8(manifestPath));
        if (check.is_open()) {
            std::string line;
            std::string searchKey = "\"file\":\"" + bareFilename + "\"";
            while (std::getline(check, line)) {
                if (line.find(searchKey) != std::string::npos) return;
            }
        }
    }

    std::ofstream f(pathFromUtf8(manifestPath), std::ios::app);
    if (!f.is_open()) return;
    f << "{\"file\":\"" << bareFilename << "\","
      << "\"title\":\"Manually Imported\","
      << "\"block\":\"" << blockName << "\"}\n";
}

// Keys (file names) and paths are UTF-8; the folder is enumerated
// with the wide API so non-ASCII names (e.g. "Peña") load (see
// app_paths.h).
static std::map<std::string,std::string> loadFolder(
    const std::string& folder)
{
    std::map<std::string,std::string> files;
#ifdef _WIN32
    // Level 1: files directly in the root data folder. This is the
    // existing behavior — Classroom-synced files land here, tagged
    // with their block via the manifest (not via physical folders).
    {
        WIN32_FIND_DATAW fd;
        std::wstring pattern = widenUtf8(folder + "\\*.c");
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string fname = utf8FromWide(fd.cFileName);
                std::string key   = fname.substr(0, fname.rfind('.'));
                files[key] = folder + "\\" + fname;
            } while (FindNextFileW(hFind, &fd));
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
        WIN32_FIND_DATAW fd;
        std::wstring dirPattern = widenUtf8(folder + "\\*");
        HANDLE hFind = FindFirstFileW(dirPattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                std::string subName = utf8FromWide(fd.cFileName);
                if (subName == "." || subName == "..") continue;

                std::string subFolder = folder + "\\" + subName;
                std::string subPattern = subFolder + "\\*.c";

                WIN32_FIND_DATAW fd2;
                HANDLE hFind2 = FindFirstFileW(widenUtf8(subPattern).c_str(), &fd2);
                if (hFind2 != INVALID_HANDLE_VALUE) {
                    do {
                        std::string fname = utf8FromWide(fd2.cFileName);
                        std::string key   = fname.substr(0, fname.rfind('.'));
                        files[key] = subFolder + "\\" + fname;
                        autoTagManualBlock(folder, fname, subName);
                    } while (FindNextFileW(hFind2, &fd2));
                    FindClose(hFind2);
                }
            } while (FindNextFileW(hFind, &fd));
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
    // Each compared file is tokenized and feature-extracted once per run
    // (on first use) and reused for every pair it is in and for the
    // flagged-pair authorship scoring below. Both are pure functions of
    // the file, so results are identical to re-reading it per pair.
    struct PreparedFile { TokenResult tok; FeatureResult feat; };
    std::map<std::string, PreparedFile> prepared;
    auto prepare = [&](const std::string& key) -> const PreparedFile& {
        auto it = prepared.find(key);
        if (it == prepared.end()) {
            const std::string& path = compareFiles.at(key);
            it = prepared.emplace(key, PreparedFile{tokenize(path),
                                                    extractFeatures(path)}).first;
        }
        return it->second;
    };

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
                        const PreparedFile& pA = prepare(nA);
                        const PreparedFile& pB = prepare(nB);
                        PairResult r = analyzePairPrepared(
                            nA, compareFiles.at(nA), pA.tok, pA.feat,
                            nB, compareFiles.at(nB), pB.tok, pB.feat);
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
        PairVerdict verdict = scorer.scorePair(sA, pair.pathA, prepare(pair.studentA).feat,
                                               sB, pair.pathB, prepare(pair.studentB).feat);

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
        // Only the highest-scoring GEMINI_MAX_PAIRS go to Gemini.
        // flaggedPairs is already ordered by combined score, highest
        // first, so that is its first N entries — and their indices (and
        // the "Pair N" numbers the prompts use) stay the same.
        const auto& all = g_analysisResults.flaggedPairs;
        size_t sent = std::min(all.size(), GEMINI_MAX_PAIRS);
        std::vector<PairAnalysisDisplay> aiPairs(all.begin(), all.begin() + sent);
        g_analysisResults.aiPairsSent = (int)sent;

        g_analysisResults.aiPairNarratives = generatePairNarratives(aiPairs);
        // Keep the vector parallel to flaggedPairs: pairs past the cap
        // get a skipped entry whose reason the pair card shows.
        for (size_t i = sent; i < all.size(); ++i) {
            AIPairNarrative n;
            n.pairId  = all[i].studentA.studentName + " vs " +
                        all[i].studentB.studentName;
            n.success = false;
            n.skipped = true;
            n.error   = "No AI note for this pair: AI notes cover the " +
                        std::to_string(sent) + " highest-scoring flagged pairs.";
            g_analysisResults.aiPairNarratives.push_back(n);
        }
        g_analysisResults.aiBatchPatterns = generateCrossPairPatterns(aiPairs);
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
