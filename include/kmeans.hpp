// Own implementation of k-means (Lloyd's algorithm with k-means++ initialisation)
// Used for the SIFT vocabulary (centers c_1..c_K) and later for the IVFFlat / IVFPQ indexes

#pragma once

#include <cstddef>
#include <vector>

struct KMeansResult {
    std::vector<float> centroids;  // k * dim, row-major (centroid j starts at j * dim)
    std::vector<int> assignment;   // cluster of each input point
    int iterations = 0;            // Lloyd iterations performed
};

// Clusters n points of dimension dim (data is n * dim, row-major) into k clusters
// Stops after maxIter iterations, or when at most tol * n points change cluster in one
// iteration. Deterministic for a given seed, independent of the number of threads
// Throws std::runtime_error if n < k
KMeansResult kmeans(const std::vector<float>& data, size_t n, int dim, int k, unsigned seed,
                    int maxIter = 100, double tol = 1e-3);

// Index of the centroid nearest to x (Euclidean). Its squared distance in *sqDist if given
int nearestCentroid(const float* x, const float* centroids, int k, int dim,
                    float* sqDist = nullptr);

// Indices of the `count` centroids nearest to x, closest first (count is capped at k)
// Used by IVFFlat / IVFPQ to choose the nprobe clusters to search
std::vector<int> nearestCentroids(const float* x, const float* centroids, int k, int dim,
                                  int count);

// Seed for one separate random step, derived from the -seed value and a fixed salt per
// step, so the steps do not share random numbers but all depend only on -seed
unsigned deriveSeed(unsigned seed, unsigned salt);
