// Inverted File Flat (-ivfflat)
//  - build: k-means on the n database vectors x_I gives kclusters centroids (a different
//    k-means from the SIFT vocabulary). Every vector is stored in the inverted list of its
//    nearest centroid. Default kclusters = ceil(sqrt(n))
//  - search: the nprobe centroids nearest to q are found and only the vectors in their
//    lists are compared with q, using the exact Euclidean distance
// Larger nprobe searches more lists (slower, closer to exact search)

#pragma once

#include <vector>

#include "args.hpp"
#include "searcher.hpp"

class IVFFlat : public Searcher {
public:
    IVFFlat(const IVFFlatParams& params, unsigned seed);

    void build(const VectorSet& db) override;
    std::vector<int> search(const float* q, int k) const override;
    std::string stats() const override;

private:
    IVFFlatParams params_;
    unsigned seed_;
    const VectorSet* db_ = nullptr;
    int kclusters_ = 0;                  // number of clusters actually used
    std::vector<float> centroids_;       // kclusters * dim, row-major
    std::vector<std::vector<int>> lists_;  // lists_[c]: database vectors of cluster c

    // Statistics over the queries run so far (for the experimental study)
    mutable long long totalCandidates_ = 0;
    mutable long long queries_ = 0;
};

// Number of clusters for IVFFlat / IVFPQ: the -kclusters value, or ceil(sqrt(n)) if it was
// not given, capped at n (k-means needs at least as many points as clusters)
int resolveClusters(int requested, size_t n);
