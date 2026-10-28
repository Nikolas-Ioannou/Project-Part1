// Lloyd's k-means with k-means++ initialisation

#include "kmeans.hpp"

#include <algorithm>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

#include "distance.hpp"
#include "parallel.hpp"

using namespace std;

namespace {

// Points per work block in parallel loops. Large enough that thread overhead is negligible
constexpr size_t kGrain = 1024;

// k-means++: the first centroid is a uniformly random point. Each next centroid is a point
// chosen with probability proportional to its squared distance from the nearest centroid
// chosen so far. This spreads the initial centroids and makes k-means converge faster
vector<float> initPlusPlus(const vector<float>& data, size_t n, int dim, int k,
                                mt19937& rng) {
    vector<float> C(static_cast<size_t>(k) * dim);
    // minD[i]: squared distance of point i from its nearest centroid chosen so far
    vector<float> minD(n, numeric_limits<float>::max());

    size_t chosen = uniform_int_distribution<size_t>(0, n - 1)(rng);
    copy_n(&data[chosen * dim], dim, &C[0]);

    for (int c = 1; c < k; ++c) {
        // Only the centroid added last can lower minD, so compare against it alone
        const float* last = &C[static_cast<size_t>(c - 1) * dim];
        parallelFor(n, [&](size_t i) {
            const float d = squaredL2(&data[i * dim], last, dim);
            if (d < minD[i]) minD[i] = d;
        }, kGrain);
        // Sample a point with probability minD[i] / sum(minD). Summed sequentially in
        // double so the choice is exactly reproducible
        double total = 0;
        for (size_t i = 0; i < n; ++i) total += minD[i];
        if (total <= 0) {
            // Every point coincides with a centroid (duplicates). Pick any point
            chosen = uniform_int_distribution<size_t>(0, n - 1)(rng);
        } else {
            double r = uniform_real_distribution<double>(0, total)(rng);
            chosen = n - 1;  // fallback for rounding at the very end of the range
            for (size_t i = 0; i < n; ++i) {
                r -= minD[i];
                if (r <= 0) {
                    chosen = i;
                    break;
                }
            }
        }
        copy_n(&data[chosen * dim], dim, &C[static_cast<size_t>(c) * dim]);
    }
    return C;
}

} 

int nearestCentroid(const float* x, const float* centroids, int k, int dim, float* sqDist) {
    int best = 0;
    float bestD = numeric_limits<float>::max();
    for (int j = 0; j < k; ++j) {
        const float d = squaredL2(x, centroids + static_cast<size_t>(j) * dim, dim);
        if (d < bestD) {
            bestD = d;
            best = j;
        }
    }
    if (sqDist) *sqDist = bestD;
    return best;
}

KMeansResult kmeans(const vector<float>& data, size_t n, int dim, int k, unsigned seed,
                    int maxIter, double tol) {
    if (k < 1 || n < static_cast<size_t>(k))
        throw runtime_error("k-means: need at least k=" + to_string(k) +
                                 " points, got " + to_string(n));

    mt19937 rng(seed);
    KMeansResult res;
    res.centroids = initPlusPlus(data, n, dim, k, rng);
    res.assignment.assign(n, -1);
    vector<int> next(n);
    vector<float> dist(n);  // squared distance of each point to its centroid

    for (int iter = 1;; ++iter) {
        res.iterations = iter;
        // Assignment step: each point goes to its nearest centroid (in parallel)
        parallelFor(n, [&](size_t i) {
            next[i] = nearestCentroid(&data[i * dim], res.centroids.data(), k, dim, &dist[i]);
        }, kGrain);

        size_t changed = 0;
        for (size_t i = 0; i < n; ++i) changed += (next[i] != res.assignment[i]);
        res.assignment.swap(next);
        // Stop when the clustering is stable enough. Stopping here, before the update,
        // keeps the assignment consistent with the returned centroids
        if ((iter > 1 && changed <= tol * n) || iter >= maxIter) break;
        // Update step: each centroid becomes the mean of its points. Sums are accumulated
        // sequentially in double, so the result does not depend on thread scheduling
        vector<double> sum(static_cast<size_t>(k) * dim, 0.0);
        vector<size_t> count(k, 0);
        for (size_t i = 0; i < n; ++i) {
            const int c = res.assignment[i];
            ++count[c];
            const float* x = &data[i * dim];
            double* s = &sum[static_cast<size_t>(c) * dim];
            for (int d = 0; d < dim; ++d) s[d] += x[d];
        }
        // Empty cluster: move the point that is currently worst served (farthest from its
        // centroid) into it, so all k centroids stay in use
        for (int c = 0; c < k; ++c) {
            if (count[c] > 0) continue;
            size_t far = 0;
            float farD = -1;
            for (size_t i = 0; i < n; ++i)
                if (count[res.assignment[i]] > 1 && dist[i] > farD) {
                    farD = dist[i];
                    far = i;
                }
            const int old = res.assignment[far];
            const float* x = &data[far * dim];
            for (int d = 0; d < dim; ++d) {
                sum[static_cast<size_t>(old) * dim + d] -= x[d];
                sum[static_cast<size_t>(c) * dim + d] = x[d];
            }
            --count[old];
            count[c] = 1;
            res.assignment[far] = c;
            dist[far] = 0;
        }

        for (int c = 0; c < k; ++c)
            for (int d = 0; d < dim; ++d)
                res.centroids[static_cast<size_t>(c) * dim + d] =
                    static_cast<float>(sum[static_cast<size_t>(c) * dim + d] / count[c]);
    }
    return res;
}
