// Reading the split file and listing the HPatches and MIRFlickr images

#include "dataset.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace std;

namespace fs = filesystem;

Split parseSplit(const string& s) {
    if (s == "train") return Split::Train;
    if (s == "validation") return Split::Validation;
    if (s == "test") return Split::Test;
    throw runtime_error("unknown split '" + s + "'");
}

string splitName(Split s) {
    switch (s) {
        case Split::Train: return "train";
        case Split::Validation: return "validation";
        case Split::Test: return "test";
    }
    return "?";
}

string typeName(SeqType t) {
    return t == SeqType::Illumination ? "illumination" : "viewpoint";
}

vector<Sequence> readSplitFile(const string& path) {
    ifstream in(path);
    if (!in) throw runtime_error("cannot open split file " + path);
    vector<Sequence> seqs;
    set<string> seen;  // to detect a sequence listed twice
    string line;
    int lineNo = 0;
    while (getline(in, line)) {
        ++lineNo;
        istringstream ls(line);
        string name, split, extra;
        if (!(ls >> name)) continue;  // blank line
        // Prefix for error messages, e.g. "split.txt:12: "
        const string where = path + ":" + to_string(lineNo) + ": ";
        // Each line must have exactly two words
        if (!(ls >> split) || (ls >> extra))
            throw runtime_error(where + "expected '<sequence> <split>'");
        Sequence s;
        s.name = name;
        try {
            s.split = parseSplit(split);
        } catch (const exception& e) {
            throw runtime_error(where + e.what());
        }
        // The sequence type is encoded in the name: i_* = illumination, v_* = viewpoint
        if (name.rfind("i_", 0) == 0) s.type = SeqType::Illumination;
        else if (name.rfind("v_", 0) == 0) s.type = SeqType::Viewpoint;
        else throw runtime_error(where + "sequence name must start with i_ or v_");

        if (!seen.insert(name).second)
            throw runtime_error(where + "duplicate sequence " + name);
        seqs.push_back(s);
    }
    if (seqs.empty()) throw runtime_error("split file " + path + " is empty");
    return seqs;
}

vector<Sequence> sequencesIn(const vector<Sequence>& all, Split s) {
    vector<Sequence> out;
    for (const auto& q : all)
        if (q.split == s) out.push_back(q);
    return out;
}

string hpatchesImagePath(const string& root, const string& seq, int idx) {
    return (fs::path(root) / seq / (to_string(idx) + ".ppm")).string();
}

void checkHPatches(const string& root, const vector<Sequence>& seqs) {
    if (!fs::is_directory(root)) throw runtime_error("HPatches directory not found: " + root);
    // Collect every missing file so the error shows how many are missing, not just the first
    vector<string> missing;
    for (const auto& s : seqs)
        for (int i = 1; i <= kImagesPerSequence; ++i) {
            string p = hpatchesImagePath(root, s.name, i);
            if (!fs::is_regular_file(p)) missing.push_back(p);
        }
    if (!missing.empty()) {
        string msg = to_string(missing.size()) + " HPatches image(s) missing, e.g.";
        for (size_t i = 0; i < missing.size() && i < 5; ++i) msg += "\n  " + missing[i];
        throw runtime_error(msg);
    }
}

vector<string> listMirflickr(const string& dir) {
    if (!fs::is_directory(dir)) throw runtime_error("MIRFlickr directory not found: " + dir);
    // Only files named im<N>.jpg are images. Other files (e.g. the doc/ folder) are skipped
    static const regex pattern(R"(im(\d+)\.jpg)");
    // (N, file name) pairs, so sorting orders the images by number, not alphabetically
    vector<pair<long, string>> found;
    for (const auto& e : fs::directory_iterator(dir)) {
        if (!e.is_regular_file()) continue;
        const string name = e.path().filename().string();
        smatch m;
        if (regex_match(name, m, pattern)) found.emplace_back(stol(m[1].str()), name);
    }
    sort(found.begin(), found.end());
    vector<string> files;
    files.reserve(found.size());
    for (auto& f : found) files.push_back(move(f.second));
    return files;
}

string mirflickrImagePath(const string& dir, const string& file) {
    return (fs::path(dir) / file).string();
}
