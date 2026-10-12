// ─────────────────────────────────────────────────────────────
// workspace_state.cpp
// Most-recent-session file (1.3.0) — see workspace_state.h.
//
// The file is a flat JSON object of strings and integers, written only
// by CALSS. The reader is a small strict parser for exactly that shape
// (no JSON library in this project): anything else — nesting, arrays,
// floats, a wrong tag, an oversized file — is rejected as Malformed and
// CALSS simply starts normally.
// ─────────────────────────────────────────────────────────────

#include "../include/workspace_state.h"
#include "../include/app_paths.h"

#include <windows.h>

#include <cstdint>
#include <fstream>
#include <map>

// ── Comparison ───────────────────────────────────────────────

bool sameWorkspaceContent(const WorkspaceState& a, const WorkspaceState& b) {
    return a.dataset == b.dataset && a.fileCount == b.fileCount &&
           a.page == b.page && a.block == b.block && a.student == b.student &&
           a.pairFileA == b.pairFileA && a.pairFileB == b.pairFileB &&
           a.pairLabel == b.pairLabel;
}

bool WorkspaceState::operator==(const WorkspaceState& o) const {
    return version == o.version && appVersion == o.appVersion &&
           savedAt == o.savedAt && sameWorkspaceContent(*this, o);
}

// ── Writing ──────────────────────────────────────────────────

static void appendJsonString(std::string& out, const std::string& s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += (char)c;  // UTF-8 bytes pass through (valid JSON)
                }
        }
    }
    out += '"';
}

std::string serializeWorkspace(const WorkspaceState& ws) {
    std::string o = "{\n";
    auto str = [&o](const char* key, const std::string& v, bool last = false) {
        o += "  ";
        appendJsonString(o, key);
        o += ": ";
        appendJsonString(o, v);
        o += last ? "\n" : ",\n";
    };
    auto num = [&o](const char* key, int v) {
        o += "  ";
        appendJsonString(o, key);
        o += ": " + std::to_string(v) + ",\n";
    };
    str("format", WORKSPACE_FORMAT_TAG);
    num("version", ws.version);
    str("app_version", ws.appVersion);
    str("saved_at", ws.savedAt);
    str("dataset", ws.dataset);
    num("file_count", ws.fileCount);
    str("page", ws.page);
    str("block", ws.block);
    str("student", ws.student);
    str("pair_file_a", ws.pairFileA);
    str("pair_file_b", ws.pairFileB);
    str("pair_label", ws.pairLabel, true);
    o += "}\n";
    return o;
}

// ── Reading ──────────────────────────────────────────────────

namespace {

struct JsonValue {
    bool        isString = false;
    std::string s;
    long long   n = 0;
};

class FlatJsonParser {
public:
    explicit FlatJsonParser(const std::string& t) : t_(t) {}

    bool parse(std::map<std::string, JsonValue>& out) {
        skipWs();
        if (!eat('{')) return false;
        skipWs();
        if (eat('}')) return atEnd();
        for (;;) {
            skipWs();
            std::string key;
            if (!parseString(key)) return false;
            skipWs();
            if (!eat(':')) return false;
            skipWs();
            JsonValue v;
            if (peek() == '"') {
                v.isString = true;
                if (!parseString(v.s)) return false;
            } else if (!parseInt(v.n)) {
                return false;
            }
            out[key] = v;
            if (out.size() > 64) return false;
            skipWs();
            if (eat(',')) continue;
            if (eat('}')) return atEnd();
            return false;
        }
    }

private:
    const std::string& t_;
    size_t i_ = 0;

    char peek() const { return i_ < t_.size() ? t_[i_] : '\0'; }
    bool eat(char c) { if (peek() == c && i_ < t_.size()) { ++i_; return true; } return false; }
    void skipWs() {
        while (i_ < t_.size() &&
               (t_[i_] == ' ' || t_[i_] == '\t' || t_[i_] == '\n' || t_[i_] == '\r'))
            ++i_;
    }
    bool atEnd() { skipWs(); return i_ == t_.size(); }

