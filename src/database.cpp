// Building the queries and the database of a query set

#include "database.hpp"

#include <stdexcept>
#include <unordered_map>

using namespace std;

SearchData buildSearchData(const FeatureData& fd, const vector<Sequence>& seqs, Split set,
                           int D) {
    const VectorSet& hp = fd.hpatches;

    // Row of every HPatches image id ("v_bird/3") in the computed vectors
    unordered_map<string, size_t> rowOf;
    for (size_t i = 0; i < hp.size(); ++i) rowOf[hp.ids[i]] = i;
    auto rowFor = [&](const string& id) {
        auto it = rowOf.find(id);
        if (it == rowOf.end()) throw runtime_error("no vector computed for " + id);
        return it->second;
    };
    auto rowVector = [](const VectorSet& vs, size_t r) {
        return vector<float>(vs.row(r), vs.row(r) + vs.dim);
    };

    SearchData sd;
    sd.queries.dim = hp.dim;
    sd.db.dim = hp.dim;

    // Queries (I1) and their relevant images (I2..I6), in split file order
    for (const auto& s : sequencesIn(seqs, set)) {
        const int q = static_cast<int>(sd.queries.size());
        const size_t r1 = rowFor(s.name + "/1");
        sd.queries.add(s.name, hp.numDescriptors[r1], rowVector(hp, r1));
        sd.queryType.push_back(s.type);
        for (int i = 2; i <= kImagesPerSequence; ++i) {
            const string id = s.name + "/" + to_string(i);
            const size_t r = rowFor(id);
            sd.db.add(id, hp.numDescriptors[r], rowVector(hp, r));
            sd.dbQuery.push_back(q);
        }
    }
    sd.numHPatches = sd.db.size();

    // Distractors: the first D MIRFlickr images with at least one SIFT descriptor
    // (images without descriptors are skipped, so the next ones take their place)
    const VectorSet& mir = fd.mirflickr;
    for (size_t i = 0; i < mir.size() && sd.numMirflickr < static_cast<size_t>(D); ++i) {
        if (mir.numDescriptors[i] == 0) continue;
        sd.db.add(mir.ids[i], mir.numDescriptors[i], rowVector(mir, i));
        sd.dbQuery.push_back(-1);
        ++sd.numMirflickr;
    }
    return sd;
}
