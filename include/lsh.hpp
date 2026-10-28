// Locality Sensitive Hashing for the Euclidean distance (-lsh)
//  - L hash tables. Table l uses g_l(x) = (h_1(x), ..., h_k(x)) built from k random
//    projections of its own, so two points share a bucket only if all k values agree
//  - build: every database vector is stored in the bucket g_l(x) of every table
//  - search: the candidates are the vectors in the buckets g_1(q), ..., g_L(q). They are
//    ranked by their exact distance to q. If fewer than 10 candidates are found, fewer
//    than 10 results are returned
// Larger k makes buckets smaller (faster, lower recall). Larger L and w find more candidates

#pragma once

#include <random>
#include <unordered_map>
#include <vector>

#include "args.hpp"
#include "projection.hpp"
#include "searcher.hpp"

class LSH : public Searcher {
public:
    LSH(const LSHParams& params, unsigned seed);

    void build(const VectorSet& db) override;
    std::vector<int> search(const float* q, int k) const override;
    std::string stats() const override;

private:
    // Key of the bucket of x in table l: the k values of g_l(x) mixed into 64 bits
    unsigned long long key(int l, const float* x) const;

    LSHParams params_;
    unsigned seed_;
    const VectorSet* db_ = nullptr;
    std::vector<ProjectionFamily> g_;  // one family of k functions per table
    // tables_[l]: bucket key -> indices of the database vectors in that bucket
    std::vector<std::unordered_map<unsigned long long, std::vector<int>>> tables_;

    // Statistics over the queries run so far (for the experimental study)
    mutable long long totalCandidates_ = 0;
    mutable long long queries_ = 0;
};
