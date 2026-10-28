// Locality Sensitive Hashing

#include "lsh.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "distance.hpp"
#include "topk.hpp"

using namespace std;

LSH::LSH(const LSHParams& params, unsigned seed) : params_(params), seed_(seed) {}

unsigned long long LSH::key(int l, const float* x) const {
    vector<long long> h(params_.k);
    g_[l].hash(x, h.data());
    // Combine the k values in order, so (1, 2) and (2, 1) give different keys
    // A 64-bit key makes accidental collisions of different g values negligible
    unsigned long long k = 0;
    for (long long v : h) k = mix64(k ^ static_cast<unsigned long long>(v));
    return k;
}

void LSH::build(const VectorSet& db) {
    db_ = &db;
    // Separate generator for LSH (salt 1), so its projections do not depend on other
    // random steps of the program
    seed_seq seq{seed_, 1u};
    mt19937 rng(seq);
    g_.clear();
    for (int l = 0; l < params_.L; ++l) g_.emplace_back(params_.k, db.dim, params_.w, rng);

    tables_.assign(params_.L, {});
    for (int l = 0; l < params_.L; ++l)
        for (size_t i = 0; i < db.size(); ++i)
            tables_[l][key(l, db.row(i))].push_back(static_cast<int>(i));
}

vector<int> LSH::search(const float* q, int k) const {
    // Union of the query's buckets over all tables. A vector can be in the query's bucket
    // in several tables, so duplicates are removed before computing distances
    vector<int> cand;
    for (int l = 0; l < params_.L; ++l) {
        auto it = tables_[l].find(key(l, q));
        if (it != tables_[l].end()) cand.insert(cand.end(), it->second.begin(), it->second.end());
    }
    sort(cand.begin(), cand.end());
    cand.erase(unique(cand.begin(), cand.end()), cand.end());

    totalCandidates_ += static_cast<long long>(cand.size());
    ++queries_;

    TopK top(k);
    for (int i : cand) top.push(squaredL2(q, db_->row(i), db_->dim), i);
    return top.sorted();
}

string LSH::stats() const {
    size_t buckets = 0;
    for (const auto& t : tables_) buckets += t.size();
    ostringstream os;
    os << fixed << setprecision(1) << "avg candidates per query: "
       << (queries_ ? static_cast<double>(totalCandidates_) / queries_ : 0.0)
       << ", avg buckets per table: " << static_cast<double>(buckets) / max(1, params_.L);
    return os.str();
}
