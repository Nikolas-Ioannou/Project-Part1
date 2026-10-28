// Exact (exhaustive) search: the query is compared with every database vector
// Gives the true nearest neighbours, so it is the quality reference for the other methods

#pragma once

#include "searcher.hpp"

class ExactSearch : public Searcher {
public:
    void build(const VectorSet& db) override;
    std::vector<int> search(const float* q, int k) const override;

private:
    const VectorSet* db_ = nullptr;
};
