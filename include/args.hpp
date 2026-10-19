#pragma once

#include <ostream>
#include <string>

// Search method selected on the command line.
enum class Method { None, Exact, LSH, Hypercube, IVFFlat, IVFPQ };

std::string methodName(Method m);

struct LSHParams {
    int k = 4;        // number of h_i functions per g
    int L = 5;        // number of hash tables
    double w = 0.05;  // cell width on the projection line
};

struct HypercubeParams {
    int kproj = 10;   // hypercube dimension d'
    double w = 0.05;  // cell width on the projection line
    int M = 500;      // max candidate points checked
    int probes = 10;  // max hypercube vertices checked
};

struct IVFFlatParams {
    int kclusters = 0;  // 0 means ceil(sqrt(n)), resolved once n is known
    int nprobe = 5;
};

struct IVFPQParams {
    int kclusters = 0;  // 0 means ceil(sqrt(n)), resolved once n is known
    int nprobe = 5;
    int M = 16;         // number of PQ sub-vectors (must divide vocab)
    int nbits = 8;      // 2^nbits centroids per sub-space
};

struct Args {
    std::string hpatchesDir;
    std::string mirflickrDir;
    std::string splitFile;
    std::string outputFile;
    int vocab = 256;
    int S = 500;
    int D = 1000;
    std::string set = "validation";
    unsigned seed = 1;

    Method method = Method::None;
    LSHParams lsh;
    HypercubeParams cube;
    IVFFlatParams ivfflat;
    IVFPQParams ivfpq;
};

// Parses and validates the command line. Throws std::runtime_error on bad input.
Args parseArgs(int argc, char** argv);

std::string usage(const char* prog);

void printArgs(std::ostream& os, const Args& a);
