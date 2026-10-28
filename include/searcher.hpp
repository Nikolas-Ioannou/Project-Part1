// Common interface of the five search methods (Exact, LSH, Hypercube, IVFFlat, IVFPQ)
// Every method first builds its index over the database (not timed), then answers
// queries (timed: from the moment x_Q is available until the 10 results are returned)

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "args.hpp"
#include "vector_set.hpp"

class Searcher {
public:
    virtual ~Searcher() = default;

    // Builds the index over the database vectors. db must outlive the searcher
    virtual void build(const VectorSet& db) = 0;

    // Indices (rows of db) of up to k database vectors nearest to q, closest first
    // Approximate methods may return fewer than k if they find fewer candidates
    virtual std::vector<int> search(const float* q, int k) const = 0;

    // Method-specific statistics for the experimental study (e.g. average number of
    // candidates per query). Empty for methods without any
    virtual std::string stats() const { return ""; }
};

// Creates the searcher selected on the command line, with its parameters
std::unique_ptr<Searcher> makeSearcher(const Args& args);
