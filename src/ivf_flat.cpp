// Inverted File Flat

#include "ivf_flat.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "distance.hpp"
#include "kmeans.hpp"
#include "topk.hpp"

using namespace std;

int resolveClusters(int requested, size_t n) {
    int k = requested > 0 ? requested : static_cast<int>(ceil(sqrt(static_cast<double>(n))));
    return max(1, min(k, static_cast<int>(n)));
}

IVFFlat::IVFFlat(const IVFFlatParams& params, unsigned seed) : params_(params), seed_(seed) {}

void IVFFlat::build(const VectorSet& db) {
    db_ = &db;
    kclusters_ = resolveClusters(params_.kclusters, db.size());

    // k-means on all n database vectors (salt 3: separate random numbers from other steps)
    KMeansResult km = kmeans(db.data, db.size(), db.dim, kclusters_, deriveSeed(seed_, 3));
    centroids_ = move(km.centroids);

    // Inverted lists: each vector goes to the list of the cluster k-means assigned it to,
    // which is its nearest centroid
    lists_.assign(kclusters_, {});
    for (size_t i = 0; i < db.size(); ++i) lists_[km.assignment[i]].push_back(static_cast<int>(i));
}

vector<int> IVFFlat::search(const float* q, int k) const {
    const vector<int> probe =
        nearestCentroids(q, centroids_.data(), kclusters_, db_->dim, params_.nprobe);

    TopK top(k);
    long long candidates = 0;
    for (int c : probe)
        for (int i : lists_[c]) {
            top.push(squaredL2(q, db_->row(i), db_->dim), i);
            ++candidates;
        }

    totalCandidates_ += candidates;
    ++queries_;
    return top.sorted();
}

string IVFFlat::stats() const {
    size_t largest = 0;
    for (const auto& l : lists_) largest = max(largest, l.size());
    ostringstream os;
    os << fixed << setprecision(1) << "kclusters: " << kclusters_
       << ", avg candidates per query: "
       << (queries_ ? static_cast<double>(totalCandidates_) / queries_ : 0.0)
       << ", largest list: " << largest;
    return os.str();
}
