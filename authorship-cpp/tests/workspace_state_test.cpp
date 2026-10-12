// ─────────────────────────────────────────────────────────────
// workspace_state_test.cpp — unit tests for workspace_state.cpp
// ("Continue previous session", 1.3.0). Run by
// tests\run_workspace_tests.bat. No framework: each CHECK prints the
// failing line; exit code = number of failures.
// ─────────────────────────────────────────────────────────────

#include "../include/workspace_state.h"
#include "../include/app_paths.h"

#include <windows.h>

#include <cstdio>
#include <fstream>
#include <string>

static int g_failures = 0;
static int g_checks   = 0;
#define CHECK(cond) do { ++g_checks; if (!(cond)) { ++g_failures; \
    std::printf("  FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

static WorkspaceState sample() {
    WorkspaceState ws;
    ws.appVersion = "1.3.0";
    ws.savedAt    = "2026-10-12 14:05";
    ws.dataset    = "C:\\Users\\Prof\\Documents\\BSCS-7\\Programming1";
    ws.fileCount  = 553;
    ws.page       = WS_PAGE_CLASSES;
    ws.block      = "Programming 1 - BSCS 7";
    ws.student    = "Pe\xC3\xB1" "a \"Quoted\" \\ Name";   // UTF-8 ñ, quotes, backslash
    return ws;
}

static void testRoundTrip() {
    WorkspaceState in = sample(), out;
    CHECK(parseWorkspace(serializeWorkspace(in), out) == WorkspaceParse::Ok);
    CHECK(out == in);

    WorkspaceState f = sample();
    f.page = WS_PAGE_FLAGGED; f.block.clear(); f.student.clear();
    f.pairFileA = "Final_Exam_Rah"; f.pairFileB = "Final_Exam_Ranu";
    f.pairLabel = "Rah vs Ranu";
    CHECK(parseWorkspace(serializeWorkspace(f), out) == WorkspaceParse::Ok);
    CHECK(out == f);
}

static void testMalformed() {
    WorkspaceState out;
    const char* bad[] = {
        "", "{", "[]", "null", "garbage",
        "{\"format\":\"calss-workspace\"}",                                 // no version
        "{\"format\":\"other\",\"version\":1,\"dataset\":\"C:\\\\x\",\"page\":\"overview\"}",
        "{\"format\":\"calss-workspace\",\"version\":\"1\"}",               // version as string
        "{\"format\":\"calss-workspace\",\"version\":0,\"dataset\":\"C:\\\\x\",\"page\":\"overview\"}",
        "{\"format\":\"calss-workspace\",\"version\":1,\"dataset\":{\"a\":1},\"page\":\"overview\"}",
        "{\"format\":\"calss-workspace\",\"version\":1,\"dataset\":\"C:\\\\x\",\"page\":\"overview\"} trailing",
        "{\"format\":\"calss-workspace\",\"version\":1.5,\"dataset\":\"C:\\\\x\",\"page\":\"overview\"}",
        "{\"format\":\"calss-workspace\",\"version\":1,\"dataset\":\"C:\\\\x\",\"page\":\"overview\",}",
        "{\"format\":\"calss-workspace\",\"version\":1,\"dataset\":\"unterminated}",
        "{\"format\":\"calss-workspace\",\"version\":1,\"dataset\":\"bad\\q\",\"page\":\"overview\"}",
        "{\"format\":\"calss-workspace\",\"version\":1,\"dataset\":\"\\ud800\",\"page\":\"overview\"}",
        "{\"format\":\"calss-workspace\",\"version\":1,\"dataset\":\"C:\\\\x\",\"page\":\"overview\",\"file_count\":\"3\"}",
        "{\"format\":\"calss-workspace\",\"version\":99999999999,\"dataset\":\"C:\\\\x\",\"page\":\"overview\"}",
    };
    for (const char* t : bad) {
        WorkspaceParse r = parseWorkspace(t, out);
        if (r != WorkspaceParse::Malformed) std::printf("  (input: %s)\n", t);
        CHECK(r == WorkspaceParse::Malformed);
        CHECK(out.dataset.empty());
    }
    // Raw control character inside a string
    std::string ctl = "{\"format\":\"calss-workspace\",\"version\":1,\"dataset\":\"C:\x01x\",\"page\":\"overview\"}";
    CHECK(parseWorkspace(ctl, out) == WorkspaceParse::Malformed);
    // Oversized
    std::string big = serializeWorkspace(sample());
    big.insert(1, std::string(WORKSPACE_MAX_FILE_BYTES, ' '));
    CHECK(parseWorkspace(big, out) == WorkspaceParse::Malformed);
}

static void testFutureAndInvalid() {
    WorkspaceState out;
    WorkspaceState ws = sample();
    ws.version = WORKSPACE_FORMAT_VERSION + 1;
    CHECK(parseWorkspace(serializeWorkspace(ws), out) == WorkspaceParse::FutureVersion);
    CHECK(out.dataset.empty());

    ws = sample(); ws.dataset.clear();
    CHECK(parseWorkspace(serializeWorkspace(ws), out) == WorkspaceParse::Invalid);
    ws = sample(); ws.page = "settings";
    CHECK(parseWorkspace(serializeWorkspace(ws), out) == WorkspaceParse::Invalid);
    ws = sample(); ws.student = std::string("a\nb");   // escaped control char
    CHECK(parseWorkspace(serializeWorkspace(ws), out) == WorkspaceParse::Invalid);
    ws = sample(); ws.student = std::string(WORKSPACE_MAX_NAME_CHARS + 1, 'x');
    CHECK(parseWorkspace(serializeWorkspace(ws), out) == WorkspaceParse::Invalid);
}

static void testFieldScoping() {
    WorkspaceState out;
    // Overview page carrying stale student/pair fields: they are dropped.
    WorkspaceState ws = sample();
    ws.page = WS_PAGE_OVERVIEW; ws.pairFileA = "a"; ws.pairFileB = "b";
    CHECK(parseWorkspace(serializeWorkspace(ws), out) == WorkspaceParse::Ok);
    CHECK(out.block.empty() && out.student.empty() && out.pairFileA.empty());
    // Flagged page with only one pair file: pair dropped, page kept.
    ws = sample(); ws.page = WS_PAGE_FLAGGED; ws.pairFileA = "a"; ws.pairLabel = "x";
    CHECK(parseWorkspace(serializeWorkspace(ws), out) == WorkspaceParse::Ok);
    CHECK(out.page == WS_PAGE_FLAGGED && out.pairFileA.empty() && out.pairLabel.empty());
    // Unknown keys are ignored; missing optional keys are fine; BOM ok.
    std::string t = "\xEF\xBB\xBF{ \"format\": \"calss-workspace\", \"version\": 1,"
                    " \"dataset\": \"D:\\\\x\", \"page\": \"overview\", \"future_key\": 5 }";
    CHECK(parseWorkspace(t, out) == WorkspaceParse::Ok);
    CHECK(out.dataset == "D:\\x" && out.fileCount == 0);
    // \u escapes decode to UTF-8 (ñ, and a surrogate pair)
    t = "{\"format\":\"calss-workspace\",\"version\":1,\"dataset\":\"C:\\\\Pe\\u00f1a\\ud83d\\ude00\",\"page\":\"overview\"}";
    CHECK(parseWorkspace(t, out) == WorkspaceParse::Ok);
    CHECK(out.dataset == "C:\\Pe\xC3\xB1" "a\xF0\x9F\x98\x80");
}

static void testDecisions() {
    CHECK(classifyDataset(false, false, 0, false) == DatasetCheck::Missing);
    CHECK(classifyDataset(false, false, 0, true)  == DatasetCheck::MissingClassroom);
    CHECK(classifyDataset(true, false, 0, false)  == DatasetCheck::NotAFolder);
    CHECK(classifyDataset(true, true, 0, true)    == DatasetCheck::NoCFiles);
    CHECK(classifyDataset(true, true, 12, true)   == DatasetCheck::Ok);

    std::string cls = "C:\\Users\\P\\AppData\\Local\\CALSS\\Classroom";
    CHECK(pathIsInside("c:/users/p/appdata/local/calss/classroom/Prog1", cls));
    CHECK(pathIsInside(cls + "\\", cls));
    CHECK(!pathIsInside("C:\\Users\\P\\AppData\\Local\\CALSS\\ClassroomOld\\x", cls));
    CHECK(!pathIsInside("D:\\data", cls));
    CHECK(!pathIsInside("", cls));

    std::vector<std::string> names = {"Ana", "Ben", "ana"};
    CHECK(findByName(names, "ana") == 2);   // exact, case-sensitive identity
    CHECK(findByName(names, "Cy") == -1);
    CHECK(findByName(names, "") == -1);

    std::vector<std::pair<std::string, std::string>> pairs = {{"a", "b"}, {"c", "d"}};
    CHECK(findPair(pairs, "d", "c") == 1);
    CHECK(findPair(pairs, "a", "b") == 0);
    CHECK(findPair(pairs, "a", "c") == -1);
    CHECK(findPair(pairs, "", "b") == -1);

    WorkspaceState ws = sample();
    CHECK(describeLocation(ws) ==
          "Classes \xE2\x86\x92 Programming 1 - BSCS 7 \xE2\x86\x92 " + ws.student);
    ws.block.clear(); ws.student.clear();
    CHECK(describeLocation(ws) == "Classes \xE2\x86\x92 All classes");
    ws.page = WS_PAGE_FLAGGED; ws.pairLabel = "A vs B";
    CHECK(describeLocation(ws) == "Flagged Pairs \xE2\x86\x92 A vs B");
    ws.page = WS_PAGE_OVERVIEW;
    CHECK(describeLocation(ws) == "Overview");
    CHECK(datasetDisplayName("C:\\a\\Programming1\\") == "Programming1");
    CHECK(datasetDisplayName("Programming1") == "Programming1");
}

static void testFiles(const std::string& dir) {
    std::string path = dir + "\\workspace.json";
    DeleteFileW(widenUtf8(path).c_str());
    WorkspaceState out;
    WorkspaceParse st = WorkspaceParse::Ok;
    CHECK(!loadWorkspaceFile(path, out, &st));       // missing: false, status untouched
    CHECK(st == WorkspaceParse::Ok);

    WorkspaceState ws = sample();
    CHECK(saveWorkspaceFile(path, ws));
    CHECK(loadWorkspaceFile(path, out, &st) && st == WorkspaceParse::Ok && out == ws);
    // Overwrite in place (replace path)
    ws.page = WS_PAGE_OVERVIEW; ws.block.clear(); ws.student.clear();
    CHECK(saveWorkspaceFile(path, ws));
    CHECK(loadWorkspaceFile(path, out) && out == ws);
    CHECK(GetFileAttributesW(widenUtf8(path + ".tmp").c_str()) == INVALID_FILE_ATTRIBUTES);

    // Corrupted file: reported as Malformed, no crash.
    { std::ofstream f(pathFromUtf8(path), std::ios::binary | std::ios::trunc); f << "{\"format\": tru"; }
    CHECK(!loadWorkspaceFile(path, out, &st) && st == WorkspaceParse::Malformed);
    // Oversized file
    { std::ofstream f(pathFromUtf8(path), std::ios::binary | std::ios::trunc);
      f << std::string(WORKSPACE_MAX_FILE_BYTES + 10, ' '); }
    CHECK(!loadWorkspaceFile(path, out, &st) && st == WorkspaceParse::Malformed);
    // Empty file
    { std::ofstream f(pathFromUtf8(path), std::ios::binary | std::ios::trunc); }
    CHECK(!loadWorkspaceFile(path, out, &st) && st == WorkspaceParse::Malformed);
    DeleteFileW(widenUtf8(path).c_str());
}

int main(int argc, char** argv) {
    std::string dir = argc > 1 ? argv[1] : ".";
    std::printf("workspace_state tests\n");
    testRoundTrip();
    testMalformed();
    testFutureAndInvalid();
    testFieldScoping();
    testDecisions();
    testFiles(dir);
    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures;
}
