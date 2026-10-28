// VectorSet and its binary file format 
// File layout (native byte order):
//   char[4] "BVEC", int32 version, int32 dim, uint64 count,
//   count x { uint32 idLength, char[idLength] id, int32 numDescriptors },
//   count * dim float32 vector values.

#include "vector_set.hpp"
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
using namespace std;

namespace {
constexpr char kMagic[4] = {'B', 'V', 'E', 'C'};

constexpr int32_t kVersion = 1;

template <typename T>
void writeRaw(ofstream& out, const T& v) {
    out.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

template <typename T>
bool readRaw(ifstream& in, T& v) {
    return static_cast<bool>(in.read(reinterpret_cast<char*>(&v), sizeof(T)));
}

}  

void VectorSet::add(const string& id, int nDesc, const vector<float>& x) {
    if (static_cast<int>(x.size()) != dim)
        throw runtime_error("VectorSet::add: wrong vector dimension for " + id);
    ids.push_back(id);
    numDescriptors.push_back(nDesc);
    data.insert(data.end(), x.begin(), x.end());
}

void saveVectorSet(const string& path, const VectorSet& vs) {
    const string tmp = path + ".tmp";
    {
        ofstream out(tmp, ios::binary);
        if (!out) throw runtime_error("cannot write " + tmp);
        out.write(kMagic, 4);
        writeRaw(out, kVersion);
        writeRaw(out, static_cast<int32_t>(vs.dim));
        writeRaw(out, static_cast<uint64_t>(vs.size()));
        for (size_t i = 0; i < vs.size(); ++i) {
            writeRaw(out, static_cast<uint32_t>(vs.ids[i].size()));
            out.write(vs.ids[i].data(), vs.ids[i].size());
            writeRaw(out, static_cast<int32_t>(vs.numDescriptors[i]));
        }
        out.write(reinterpret_cast<const char*>(vs.data.data()), vs.data.size() * sizeof(float));
        if (!out) throw runtime_error("error while writing " + tmp);
    }
    filesystem::rename(tmp, path);
}

bool loadVectorSet(const string& path, int expectedDim, VectorSet& vs) {
    ifstream in(path, ios::binary);
    if (!in) return false;

    char magic[4];
    int32_t version = 0, dim = 0;
    uint64_t count = 0;
    if (!in.read(magic, 4) || memcmp(magic, kMagic, 4) != 0) return false;
    if (!readRaw(in, version) || version != kVersion) return false;
    if (!readRaw(in, dim) || dim != expectedDim) return false;
    if (!readRaw(in, count)) return false;
    // A damaged header could claim billions of vectors. Reject counts larger than what the
    // file can hold before allocating memory for them
    const auto fileSize = filesystem::file_size(path);
    if (count > fileSize / (static_cast<uint64_t>(dim) * sizeof(float) + 1)) return false;

    VectorSet out;
    out.dim = dim;
    out.ids.resize(count);
    out.numDescriptors.resize(count);
    for (uint64_t i = 0; i < count; ++i) {
        uint32_t len = 0;
        int32_t nDesc = 0;
        if (!readRaw(in, len) || len > 4096) return false;
        out.ids[i].resize(len);
        if (!in.read(out.ids[i].data(), len) || !readRaw(in, nDesc)) return false;
        out.numDescriptors[i] = nDesc;
    }
    out.data.resize(count * dim);
    if (!in.read(reinterpret_cast<char*>(out.data.data()), out.data.size() * sizeof(float)))
        return false;

    vs = move(out);
    return true;
}
