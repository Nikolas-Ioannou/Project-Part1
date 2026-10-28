// Entry point of ./search.
//  1. parse the command line, read the split file and check that all images are present
//  2. build (or load from the cache) the vocabulary and the vectors x_I of the
//     validation/test HPatches images and of the first D usable MIRFlickr images
//  3. build the queries and the database of the chosen set (-set), run the chosen method
//     on every query, write the output file and print a summary

#include <opencv2/core.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include "args.hpp"
#include "database.hpp"
#include "dataset.hpp"
#include "experiment.hpp"
#include "features.hpp"
#include "searcher.hpp"

using namespace std;

namespace {

// Prints a table with the number of illumination / viewpoint sequences in each split,
// to compare against the table in the assignment (35/11/11 and 35/12/12)
void printSplitSummary(const vector<Sequence>& seqs) {
    // count[type][split], indexed by the enum values
    int count[2][3] = {};
    for (const auto& s : seqs) count[static_cast<int>(s.type)][static_cast<int>(s.split)]++;
    cout << "\nSplit summary:\n"
              << setw(14) << "" << setw(8) << "train" << setw(12) << "validation"
              << setw(6) << "test\n";
    for (int t = 0; t < 2; ++t) {
        cout << setw(14) << left << typeName(static_cast<SeqType>(t)) << right;
        // Column widths match the header above
        for (int s = 0; s < 3; ++s) cout << setw(s == 1 ? 12 : s == 2 ? 5 : 8) << count[t][s];
        cout << "\n";
    }
}

// Prints the size of a vector set, the average n_I, and the largest deviation of
// sum(x_I) from 1 over the images that have descriptors (should be ~1e-7, i.e. only rounding)
void printVectorStats(const string& name, const VectorSet& vs) {
    size_t valid = 0;
    double descTotal = 0, maxDev = 0;
    for (size_t i = 0; i < vs.size(); ++i) {
        if (vs.numDescriptors[i] == 0) continue;
        ++valid;
        descTotal += vs.numDescriptors[i];
        double sum = 0;
        for (int d = 0; d < vs.dim; ++d) sum += vs.row(i)[d];
        maxDev = max(maxDev, fabs(sum - 1.0));
    }
    cout << name << ": " << vs.size() << " vectors, " << valid << " with descriptors, "
              << vs.size() - valid << " without";
    if (valid)
        cout << ", avg n_I = " << fixed << setprecision(1) << descTotal / valid
                  << defaultfloat << ", max |sum(x_I) - 1| = " << maxDev;
    cout << "\n";
}

} 

int main(int argc, char** argv) {
    // -h / --help prints the usage, even if other options are missing
    for (int i = 1; i < argc; ++i)
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            cout << usage(argv[0]);
            return 0;
        }
    // Bad options: show the error and the usage text
    Args args;
    try {
        args = parseArgs(argc, argv);
    } catch (const exception& e) {
        cerr << "error: " << e.what() << "\n\n" << usage(argv[0]);
        return 1;
    }
    // Problems with the data (missing files, too few images): show only the error
    try {
        printArgs(cout, args);
        const auto seqs = readSplitFile(args.splitFile);
        printSplitSummary(seqs);
        checkHPatches(args.hpatchesDir, seqs);
        cout << "\nHPatches: all " << seqs.size() * kImagesPerSequence << " images present\n";
        const auto mir = listMirflickr(args.mirflickrDir);
        cout << "MIRFlickr: " << mir.size() << " images found";
        if (!mir.empty()) cout << " (" << mir.front() << " .. " << mir.back() << ")";
        cout << "\n";
        // At least D distractor images are needed
        if (static_cast<int>(mir.size()) < args.D)
            throw runtime_error("-D " + to_string(args.D) + " requested but only " +
                                     to_string(mir.size()) + " MIRFlickr images found");
        // Images are processed in parallel by our own threads, so OpenCV's internal
        // threading is turned off to avoid running 8 x 8 threads at once
        cv::setNumThreads(1);
        cout << "\n";
        const FeatureData fd = loadOrBuildFeatures(args, seqs, mir);
        cout << "Vocabulary: K = " << fd.vocab.K << " centers in R^128\n";
        printVectorStats("HPatches (validation + test)", fd.hpatches);
        printVectorStats("MIRFlickr (cached prefix)", fd.mirflickr);
        // Queries and database of the chosen set
        const SearchData sd = buildSearchData(fd, seqs, parseSplit(args.set), args.D);
        cout << "\nQuery set: " << args.set << ", " << sd.queries.size() << " queries\n"
             << "Database: " << sd.db.size() << " images (" << sd.numHPatches << " HPatches + "
             << sd.numMirflickr << " MIRFlickr)\n";
        // Search, evaluation and output file
        auto searcher = makeSearcher(args);
        const ExperimentResult res = runExperiment(*searcher, methodName(args.method), sd,
                                                   args.outputFile);
        cout << "\n" << methodName(args.method) << " results:\n";
        printSummary(cout, res);
        cout << "Output written to " << args.outputFile << "\n";
    } catch (const exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
