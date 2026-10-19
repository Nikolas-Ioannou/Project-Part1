#include "dataset.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace fs = std::filesystem;

Split parseSplit(const std::string& s) {
    if (s == "train") return Split::Train;
    if (s == "validation") return Split::Validation;
    if (s == "test") return Split::Test;
    throw std::runtime_error("unknown split '" + s + "'");
}

std::string splitName(Split s) {
    switch (s) {
        case Split::Train: return "train";
        case Split::Validation: return "validation";
        case Split::Test: return "test";
    }
    return "?";
}

std::string typeName(SeqType t) {
    return t == SeqType::Illumination ? "illumination" : "viewpoint";
}

std::vector<Sequence> readSplitFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open split file " + path);

    std::vector<Sequence> seqs;
    std::set<std::string> seen;
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::istringstream ls(line);
        std::string name, split, extra;
        if (!(ls >> name)) continue;  // blank line
        const std::string where = path + ":" + std::to_string(lineNo) + ": ";
        if (!(ls >> split) || (ls >> extra))
            throw std::runtime_error(where + "expected '<sequence> <split>'");

        Sequence s;
        s.name = name;
        try {
            s.split = parseSplit(split);
        } catch (const std::exception& e) {
            throw std::runtime_error(where + e.what());
        }
        if (name.rfind("i_", 0) == 0) s.type = SeqType::Illumination;
        else if (name.rfind("v_", 0) == 0) s.type = SeqType::Viewpoint;
        else throw std::runtime_error(where + "sequence name must start with i_ or v_");

        if (!seen.insert(name).second)
            throw std::runtime_error(where + "duplicate sequence " + name);
        seqs.push_back(s);
    }
    if (seqs.empty()) throw std::runtime_error("split file " + path + " is empty");
    return seqs;
}

std::vector<Sequence> sequencesIn(const std::vector<Sequence>& all, Split s) {
    std::vector<Sequence> out;
    for (const auto& q : all)
        if (q.split == s) out.push_back(q);
    return out;
}

std::string hpatchesImagePath(const std::string& root, const std::string& seq, int idx) {
    return (fs::path(root) / seq / (std::to_string(idx) + ".ppm")).string();
}

void checkHPatches(const std::string& root, const std::vector<Sequence>& seqs) {
    if (!fs::is_directory(root)) throw std::runtime_error("HPatches directory not found: " + root);
    std::vector<std::string> missing;
    for (const auto& s : seqs)
        for (int i = 1; i <= kImagesPerSequence; ++i) {
            std::string p = hpatchesImagePath(root, s.name, i);
            if (!fs::is_regular_file(p)) missing.push_back(p);
        }
    if (!missing.empty()) {
        std::string msg = std::to_string(missing.size()) + " HPatches image(s) missing, e.g.";
        for (size_t i = 0; i < missing.size() && i < 5; ++i) msg += "\n  " + missing[i];
        throw std::runtime_error(msg);
    }
}

std::vector<std::string> listMirflickr(const std::string& dir) {
    if (!fs::is_directory(dir)) throw std::runtime_error("MIRFlickr directory not found: " + dir);
    static const std::regex pattern(R"(im(\d+)\.jpg)");
    std::vector<std::pair<long, std::string>> found;
    for (const auto& e : fs::directory_iterator(dir)) {
        if (!e.is_regular_file()) continue;
        const std::string name = e.path().filename().string();
        std::smatch m;
        if (std::regex_match(name, m, pattern)) found.emplace_back(std::stol(m[1].str()), name);
    }
    std::sort(found.begin(), found.end());
    std::vector<std::string> files;
    files.reserve(found.size());
    for (auto& f : found) files.push_back(std::move(f.second));
    return files;
}

std::string mirflickrImagePath(const std::string& dir, const std::string& file) {
    return (fs::path(dir) / file).string();
}
