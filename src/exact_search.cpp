// Exact search

#include "exact_search.hpp"

#include "distance.hpp"
#include "topk.hpp"

using namespace std;

void ExactSearch::build(const VectorSet& db) {
    // No index: only the database is kept
    db_ = &db;
}

vector<int> ExactSearch::search(const float* q, int k) const {
    TopK top(k);
    for (size_t i = 0; i < db_->size(); ++i)
        top.push(squaredL2(q, db_->row(i), db_->dim), static_cast<int>(i));
    return top.sorted();
}