    static void appendUtf8(std::string& s, uint32_t cp) {
        if (cp < 0x80) s += (char)cp;
        else if (cp < 0x800) { s += (char)(0xC0 | (cp >> 6)); s += (char)(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) {
            s += (char)(0xE0 | (cp >> 12));
            s += (char)(0x80 | ((cp >> 6) & 0x3F));
            s += (char)(0x80 | (cp & 0x3F));
        } else {
            s += (char)(0xF0 | (cp >> 18));
            s += (char)(0x80 | ((cp >> 12) & 0x3F));
            s += (char)(0x80 | ((cp >> 6) & 0x3F));
            s += (char)(0x80 | (cp & 0x3F));
        }
    }

    bool hex4(uint32_t& v) {
        if (i_ + 4 > t_.size()) return false;
        v = 0;
        for (int k = 0; k < 4; ++k) {
            char c = t_[i_++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= (uint32_t)(c - '0');
            else if (c >= 'a' && c <= 'f') v |= (uint32_t)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= (uint32_t)(c - 'A' + 10);
            else return false;
        }
        return true;
    }

    bool parseString(std::string& out) {
        if (!eat('"')) return false;
        out.clear();
        while (i_ < t_.size()) {
            unsigned char c = (unsigned char)t_[i_++];
            if (c == '"') return true;
            if (c < 0x20) return false;          // raw control char: not JSON
            if (c != '\\') { out += (char)c; }
            else {
                if (i_ >= t_.size()) return false;
                char e = t_[i_++];
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': {
                        uint32_t cp;
                        if (!hex4(cp)) return false;
                        if (cp >= 0xD800 && cp <= 0xDBFF) {      // surrogate pair
                            uint32_t lo;
                            if (!eat('\\') || !eat('u') || !hex4(lo) ||
                                lo < 0xDC00 || lo > 0xDFFF) return false;
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                            return false;
                        }
                        appendUtf8(out, cp);
                        break;
                    }
                    default: return false;
                }
            }
            if (out.size() > WORKSPACE_MAX_PATH_CHARS) return false;
        }
        return false;  // unterminated
    }

    bool parseInt(long long& v) {
        bool neg = eat('-');
        if (peek() < '0' || peek() > '9') return false;
        v = 0;
        int digits = 0;
        while (peek() >= '0' && peek() <= '9') {
            v = v * 10 + (t_[i_++] - '0');
            if (++digits > 9) return false;  // far beyond any real value
        }
        if (peek() == '.' || peek() == 'e' || peek() == 'E') return false;
        if (neg) v = -v;
        return true;
    }
};

bool hasControlChars(const std::string& s) {
    for (unsigned char c : s) if (c < 0x20 || c == 0x7F) return true;
    return false;
}

} // namespace

WorkspaceParse parseWorkspace(const std::string& text, WorkspaceState& out) {
    out = WorkspaceState{};
    if (text.empty() || text.size() > WORKSPACE_MAX_FILE_BYTES)
        return WorkspaceParse::Malformed;

    // Tolerate a UTF-8 BOM (e.g. after a hand edit in Notepad).
    std::string body = text;
    if (body.size() >= 3 && (unsigned char)body[0] == 0xEF &&
        (unsigned char)body[1] == 0xBB && (unsigned char)body[2] == 0xBF)
        body.erase(0, 3);

    std::map<std::string, JsonValue> kv;
    FlatJsonParser p(body);
    if (!p.parse(kv)) return WorkspaceParse::Malformed;

    auto getStr = [&kv](const char* key, std::string& dst) -> bool {
        auto it = kv.find(key);
        if (it == kv.end()) { dst.clear(); return true; }   // optional
        if (!it->second.isString) return false;
        dst = it->second.s;
        return true;
    };

    auto fmt = kv.find("format");
    if (fmt == kv.end() || !fmt->second.isString || fmt->second.s != WORKSPACE_FORMAT_TAG)
        return WorkspaceParse::Malformed;
    auto ver = kv.find("version");
    if (ver == kv.end() || ver->second.isString || ver->second.n < 1)
        return WorkspaceParse::Malformed;
    if (ver->second.n > WORKSPACE_FORMAT_VERSION)
        return WorkspaceParse::FutureVersion;

    WorkspaceState ws;
    ws.version = (int)ver->second.n;
    if (!getStr("app_version", ws.appVersion) || !getStr("saved_at", ws.savedAt) ||
        !getStr("dataset", ws.dataset) || !getStr("page", ws.page) ||
        !getStr("block", ws.block) || !getStr("student", ws.student) ||
        !getStr("pair_file_a", ws.pairFileA) || !getStr("pair_file_b", ws.pairFileB) ||
        !getStr("pair_label", ws.pairLabel))
        return WorkspaceParse::Malformed;
    auto fc = kv.find("file_count");
    if (fc != kv.end()) {
        if (fc->second.isString) return WorkspaceParse::Malformed;
        ws.fileCount = (fc->second.n >= 0 && fc->second.n < 10000000) ? (int)fc->second.n : 0;
    }

    // Usability checks.
    if (ws.dataset.empty() || ws.dataset.size() > WORKSPACE_MAX_PATH_CHARS ||
        hasControlChars(ws.dataset))
        return WorkspaceParse::Invalid;
    if (ws.page != WS_PAGE_OVERVIEW && ws.page != WS_PAGE_CLASSES && ws.page != WS_PAGE_FLAGGED)
        return WorkspaceParse::Invalid;
    for (std::string* s : {&ws.appVersion, &ws.savedAt, &ws.block, &ws.student,
                           &ws.pairFileA, &ws.pairFileB, &ws.pairLabel}) {
        if (s->size() > WORKSPACE_MAX_NAME_CHARS || hasControlChars(*s))
            return WorkspaceParse::Invalid;
    }
    // Fields that don't belong to the saved page are dropped, not trusted.
    if (ws.page != WS_PAGE_CLASSES) { ws.block.clear(); ws.student.clear(); }
    if (ws.page != WS_PAGE_FLAGGED || ws.pairFileA.empty() || ws.pairFileB.empty()) {
        ws.pairFileA.clear(); ws.pairFileB.clear(); ws.pairLabel.clear();
    }

    out = ws;
    return WorkspaceParse::Ok;
}

