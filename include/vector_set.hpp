// A collection of image vectors x_I stored as one contiguous row-major matrix, plus the
// binary file format used to cache it on disk

#pragma once

#include <string>
#include <vector>

struct VectorSet {
    int dim = 0;                      // vector dimension (K)
    std::vector<std::string> ids;     // image id: "v_bird/2" (HPatches) or "im123.jpg" (MIRFlickr)
    std::vector<int> numDescriptors;  // n_I, number of SIFT descriptors of each image
    std::vector<float> data;          // size() * dim values, row i is the vector of ids[i]

    size_t size() const { return ids.size(); }
    const float* row(size_t i) const { return data.data() + i * dim; }

    // Appends one image vector (x.size() must equal dim)
    void add(const std::string& id, int nDesc, const std::vector<float>& x);
};

// Writes the set to a binary file (written to <path>.tmp and renamed, so an interrupted
// run never leaves a half-written cache). Throws std::runtime_error on I/O errors
void saveVectorSet(const std::string& path, const VectorSet& vs);

// Reads a set written by saveVectorSet. Returns false if the file does not exist or is
// not a valid vector file of dimension expectedDim (the caller then rebuilds it)
bool loadVectorSet(const std::string& path, int expectedDim, VectorSet& vs);
