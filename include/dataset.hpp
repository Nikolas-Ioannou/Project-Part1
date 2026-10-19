#pragma once

#include <string>
#include <vector>

enum class Split { Train, Validation, Test };
enum class SeqType { Illumination, Viewpoint };

struct Sequence {
    std::string name;  // directory name, e.g. "v_bird"
    Split split;
    SeqType type;      // from the "i_" / "v_" prefix
};

constexpr int kImagesPerSequence = 6;

Split parseSplit(const std::string& s);
std::string splitName(Split s);
std::string typeName(SeqType t);

// Reads "<sequence> <train|validation|test>" lines. Throws std::runtime_error on bad input.
std::vector<Sequence> readSplitFile(const std::string& path);

std::vector<Sequence> sequencesIn(const std::vector<Sequence>& all, Split s);

// Path of image I_idx (idx in 1..6) of a sequence: <root>/<seq>/<idx>.ppm
std::string hpatchesImagePath(const std::string& root, const std::string& seq, int idx);

// Throws if any sequence directory or image 1..6.ppm is missing.
void checkHPatches(const std::string& root, const std::vector<Sequence>& seqs);

// File names im<N>.jpg in the directory, sorted by increasing N.
std::vector<std::string> listMirflickr(const std::string& dir);

std::string mirflickrImagePath(const std::string& dir, const std::string& file);