// ── Files ────────────────────────────────────────────────────

std::string workspaceFilePath() {
    return appDataDirU8() + "\\workspace.json";
}

bool loadWorkspaceFile(const std::string& path, WorkspaceState& out,
                       WorkspaceParse* status) {
    out = WorkspaceState{};
    std::ifstream f(pathFromUtf8(path), std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    std::streamoff size = f.tellg();
    if (size < 0 || (size_t)size > WORKSPACE_MAX_FILE_BYTES) {
        if (status) *status = WorkspaceParse::Malformed;
        return false;
    }
    f.seekg(0, std::ios::beg);
    std::string text((size_t)size, '\0');
    if (size > 0 && !f.read(&text[0], size)) {
        if (status) *status = WorkspaceParse::Malformed;
        return false;
    }
    WorkspaceParse r = parseWorkspace(text, out);
    if (status) *status = r;
    return r == WorkspaceParse::Ok;
}

bool saveWorkspaceFile(const std::string& path, const WorkspaceState& ws) {
    std::string tmp = path + ".tmp";
    {
        std::ofstream f(pathFromUtf8(tmp), std::ios::binary | std::ios::trunc);
        if (!f) return false;
        std::string text = serializeWorkspace(ws);
        f.write(text.data(), (std::streamsize)text.size());
        f.flush();
        if (!f) { f.close(); DeleteFileW(widenUtf8(tmp).c_str()); return false; }
    }
    if (!MoveFileExW(widenUtf8(tmp).c_str(), widenUtf8(path).c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(widenUtf8(tmp).c_str());
        return false;
    }
    return true;
}

// ── Decisions ────────────────────────────────────────────────

DatasetCheck classifyDataset(bool exists, bool isDirectory, int cFileCount,
                             bool isClassroomDownload) {
    if (!exists) return isClassroomDownload ? DatasetCheck::MissingClassroom
                                            : DatasetCheck::Missing;
    if (!isDirectory) return DatasetCheck::NotAFolder;
    if (cFileCount <= 0) return DatasetCheck::NoCFiles;
    return DatasetCheck::Ok;
}

static std::string normPathForCompare(const std::string& p) {
    std::string s;
    s.reserve(p.size());
    for (char c : p) {
        if (c == '/') c = '\\';
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        s += c;
    }
    while (s.size() > 3 && s.back() == '\\') s.pop_back();
    return s;
}

bool pathIsInside(const std::string& path, const std::string& dir) {
    if (path.empty() || dir.empty()) return false;
    std::string p = normPathForCompare(path), d = normPathForCompare(dir);
    if (p == d) return true;
    return p.size() > d.size() && p.compare(0, d.size(), d) == 0 && p[d.size()] == '\\';
}

int findByName(const std::vector<std::string>& names, const std::string& name) {
    if (name.empty()) return -1;
    for (size_t i = 0; i < names.size(); ++i)
        if (names[i] == name) return (int)i;
    return -1;
}

int findPair(const std::vector<std::pair<std::string, std::string>>& pairs,
             const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return -1;
    for (size_t i = 0; i < pairs.size(); ++i) {
        const auto& p = pairs[i];
        if ((p.first == a && p.second == b) || (p.first == b && p.second == a))
            return (int)i;
    }
    return -1;
}

std::string describeLocation(const WorkspaceState& ws) {
    static const char* ARROW = " \xE2\x86\x92 ";  // " → "
    if (ws.page == WS_PAGE_CLASSES) {
        std::string s = "Classes";
        s += ARROW;
        s += ws.block.empty() ? "All classes" : ws.block;
        if (!ws.student.empty()) { s += ARROW; s += ws.student; }
        return s;
    }
    if (ws.page == WS_PAGE_FLAGGED) {
        std::string s = "Flagged Pairs";
        if (!ws.pairLabel.empty()) { s += ARROW; s += ws.pairLabel; }
        return s;
    }
    return "Overview";
}

std::string datasetDisplayName(const std::string& path) {
    std::string p = path;
    while (p.size() > 3 && (p.back() == '\\' || p.back() == '/')) p.pop_back();
    size_t slash = p.find_last_of("\\/");
    return slash == std::string::npos ? p : p.substr(slash + 1);
}
