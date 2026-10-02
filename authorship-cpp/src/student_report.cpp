// ─────────────────────────────────────────────────────────────
// student_report.cpp - Code Authorship Likelihood Scoring System
// generateStudentSimilarityReport(): the printable per-student HTML
// report behind the student page's "Generate Similarity Report".
//
// Moved unchanged out of main.cpp (2026-10-02). Reads the results of
// the last run from g_analysisResults (analysis_pipeline.cpp).
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

#include "results_data.h"

// Defined in analysis_pipeline.cpp.
extern AnalysisResults g_analysisResults;

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
    std::string outputPath = appReportsDirU8() + "\\similarity_report_" +
                             safeName + ".html";

    std::ofstream out(pathFromUtf8(outputPath));
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
