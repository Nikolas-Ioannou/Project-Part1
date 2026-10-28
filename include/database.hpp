// Search database and queries for one query set (section II.B)
//  - queries: image I1 of every sequence of the set (validation or test)
//  - database: images I2..I6 of those sequences (5 relevant images per query) followed by
//    the first D MIRFlickr images that have SIFT descriptors (distractors)
// Training images are never in the database. With 23 sequences per set the database has
// 115 + D images

#pragma once

#include <string>
#include <vector>

#include "dataset.hpp"
#include "features.hpp"
#include "vector_set.hpp"

// Number of relevant images per query (I2..I6)
constexpr int kRelevantPerQuery = kImagesPerSequence - 1;

struct SearchData {
    VectorSet queries;              // ids are the sequence names, e.g. "v_bird"
    std::vector<SeqType> queryType; // illumination or viewpoint, per query
    VectorSet db;                   // ids "v_bird/2" (HPatches) or "im123.jpg" (MIRFlickr)
    // Per database image: the index of the query it is relevant to, or -1 for MIRFlickr
    std::vector<int> dbQuery;
    size_t numHPatches = 0;         // database images from HPatches (115)
    size_t numMirflickr = 0;        // database images from MIRFlickr (D, or fewer if not enough)
};

// Builds the queries and the database of the given set from the computed vectors
SearchData buildSearchData(const FeatureData& fd, const std::vector<Sequence>& seqs, Split set,
                           int D);
