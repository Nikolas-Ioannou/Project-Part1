#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>

#include "args.hpp"
#include "dataset.hpp"

namespace {

void printSplitSummary(const std::vector<Sequence>& seqs) {
    int count[2][3] = {};
    for (const auto& s : seqs) count[static_cast<int>(s.type)][static_cast<int>(s.split)]++;

    std::cout << "\nSplit summary:\n"
              << std::setw(14) << "" << std::setw(8) << "train" << std::setw(12) << "validation"
              << std::setw(6) << "test\n";
    for (int t = 0; t < 2; ++t) {
        std::cout << std::setw(14) << std::left << typeName(static_cast<SeqType>(t)) << std::right;
        for (int s = 0; s < 3; ++s) std::cout << std::setw(s == 1 ? 12 : s == 2 ? 5 : 8) << count[t][s];
        std::cout << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        if (!std::strcmp(argv[i], "-h") || !std::strcmp(argv[i], "--help")) {
            std::cout << usage(argv[0]);
            return 0;
        }

    Args args;
    try {
        args = parseArgs(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n\n" << usage(argv[0]);
        return 1;
    }

    try {
        printArgs(std::cout, args);

        const auto seqs = readSplitFile(args.splitFile);
        printSplitSummary(seqs);

        checkHPatches(args.hpatchesDir, seqs);
        std::cout << "\nHPatches: all " << seqs.size() * kImagesPerSequence << " images present\n";

        const auto mir = listMirflickr(args.mirflickrDir);
        std::cout << "MIRFlickr: " << mir.size() << " images found";
        if (!mir.empty()) std::cout << " (" << mir.front() << " .. " << mir.back() << ")";
        std::cout << "\n";
        if (static_cast<int>(mir.size()) < args.D)
            throw std::runtime_error("-D " + std::to_string(args.D) + " requested but only " +
                                     std::to_string(mir.size()) + " MIRFlickr images found");
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
