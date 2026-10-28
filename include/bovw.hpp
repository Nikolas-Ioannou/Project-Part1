
// Bag of visual words representation (sections I and III)
//  1. Vocabulary: up to S random SIFT descriptors are taken from every training image and
//     k-means on their union gives K centers c_1..c_K in R^128
//  2. Image vector: every SIFT descriptor of an image I is assigned to its nearest center
//     with h_j descriptors assigned to c_j and n_I descriptors in total,
//     x_I = (h_1 / n_I, ..., h_K / n_I), so the coordinates are >= 0 and sum to 1

#pragma once

#include <opencv2/core.hpp>
#include <string>
#include <vector>

#include "vector_set.hpp"

struct Vocabulary {
    int K = 0;                   // number of centers
    std::vector<float> centers;  // K * 128, row-major
};

// Builds the vocabulary from the given training images. For image i the descriptors are
// sampled with a generator seeded from (seed, i), so the sample is reproducible and does
// not depend on the order in which threads finish
Vocabulary trainVocabulary(const std::vector<std::string>& trainImages, int K, int S,
                           unsigned seed);

// x_I for one image from all its descriptors. All zeros if the image has no descriptors
std::vector<float> bovwVector(const cv::Mat& descriptors, const Vocabulary& vocab);

// Computes x_I for every image (in parallel). paths[i] is read and stored under ids[i]
// If skipUnreadable is true, an image that cannot be read is stored with 0 descriptors
// (and a warning) instead of stopping the program. `label` is used in progress messages
VectorSet computeVectors(const std::vector<std::string>& paths,
                         const std::vector<std::string>& ids, const Vocabulary& vocab,
                         bool skipUnreadable, const std::string& label);

// Binary cache of the vocabulary. loadVocabulary returns false if the file is missing or
// does not hold K centers
void saveVocabulary(const std::string& path, const Vocabulary& vocab);
bool loadVocabulary(const std::string& path, int K, Vocabulary& vocab);
