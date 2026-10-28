// Random projections h_i(x) = floor((v_i · x + t_i) / w)

#include "projection.hpp"

#include <cmath>

#include "distance.hpp"

using namespace std;

ProjectionFamily::ProjectionFamily(int count, int dim, double w, mt19937& rng)
    : count_(count), dim_(dim), w_(w), v_(static_cast<size_t>(count) * dim), t_(count) {
    normal_distribution<float> normal(0.f, 1.f);
    uniform_real_distribution<double> uniform(0.0, w);
    for (float& x : v_) x = normal(rng);
    for (double& x : t_) x = uniform(rng);
}

void ProjectionFamily::hash(const float* x, long long* out) const {
    for (int i = 0; i < count_; ++i) {
        const double p = dot(&v_[static_cast<size_t>(i) * dim_], x, dim_);
        // floor (not truncation), so negative projections fall into the correct cell
        out[i] = static_cast<long long>(floor((p + t_[i]) / w_));
    }
}
