// Inverted File with Product Quantization (-ivfpq)
//  - build:
//    1. coarse k-means on the n database vectors, as in IVFFlat (kclusters centroids)
//    2. residual of every vector: r = x - c, where c is the centroid of its cluster
//    3. the K coordinates are split into M sub-vectors of K / M coordinates. For every
//       sub-space m, k-means on the residual sub-vectors of all n vectors gives 2^nbits
//       sub-centroids
//    4. every vector is stored in its cluster's list as M codes: code m is the index of
//       the sub-centroid nearest to its m-th residual sub-vector
//  - search (asymmetric distance computation): for each of the nprobe nearest clusters,
//    the query residual r_q = q - c is computed once and a table
//    T[m][j] = ||r_q,m - subcentroid_m,j||^2 is filled. The approximate squared distance to
//    a stored vector is then sum over m of T[m][code_m], i.e. M table lookups instead of a
//    K-dimensional distance. Results are ranked by this approximate distance
// Each vector takes M * nbits bits instead of K floats

#pragma once

#include <cstdint>
#include <vector>

#include "args.hpp"
#include "searcher.hpp"

class IVFPQ : public Searcher {
public:
    IVFPQ(const IVFPQParams& params, unsigned seed);

    void build(const VectorSet& db) override;
    std::vector<int> search(const float* q, int k) const override;
    std::string stats() const override;

private:
    IVFPQParams params_;
    unsigned seed_;
    int dim_ = 0;        // K
    int dsub_ = 0;       // K / M, coordinates per sub-vector
    int ksub_ = 0;       // sub-centroids per sub-space: 2^nbits (capped at n)
    int kclusters_ = 0;  // coarse clusters actually used
    std::vector<float> centroids_;  // kclusters * K, row-major
    // subCentroids_[m]: ksub * dsub values, the sub-centroids of sub-space m
    std::vector<std::vector<float>> subCentroids_;
    // Inverted lists: ids_[c] are the database indices of cluster c, codes_[c] their codes
    // (M per vector, stored one vector after the other)
    std::vector<std::vector<int>> ids_;
    std::vector<std::vector<uint16_t>> codes_;

    // Statistics over the queries run so far (for the experimental study)
    mutable long long totalCandidates_ = 0;
    mutable long long queries_ = 0;
};
