// ─────────────────────────────────────────────────────────────
// golden_harness.cpp — regression harness for the analysis pipeline
//
// Runs runAnalysisPipeline() (AI off) on one data folder and writes
// everything the pipeline produces that the GUI or the thesis relies on
// to CSV files: every pair score, every flagged-pair authorship result,
// every profile vector, and the run summary. Doubles are printed with
// %.17g so any numeric drift — however small — shows up in the diff.
//
// Paths are reduced to file names so the output doesn't depend on
// where the dataset was copied. Build and run via tests/run_golden.bat;
// never part of the app build.
//
// Usage: golden_harness.exe <data folder> <output folder>
// ─────────────────────────────────────────────────────────────

#include <cstdio>
#include <string>
#include <vector>
#include <windows.h>

#include "audit_log.h"
#include "results_data.h"

std::string runAnalysisPipeline(const std::string& dataFolder, bool useAI);
extern AnalysisResults g_analysisResults;

static std::string baseName(const std::string& p) {
    size_t s = p.find_last_of("\\/");
    return s == std::string::npos ? p : p.substr(s + 1);
}

// Quote a CSV field only when needed (commas, quotes, newlines).
static std::string csv(const std::string& s) {
    if (s.find_first_of(",\"\n\r") == std::string::npos) return s;
    std::string out = "\"";
    for (char c : s) { if (c == '"') out += '"'; out += c; }
    return out + "\"";
}

static FILE* openOut(const std::string& dir, const char* name) {
    std::string path = dir + "\\" + name;
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) fprintf(stderr, "cannot write %s\n", path.c_str());
    return f;
}

static void writeFeatures(FILE* f, const std::vector<StyleNoteDisplay>& v) {
    for (const auto& n : v)
        fprintf(f, ",%s|%s|%.17g|%.17g|%d", csv(n.feature).c_str(),
                csv(n.observation).c_str(), n.profileValue,
                n.submissionValue, n.isMatch ? 1 : 0);
}

static void writeSide(FILE* f, const AuthorshipDisplay& d) {
    fprintf(f, ",%s,%s,%.17g,%s,%d,%s,%d,%d", csv(d.filename).c_str(),
            csv(d.studentName).c_str(), d.scorePct, csv(d.label).c_str(),
            d.profileSize, csv(d.reliabilityLabel).c_str(), d.matchedCount,
            d.totalFeatures);
    writeFeatures(f, d.lexicalFeatures);
    writeFeatures(f, d.layoutFeatures);
    writeFeatures(f, d.syntacticFeatures);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: golden_harness <data folder> <output folder>\n");
        return 2;
    }
    std::string folder = argv[1], out = argv[2];
    CreateDirectoryA(out.c_str(), nullptr);

    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    std::string msg = runAnalysisPipeline(folder, false);
    QueryPerformanceCounter(&t1);
    double ms = (t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;

    const AnalysisResults& r = g_analysisResults;

    if (FILE* f = openOut(out, "summary.csv")) {
        fprintf(f, "message,%s\n", csv(msg).c_str());
        fprintf(f, "valid,%d\n", r.valid ? 1 : 0);
        fprintf(f, "totalPairs,%d\nflaggedCount,%d\nexactDuplicates,%d\n"
                   "highSimilarity,%d\nmaxScore,%.17g\naverageScore,%.17g\n",
                r.totalPairs, r.flaggedCount, r.exactDuplicates,
                r.highSimilarity, r.maxScore, r.averageScore);
        for (const auto& u : r.unclassifiedFiles)
            fprintf(f, "unclassified,%s\n", csv(u.key).c_str());
        for (const auto& l : r.languageMismatchFiles)
            fprintf(f, "languageMismatch,%s\n", csv(l).c_str());
        fclose(f);
    }

    if (FILE* f = openOut(out, "pairs.csv")) {
        fprintf(f, "fileA,fileB,token,style,combined,label,flagged\n");
        for (const auto& p : r.allPairs)
            fprintf(f, "%s,%s,%.17g,%.17g,%.17g,%s,%d\n",
                    csv(p.filenameA).c_str(), csv(p.filenameB).c_str(),
                    p.tokenScore, p.styleScore, p.combinedScore,
                    csv(p.label).c_str(), p.flagged ? 1 : 0);
        fclose(f);
    }

    if (FILE* f = openOut(out, "flagged.csv")) {
        for (const auto& p : r.flaggedPairs) {
            fprintf(f, "%s,%s,%.17g,%.17g,%.17g,%s",
                    csv(p.filenameA).c_str(), csv(p.filenameB).c_str(),
                    p.tokenScore, p.styleScore, p.combinedScore,
                    csv(p.similarityLabel).c_str());
            writeSide(f, p.studentA);
            writeSide(f, p.studentB);
            fprintf(f, ",%s\n", csv(p.interpretation).c_str());
        }
        fclose(f);
    }

    if (FILE* f = openOut(out, "profiles.csv")) {
        for (const auto& p : r.profiles) {
            fprintf(f, "%s,%d", csv(p.name).c_str(), p.submissionCount);
            for (double v : p.featureVector) fprintf(f, ",%.17g", v);
            for (const auto& n : p.styleNotes) fprintf(f, ",%s", csv(n).c_str());
            fprintf(f, "\n");
            for (size_t i = 0; i < p.submissionPaths.size(); ++i) {
                fprintf(f, "  %s", csv(baseName(p.submissionPaths[i])).c_str());
                for (double v : p.submissionRaw[i]) fprintf(f, ",%.17g", v);
                fprintf(f, "\n");
            }
        }
        fclose(f);
    }

    // hashes.txt is what run_golden.bat compares. Hashing (rather than
    // committing every CSV) keeps the four-block dataset's 46 MB
    // flagged.csv out of git while still catching any byte of drift.
    if (FILE* f = openOut(out, "hashes.txt")) {
        for (const char* name : {"summary.csv", "pairs.csv", "flagged.csv", "profiles.csv"})
            fprintf(f, "%s %s\n", audit_sha256File(out + "\\" + name).c_str(), name);
        fclose(f);
    }

    // Timing is informational only — kept out of the compared files.
    printf("%s: %.0f ms, %d pairs, %d flagged — %s\n",
           baseName(folder).c_str(), ms, r.totalPairs, r.flaggedCount, msg.c_str());
    return r.valid ? 0 : 1;
}
