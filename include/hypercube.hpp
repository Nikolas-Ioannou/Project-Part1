// Random projection on the hypercube (-hypercube)
//  - d' = kproj random projections h_1..h_d' (as in LSH). Each value h_i(x) is mapped to a
//    random bit f_i(h_i(x)) in {0, 1}, so every vector falls on a vertex of the
//    d'-dimensional hypercube {0, 1}^d'
//  - build: every database vector is stored at its vertex
//  - search: the vertices are visited in increasing Hamming distance from the query's
//    vertex (the query's vertex first, then those differing in 1 bit, then 2 bits, ...)
//    The search stops after `probes` vertices or after M candidate vectors, whichever comes
//    first. The candidates are ranked by their exact distance to q

#pragma once

#include <unordered_map>
#include <vector>

#include "args.hpp"
#include "projection.hpp"
#include "searcher.hpp"

class Hypercube : public Searcher {
public:
    Hypercube(const HypercubeParams& params, unsigned seed);
    void build(const VectorSet& db) override;
    std::vector<int> search(const float* q, int k) const override;
    std::string stats() const override;

private:
    // Vertex of x: bit i is f_i(h_i(x))
    unsigned vertex(const float* x) const;
    HypercubeParams params_;
    unsigned seed_;
    const VectorSet* db_ = nullptr;
    std::vector<ProjectionFamily> h_;  // a single family of kproj functions
    // vertex -> indices of the database vectors on it (only non-empty vertices are stored)
    std::unordered_map<unsigned, std::vector<int>> vertices_;
    // Statistics over the queries run so far (for the experimental study)
    mutable long long totalCandidates_ = 0;
    mutable long long totalProbes_ = 0;
    mutable long long queries_ = 0;
};
