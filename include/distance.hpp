// Euclidean distance, the metric used by every method in the assignment

#pragma once
// Squared Euclidean distance between two vectors of dimension d
// Ranking by squared distance gives the same order as ranking by Euclidean distance,
// so the square root is skipped wherever only comparisons are needed
// The pragma lets the compiler use SIMD instructions for the sum (-fopenmp-simd)
inline float squaredL2(const float* a, const float* b, int d) {
    float s = 0.f;
#pragma omp simd reduction(+ : s)
    for (int i = 0; i < d; ++i) {
        const float t = a[i] - b[i];
        s += t * t;
    }
    return s;
}

// Dot product of two vectors of dimension d (used by the random projections v · x)
inline float dot(const float* a, const float* b, int d) {
    float s = 0.f;
#pragma omp simd reduction(+ : s)
    for (int i = 0; i < d; ++i) s += a[i] * b[i];
    return s;
}
