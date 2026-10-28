// Command line options of the program (section V of the assignment)
// The common options (-hp, -mir, -split, -vocab, -S, -D, -set, -o, -seed) are stored directly in Args
// Exactly one search method is selected with -exact, -lsh, -hypercube,
// -ivfflat or -ivfpq, and each method keeps its own optional parameters in a separate
// struct, initialised with the default values given in the assignment.

#pragma once

#include <ostream>
#include <string>

// Search method selected on the command line. None means no method flag was given
enum class Method { None, Exact, LSH, Hypercube, IVFFlat, IVFPQ };

// Readable method name, used as <METHOD NAME> in the output file.
std::string methodName(Method m);

// Parameters of -lsh
struct LSHParams {
    int k = 4;        // -k: number of h_i functions combined into each g
    int L = 5;        // -L: number of hash tables
    double w = 0.05;  // -w: cell width on the projection line
};

// Parameters of the random projection on the -hypercube
struct HypercubeParams {
    int kproj = 10;   // -kproj: hypercube dimension d'
    double w = 0.05;  // -w: cell width on the projection line
    int M = 500;      // -M: max number of candidate points checked
    int probes = 10;  // -probes: max number of hypercube vertices checked
};

// Parameters of -ivfflat
struct IVFFlatParams {
    int kclusters = 0;  // -kclusters: 0 means ceil(sqrt(n)), resolved once n is known
    int nprobe = 5;     // -nprobe: number of nearest clusters searched
};

// Parameters of -ivfpq
struct IVFPQParams {
    int kclusters = 0;  // -kclusters: 0 means ceil(sqrt(n)), resolved once n is known
    int nprobe = 5;     // -nprobe: number of nearest clusters searched
    int M = 16;         // -M: number of PQ sub-vectors (must divide vocab)
    int nbits = 8;      // -nbits: 2^nbits centroids per sub-space
};

// All options of one run
struct Args {
    std::string hpatchesDir;   // -hp: HPatches directory
    std::string mirflickrDir;  // -mir: MIRFlickr-25K directory
    std::string splitFile;     // -split: train/validation/test split file
    std::string outputFile;    // -o: output file
    int vocab = 256;           // -vocab: number K of SIFT k-means centers
    int S = 500;               // -S: max SIFT descriptors per training image for k-means
    int D = 1000;              // -D: number of MIRFlickr distractor images
    std::string set = "validation";  // -set: query set (validation or test)
    unsigned seed = 1;         // -seed: seed for every random step
    std::string cacheDir = "cache";  // -cache: where vocabulary and vectors are cached

    Method method = Method::None;
    LSHParams lsh;
    HypercubeParams cube;
    IVFFlatParams ivfflat;
    IVFPQParams ivfpq;
};

// Parses and validates the command lin
// Message on bad input 
Args parseArgs(int argc, char** argv);

// Helps text listing every option and its default
std::string usage(const char* prog);

// Prints the parsed options, so each run records the parameters it used
void printArgs(std::ostream& os, const Args& a);
