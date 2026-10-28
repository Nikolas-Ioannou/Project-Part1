// Builds (or loads from the cache) everything that comes before the search:
// the vocabulary and the vectors x_I of the HPatches and MIRFlickr images

// SIFT extraction is by far the slowest step, so the results are cached in the cache
// directory. The file names contain K, S, the seed and a hash of the split file, so a
// change in any of them builds a new cache instead of reusing a wrong one

#pragma once

#include <string>
#include <vector>

#include "args.hpp"
#include "bovw.hpp"
#include "dataset.hpp"
#include "vector_set.hpp"

struct FeatureData {
    Vocabulary vocab;
    // All 6 images of every validation and test sequence (ids "v_bird/1" .. "v_bird/6")
    // Training images are only used for the vocabulary and are not stored
    VectorSet hpatches;
    // MIRFlickr images in increasing number order, as many as needed to have at least D
    // images with SIFT descriptors. Images with 0 descriptors are kept here (marked by
    // numDescriptors == 0) and skipped when the database is built
    VectorSet mirflickr;
};

// Loads the cached data or computes what is missing. The MIRFlickr cache only grows:
// a later run with a larger -D extends it instead of starting over
// If fewer than D MIRFlickr images have descriptors, a warning is printed and all usable
// images are used (this happens for D = 25000: 15 images have no SIFT keypoints)
FeatureData loadOrBuildFeatures(const Args& args, const std::vector<Sequence>& seqs,
                                const std::vector<std::string>& mirFiles);

// Number of MIRFlickr vectors in the set with at least one SIFT descriptor
size_t countValid(const VectorSet& vs);
