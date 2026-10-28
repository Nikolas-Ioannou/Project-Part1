// Loading / building the vocabulary and the image vectors with caching 

#include "features.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace fs = filesystem;

namespace {

// MIRFlickr vectors are computed in chunks of this many images and the cache is saved
// after each chunk, so an interrupted run loses at most one chunk of work
constexpr size_t kMirChunk = 1000;

// FNV-1a hash of the split (sequence names and their parts), so a different split file
// never reuses a cache built for another one
uint32_t splitHash(const vector<Sequence>& seqs) {
    uint32_t h = 2166136261u;
    auto mix = [&h](const string& s) {
        for (unsigned char c : s) {
            h ^= c;
            h *= 16777619u;
        }
        h ^= '\n';
        h *= 16777619u;
    };
    for (const auto& s : seqs) mix(s.name + " " + splitName(s.split));
    return h;
}

// Common part of the cache file names, e.g. "K256_S500_seed1_1a2b3c4d"
string cacheTag(const Args& a, const vector<Sequence>& seqs) {
    ostringstream os;
    os << "K" << a.vocab << "_S" << a.S << "_seed" << a.seed << "_" << hex
       << setw(8) << setfill('0') << splitHash(seqs);
    return os.str();
}

Vocabulary loadOrTrainVocabulary(const Args& a, const vector<Sequence>& seqs,
                                 const string& path) {
    Vocabulary vocab;
    if (loadVocabulary(path, a.vocab, vocab)) {
        cerr << "Vocabulary: loaded from " << path << "\n";
        return vocab;
    }
    // The centers are computed only from the training images (all 6 of each sequence)
    vector<string> train;
    for (const auto& s : sequencesIn(seqs, Split::Train))
        for (int i = 1; i <= kImagesPerSequence; ++i)
            train.push_back(hpatchesImagePath(a.hpatchesDir, s.name, i));
    cerr << "Vocabulary: training on " << train.size() << " images\n";
    vocab = trainVocabulary(train, a.vocab, a.S, a.seed);
    saveVocabulary(path, vocab);
    return vocab;
}

VectorSet loadOrBuildHPatches(const Args& a, const vector<Sequence>& seqs,
                              const Vocabulary& vocab, const string& path) {
    VectorSet vs;
    if (loadVectorSet(path, a.vocab, vs)) {
        cerr << "HPatches vectors: loaded from " << path << "\n";
        return vs;
    }
    // Validation and test sequences, both query (image 1) and database images (2..6)
    vector<string> paths, ids;
    for (const auto& s : seqs) {
        if (s.split == Split::Train) continue;
        for (int i = 1; i <= kImagesPerSequence; ++i) {
            paths.push_back(hpatchesImagePath(a.hpatchesDir, s.name, i));
            ids.push_back(s.name + "/" + to_string(i));
        }
    }
    cerr << "HPatches vectors: computing " << paths.size() << " images\n";
    vs = computeVectors(paths, ids, vocab, false, "HPatches");
    saveVectorSet(path, vs);
    return vs;
}

VectorSet loadOrExtendMirflickr(const Args& a, const vector<string>& files,
                                const Vocabulary& vocab, const string& path) {
    VectorSet vs;
    if (loadVectorSet(path, a.vocab, vs))
        cerr << "MIRFlickr vectors: " << vs.size() << " loaded from " << path << "\n";
    else
        vs.dim = a.vocab;
    // The cache must hold a prefix of the sorted file list; if it does not (e.g. images were
    // added or removed), start again
    for (size_t i = 0; i < vs.size(); ++i)
        if (i >= files.size() || vs.ids[i] != files[i]) {
            cerr << "MIRFlickr cache does not match the image list, rebuilding\n";
            vs = VectorSet();
            vs.dim = a.vocab;
            break;
        }

    // Images without descriptors are replaced by the next ones, so keep going until there
    // are D valid images or no images left
    size_t valid = countValid(vs);
    const size_t D = static_cast<size_t>(a.D);
    while (valid < D && vs.size() < files.size()) {
        const size_t start = vs.size();
        const size_t count = min({files.size() - start, D - valid, kMirChunk});
        vector<string> paths, ids;
        for (size_t i = start; i < start + count; ++i) {
            paths.push_back(mirflickrImagePath(a.mirflickrDir, files[i]));
            ids.push_back(files[i]);
        }
        const string label = "MIRFlickr " + to_string(start + 1) + "-" +
                                  to_string(start + count);
        VectorSet chunk = computeVectors(paths, ids, vocab, true, label);
        for (size_t i = 0; i < chunk.size(); ++i)
            vs.add(chunk.ids[i], chunk.numDescriptors[i],
                   vector<float>(chunk.row(i), chunk.row(i) + chunk.dim));
        valid = countValid(vs);
        saveVectorSet(path, vs);
    }

    // MIRFlickr-25K has 15 images without any SIFT keypoint, so -D 25000 cannot be met
    // exactly. All usable images are taken instead (24985 for the full set)
    if (valid < D)
        cerr << "warning: only " << valid << " MIRFlickr images have SIFT descriptors, "
                  << "using all of them instead of -D " << D << "\n";
    return vs;
}

}  

size_t countValid(const VectorSet& vs) {
    return static_cast<size_t>(count_if(vs.numDescriptors.begin(), vs.numDescriptors.end(),
                                             [](int n) { return n > 0; }));
}

FeatureData loadOrBuildFeatures(const Args& args, const vector<Sequence>& seqs,
                                const vector<string>& mirFiles) {
    fs::create_directories(args.cacheDir);
    const string tag = cacheTag(args, seqs);
    const fs::path dir(args.cacheDir);
    FeatureData fd;
    fd.vocab = loadOrTrainVocabulary(args, seqs, (dir / ("vocab_" + tag + ".bin")).string());
    fd.hpatches = loadOrBuildHPatches(args, seqs, fd.vocab,
                                      (dir / ("hpatches_" + tag + ".bin")).string());
    fd.mirflickr = loadOrExtendMirflickr(args, mirFiles, fd.vocab,
                                         (dir / ("mirflickr_" + tag + ".bin")).string());
    return fd;
}
