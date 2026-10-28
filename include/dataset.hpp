// Access to the two datasets (section II of the assignment)
//  - HPatches: 116 sequences of 6 images (<root>/<sequence>/1.ppm .. 6.ppm). Image 1 of each
//    sequence is the query and images 2..6 are the relevant images. The split file assigns
//    each sequence to train, validation or test
//  - MIRFlickr-25K: 25000 distractor photos named im1.jpg .. im25000.jpg

#pragma once

#include <string>
#include <vector>

enum class Split { Train, Validation, Test };
enum class SeqType { Illumination, Viewpoint };

// One HPatches sequence, i.e. one line of the split file
struct Sequence {
    std::string name;  // directory name, e.g. "v_bird"
    Split split;       // which part of the split it belongs to
    SeqType type;      // from the "i_" / "v_" prefix of the name
};

// Every HPatches sequence has images 1..6
constexpr int kImagesPerSequence = 6;

// Conversions between Split / SeqType and their names ("train", "illumination", ...)
Split parseSplit(const std::string& s);
std::string splitName(Split s);
std::string typeName(SeqType t);

// Reads "<sequence> <train|validation|test>" lines. Throws std::runtime_error on bad input
// (unknown split, name without i_/v_ prefix, duplicate sequence, empty file)
std::vector<Sequence> readSplitFile(const std::string& path);

// The sequences that belong to one part of the split, in file order
std::vector<Sequence> sequencesIn(const std::vector<Sequence>& all, Split s);

// Path of image I_idx (idx in 1..6) of a sequence: <root>/<seq>/<idx>.ppm
std::string hpatchesImagePath(const std::string& root, const std::string& seq, int idx);

// Throws if any sequence directory or image 1..6.ppm is missing
void checkHPatches(const std::string& root, const std::vector<Sequence>& seqs);

// File names im<N>.jpg in the directory, sorted by increasing N (numeric order, so
// im2.jpg comes before im10.jpg). The first D of these are the distractors
std::vector<std::string> listMirflickr(const std::string& dir);

// Full path of a MIRFlickr image: <dir>/<file>
std::string mirflickrImagePath(const std::string& dir, const std::string& file);
