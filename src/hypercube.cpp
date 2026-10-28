// Random projection on the hypercube

#include "hypercube.hpp"

#include <iomanip>
#include <random>
#include <sstream>

#include "distance.hpp"
#include "topk.hpp"

using namespace std;

Hypercube::Hypercube(const HypercubeParams& params, unsigned seed)
    : params_(params), seed_(seed) {}

unsigned Hypercube::vertex(const float* x) const {
    vector<long long> h(params_.kproj);
    h_[0].hash(x, h.data());
    unsigned v = 0;
    for (int i = 0; i < params_.kproj; ++i) {
        // f_i: a fixed random map from the integers to {0, 1}. Computed from a hash of
        // (seed, i, h) instead of a stored table, so it is defined for every value h
        // (also ones first seen at query time) and does not change between calls
        const unsigned long long r =
            mix64(mix64(mix64(seed_) ^ static_cast<unsigned long long>(i)) ^
                  static_cast<unsigned long long>(h[i]));
        v |= static_cast<unsigned>(r & 1ULL) << i;
    }
    return v;
}

void Hypercube::build(const VectorSet& db) {
    db_ = &db;
    // Separate generator for the hypercube (salt 2)
    seed_seq seq{seed_, 2u};
    mt19937 rng(seq);
    h_.clear();
    h_.emplace_back(params_.kproj, db.dim, params_.w, rng);
    vertices_.clear();
    for (size_t i = 0; i < db.size(); ++i) vertices_[vertex(db.row(i))].push_back(static_cast<int>(i));
}

vector<int> Hypercube::search(const float* q, int k) const {
    const int d = params_.kproj;
    const unsigned qv = vertex(q);
    TopK top(k);
    int probes = 0;
    int checked = 0;
    // Visit vertices at Hamming distance 0, 1, 2, ... from qv. The vertices at distance r
    // are qv XOR mask for every d-bit mask with r bits set. The masks are enumerated in
    // increasing numeric order (Gosper's hack: next larger number with the same bit count)
    for (int r = 0; r <= d && probes < params_.probes && checked < params_.M; ++r) {
        const unsigned long long limit = 1ULL << d;
        unsigned long long mask = (1ULL << r) - 1;  // smallest mask with r bits set
        while (mask < limit && probes < params_.probes && checked < params_.M) {
            ++probes;
            auto it = vertices_.find(qv ^ static_cast<unsigned>(mask));
            if (it != vertices_.end())
                for (int i : it->second) {
                    if (checked >= params_.M) break;
                    ++checked;
                    top.push(squaredL2(q, db_->row(i), db_->dim), i);
                }
            if (mask == 0) break;  // r = 0: the only mask is 0
            // Gosper's hack
            const unsigned long long c = mask & (~mask + 1);
            const unsigned long long nr = mask + c;
            mask = (((nr ^ mask) >> 2) / c) | nr;
        }
    }
    totalCandidates_ += checked;
    totalProbes_ += probes;
    ++queries_;
    return top.sorted();
}

string Hypercube::stats() const {
    const double nq = queries_ ? static_cast<double>(queries_) : 1.0;
    ostringstream os;
    os << fixed << setprecision(1) << "avg candidates per query: " << totalCandidates_ / nq
       << ", avg vertices probed: " << totalProbes_ / nq
       << ", non-empty vertices: " << vertices_.size();
    return os.str();
}
