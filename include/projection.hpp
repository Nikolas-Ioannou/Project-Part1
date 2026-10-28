// Random projections used by LSH and Hypercube:
//   h_i(x) = floor((v_i · x + t_i) / w)
// where v_i has independent N(0, 1) coordinates and t_i is uniform in [0, w)
// Points that are close in Euclidean distance get the same h_i value with high probability

#pragma once

#include <random>
#include <vector>

class ProjectionFamily {
public:
    // count independent functions h_0..h_{count-1} on vectors of dimension dim
    ProjectionFamily(int count, int dim, double w, std::mt19937& rng);
    // Writes h_0(x)..h_{count-1}(x) to out (out must hold count values)
    void hash(const float* x, long long* out) const;
    int count() const { return count_; }

private:
    int count_;
    int dim_;
    double w_;
    std::vector<float> v_;   // count * dim projection directions, row-major
    std::vector<double> t_;  // count offsets in [0, w)
};

// 64-bit mixing function (splitmix64 finaliser): spreads the bits of x so that similar
// inputs give unrelated outputs. Used to combine several h values into one key
inline unsigned long long mix64(unsigned long long x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
