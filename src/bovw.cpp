// Vocabulary training and bag of visual words vectors

#include "bovw.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <stdexcept>

#include "kmeans.hpp"
#include "parallel.hpp"
#include "sift_features.hpp"

using namespace std;

namespace {

// Prints "label: done/total" on one line of stderr, overwriting itself. Thread-safe
class Progress {
public:
    Progress(string label, size_t total) : label_(move(label)), total_(total) {}
    void tick() {
        const size_t d = ++done_;
        // Print about 1% every time
        if (d == total_ || d % max<size_t>(1, total_ / 100) == 0) {
            lock_guard<mutex> lock(mutex_);
            cerr << "\r  " << label_ << ": " << d << "/" << total_ << flush;
            if (d == total_) cerr << "\n";
        }
    }

private:
    string label_;
    size_t total_;
    atomic<size_t> done_{0};
    mutex mutex_;
};

double secondsSince(chrono::steady_clock::time_point t0) {
    return chrono::duration<double>(chrono::steady_clock::now() - t0).count();
}

constexpr char kVocabMagic[4] = {'B', 'V', 'O', 'C'};

}  

Vocabulary trainVocabulary(const vector<string>& trainImages, int K, int S,
                           unsigned seed) {
    const auto t0 = chrono::steady_clock::now();
    // samples[i]: the (up to S) descriptors chosen from training image i, row-major
    vector<vector<float>> samples(trainImages.size());
    Progress progress("SIFT on training images", trainImages.size());
    parallelFor(trainImages.size(), [&](size_t i) {
        const cv::Mat desc = extractSift(trainImages[i]);
        const int n = desc.rows;
        // Choose min(n, S) distinct rows at random: partial Fisher-Yates shuffle
        // The generator depends only on (seed, i), not on the thread
        vector<int> idx(n);
        iota(idx.begin(), idx.end(), 0);
        const int take = min(n, S);
        seed_seq seq{seed, static_cast<unsigned>(i)};
        mt19937 rng(seq);
        for (int j = 0; j < take; ++j) {
            const int r = uniform_int_distribution<int>(j, n - 1)(rng);
            swap(idx[j], idx[r]);
        }
        samples[i].resize(static_cast<size_t>(take) * kSiftDim);
        for (int j = 0; j < take; ++j)
            memcpy(&samples[i][static_cast<size_t>(j) * kSiftDim], desc.ptr<float>(idx[j]),
                        kSiftDim * sizeof(float));
        progress.tick();
    });
    // Union of all samples, in image order
    vector<float> all;
    for (const auto& s : samples) all.insert(all.end(), s.begin(), s.end());
    const size_t n = all.size() / kSiftDim;
    cerr << "  k-means on " << n << " descriptors (K=" << K << ")...\n";
    // At most 50 iterations: the vocabulary changes very little after that
    KMeansResult km = kmeans(all, n, kSiftDim, K, seed, 50);
    cerr << "  vocabulary ready: " << km.iterations << " iterations, "
              << secondsSince(t0) << " s\n";
    Vocabulary vocab;
    vocab.K = K;
    vocab.centers = move(km.centroids);
    return vocab;
}

vector<float> bovwVector(const cv::Mat& descriptors, const Vocabulary& vocab) {
    // h_j: number of descriptors whose nearest center is c_j
    vector<float> x(vocab.K, 0.f);
    const int nI = descriptors.rows;
    if (nI == 0) return x;
    for (int r = 0; r < nI; ++r)
        x[nearestCentroid(descriptors.ptr<float>(r), vocab.centers.data(), vocab.K, kSiftDim)] += 1.f;
    // Divide by n_I so the coordinates sum to 1
    for (float& v : x) v /= static_cast<float>(nI);
    return x;
}

VectorSet computeVectors(const vector<string>& paths,
                         const vector<string>& ids, const Vocabulary& vocab,
                         bool skipUnreadable, const string& label) {
    // Each image writes only its own slot, so the threads never share data
    vector<vector<float>> vecs(paths.size());
    vector<int> nDesc(paths.size(), 0);
    Progress progress(label, paths.size());
    mutex warnMutex;
    parallelFor(paths.size(), [&](size_t i) {
        cv::Mat desc;
        try {
            desc = extractSift(paths[i]);
        } catch (const exception& e) {
            if (!skipUnreadable) throw;
            lock_guard<mutex> lock(warnMutex);
            cerr << "\n  warning: " << e.what() << " (treated as 0 descriptors)\n";
        }
        nDesc[i] = desc.rows;
        vecs[i] = bovwVector(desc, vocab);
        progress.tick();
    });
    VectorSet vs;
    vs.dim = vocab.K;
    for (size_t i = 0; i < paths.size(); ++i) vs.add(ids[i], nDesc[i], vecs[i]);
    return vs;
}

void saveVocabulary(const string& path, const Vocabulary& vocab) {
    const string tmp = path + ".tmp";
    {
        ofstream out(tmp, ios::binary);
        if (!out) throw runtime_error("cannot write " + tmp);
        const int32_t K = vocab.K, dim = kSiftDim;
        out.write(kVocabMagic, 4);
        out.write(reinterpret_cast<const char*>(&K), sizeof K);
        out.write(reinterpret_cast<const char*>(&dim), sizeof dim);
        out.write(reinterpret_cast<const char*>(vocab.centers.data()),
                  vocab.centers.size() * sizeof(float));
        if (!out) throw runtime_error("error while writing " + tmp);
    }
    filesystem::rename(tmp, path);
}

bool loadVocabulary(const string& path, int K, Vocabulary& vocab) {
    ifstream in(path, ios::binary);
    if (!in) return false;
    char magic[4];
    int32_t k = 0, dim = 0;
    if (!in.read(magic, 4) || memcmp(magic, kVocabMagic, 4) != 0) return false;
    if (!in.read(reinterpret_cast<char*>(&k), sizeof k) || k != K) return false;
    if (!in.read(reinterpret_cast<char*>(&dim), sizeof dim) || dim != kSiftDim) return false;
    Vocabulary v;
    v.K = K;
    v.centers.resize(static_cast<size_t>(K) * kSiftDim);
    if (!in.read(reinterpret_cast<char*>(v.centers.data()), v.centers.size() * sizeof(float)))
        return false;
    vocab = move(v);
    return true;
}
