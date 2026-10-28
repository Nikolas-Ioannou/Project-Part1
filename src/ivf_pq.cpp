// Inverted File with Product Quantization

#include "ivf_pq.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "distance.hpp"
#include "ivf_flat.hpp"
#include "kmeans.hpp"
#include "parallel.hpp"
#include "topk.hpp"

using namespace std;

namespace {

// Lloyd iterations for the sub-space k-means. The sub-spaces are small (K / M coordinates),
// so they converge quickly
constexpr int kSubIterations = 25;

}  // namespace

IVFPQ::IVFPQ(const IVFPQParams& params, unsigned seed) : params_(params), seed_(seed) {}

void IVFPQ::build(const VectorSet& db) {
    const size_t n = db.size();
    const int M = params_.M;
    dim_ = db.dim;
    dsub_ = dim_ / M;  // -vocab divisible by -M is checked when the options are parsed
    kclusters_ = resolveClusters(params_.kclusters, n);
    // k-means needs at least as many points as clusters
    ksub_ = static_cast<int>(min<size_t>(size_t{1} << params_.nbits, n));

    // 1. Coarse quantizer (salt 4)
    KMeansResult coarse = kmeans(db.data, n, dim_, kclusters_, deriveSeed(seed_, 4));
    centroids_ = move(coarse.centroids);

    // 2. Residuals r = x - c(x)
    vector<float> residual(n * dim_);
    for (size_t i = 0; i < n; ++i) {
        const float* x = db.row(i);
        const float* c = &centroids_[static_cast<size_t>(coarse.assignment[i]) * dim_];
        for (int d = 0; d < dim_; ++d) residual[i * dim_ + d] = x[d] - c[d];
    }

    // 3. One k-means per sub-space on the residual sub-vectors (salt 100 + m)
    // 4. The code of every vector in every sub-space
    subCentroids_.assign(M, {});
    vector<uint16_t> code(n * M);
    for (int m = 0; m < M; ++m) {
        // Copy coordinates [m * dsub, (m + 1) * dsub) of every residual into one matrix
        vector<float> sub(n * dsub_);
        for (size_t i = 0; i < n; ++i)
            copy_n(&residual[i * dim_ + m * dsub_], dsub_, &sub[i * dsub_]);
        KMeansResult km = kmeans(sub, n, dsub_, ksub_, deriveSeed(seed_, 100 + m), kSubIterations);
        for (size_t i = 0; i < n; ++i) code[i * M + m] = static_cast<uint16_t>(km.assignment[i]);
        subCentroids_[m] = move(km.centroids);
    }

    // Inverted lists with the codes
    ids_.assign(kclusters_, {});
    codes_.assign(kclusters_, {});
    for (size_t i = 0; i < n; ++i) {
        const int c = coarse.assignment[i];
        ids_[c].push_back(static_cast<int>(i));
        codes_[c].insert(codes_[c].end(), &code[i * M], &code[i * M] + M);
    }
}

vector<int> IVFPQ::search(const float* q, int k) const {
    const int M = params_.M;
    const vector<int> probe =
        nearestCentroids(q, centroids_.data(), kclusters_, dim_, params_.nprobe);

    TopK top(k);
    vector<float> rq(dim_);
    vector<float> table(static_cast<size_t>(M) * ksub_);  // T[m][j] at m * ksub + j
    long long candidates = 0;

    for (int c : probe) {
        if (ids_[c].empty()) continue;
        // Query residual with respect to this cluster's centroid
        const float* cent = &centroids_[static_cast<size_t>(c) * dim_];
        for (int d = 0; d < dim_; ++d) rq[d] = q[d] - cent[d];

        // Distance table: squared distance of each query sub-vector to each sub-centroid
        for (int m = 0; m < M; ++m)
            for (int j = 0; j < ksub_; ++j)
                table[static_cast<size_t>(m) * ksub_ + j] =
                    squaredL2(&rq[m * dsub_], &subCentroids_[m][static_cast<size_t>(j) * dsub_], dsub_);

        // Approximate distance of every vector in the list: M table lookups
        const vector<int>& ids = ids_[c];
        const uint16_t* codes = codes_[c].data();
        for (size_t v = 0; v < ids.size(); ++v) {
            float d = 0.f;
            for (int m = 0; m < M; ++m) d += table[static_cast<size_t>(m) * ksub_ + codes[v * M + m]];
            top.push(d, ids[v]);
        }
        candidates += static_cast<long long>(ids.size());
    }

    totalCandidates_ += candidates;
    ++queries_;
    return top.sorted();
}

string IVFPQ::stats() const {
    ostringstream os;
    os << fixed << setprecision(1) << "kclusters: " << kclusters_ << ", M: " << params_.M
       << " sub-vectors of " << dsub_ << ", " << ksub_ << " sub-centroids each"
       << ", avg candidates per query: "
       << (queries_ ? static_cast<double>(totalCandidates_) / queries_ : 0.0)
       << ", code size: " << params_.M * params_.nbits << " bits per vector";
    return os.str();
}
