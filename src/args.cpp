// Parsing and validation of the command-line options declared in args.hpp

#include "args.hpp"
#include <sstream>
#include <stdexcept>

using namespace std;

namespace {

// Converts the value of an option to int. The whole string must be a number,
// so "12abc" or "" are rejected instead of being silently read as 12 or 0
int toInt(const string& flag, const string& v) {
    size_t pos = 0;  // number of characters consumed by stoi
    int x = 0;
    try {
        x = stoi(v, &pos);
    } catch (const exception&) {
        pos = 0;  // not a number at all, or out of range
    }
    if (pos == 0 || pos != v.size())
        throw runtime_error("invalid integer for " + flag + ": '" + v + "'");
    return x;
}

// Same as toInt, for options that take a real number (e.g. -w)
double toDouble(const string& flag, const string& v) {
    size_t pos = 0;
    double x = 0;
    try {
        x = stod(v, &pos);
    } catch (const exception&) {
        pos = 0;
    }
    if (pos == 0 || pos != v.size())
        throw runtime_error("invalid number for " + flag + ": '" + v + "'");
    return x;
}

// Maps a method flag ("-lsh", ...) to the Method enum. None if it is not a method flag
Method methodFromFlag(const string& f) {
    if (f == "-exact") return Method::Exact;
    if (f == "-lsh") return Method::LSH;
    if (f == "-hypercube") return Method::Hypercube;
    if (f == "-ivfflat") return Method::IVFFlat;
    if (f == "-ivfpq") return Method::IVFPQ;
    return Method::None;
}

// Inverse of methodFromFlag, used in error messages
string methodFlag(Method m) {
    switch (m) {
        case Method::Exact: return "-exact";
        case Method::LSH: return "-lsh";
        case Method::Hypercube: return "-hypercube";
        case Method::IVFFlat: return "-ivfflat";
        case Method::IVFPQ: return "-ivfpq";
        default: return "(none)";
    }
}

// Throws message as an error if condition is false
void require(bool cond, const string& msg) {
    if (!cond) throw runtime_error(msg);
}

// Checks that all values are in a valid range once parsing is complete
// Only the parameters of the selected method are checked
void validate(const Args& a) {
    // Common options
    require(!a.hpatchesDir.empty(), "missing required option -hp");
    require(!a.mirflickrDir.empty(), "missing required option -mir");
    require(!a.splitFile.empty(), "missing required option -split");
    require(!a.outputFile.empty(), "missing required option -o");
    require(!a.cacheDir.empty(), "-cache must not be empty");
    require(a.vocab >= 1, "-vocab must be >= 1");
    require(a.S >= 1, "-S must be >= 1");
    require(a.D >= 0 && a.D <= 25000, "-D must be in [0, 25000]");  // MIRFlickr has 25000 images
    require(a.set == "validation" || a.set == "test", "-set must be 'validation' or 'test'");
    // Method specific options
    switch (a.method) {
        case Method::LSH:
            require(a.lsh.k >= 1, "-k must be >= 1");
            require(a.lsh.L >= 1, "-L must be >= 1");
            require(a.lsh.w > 0, "-w must be > 0");
            break;
        case Method::Hypercube:
            // A vertex of the hypercube is a kproj-bit number, so kproj must fit in an int
            require(a.cube.kproj >= 1 && a.cube.kproj <= 30, "-kproj must be in [1, 30]");
            require(a.cube.w > 0, "-w must be > 0");
            require(a.cube.M >= 1, "-M must be >= 1");
            require(a.cube.probes >= 1, "-probes must be >= 1");
            break;
        case Method::IVFFlat:
            // kclusters == 0 is the "not given" value, replaced later by ceil(sqrt(n))
            require(a.ivfflat.kclusters >= 0, "-kclusters must be >= 1");
            require(a.ivfflat.nprobe >= 1, "-nprobe must be >= 1");
            break;
        case Method::IVFPQ:
            require(a.ivfpq.kclusters >= 0, "-kclusters must be >= 1");
            require(a.ivfpq.nprobe >= 1, "-nprobe must be >= 1");
            require(a.ivfpq.M >= 1, "-M must be >= 1");
            // Each image vector (dimension K = vocab) is split into M equal sub-vectors
            require(a.vocab % a.ivfpq.M == 0,
                    "-vocab (" + to_string(a.vocab) + ") must be divisible by -M (" +
                        to_string(a.ivfpq.M) + ")");
            require(a.ivfpq.nbits >= 1 && a.ivfpq.nbits <= 16, "-nbits must be in [1, 16]");
            break;
        default:
            break;
    }
}

}  // namespace

string methodName(Method m) {
    switch (m) {
        case Method::Exact: return "Exact Search";
        case Method::LSH: return "LSH";
        case Method::Hypercube: return "Hypercube";
        case Method::IVFFlat: return "IVFFlat";
        case Method::IVFPQ: return "IVFPQ";
        default: return "None";
    }
}

Args parseArgs(int argc, char** argv) {
    Args a;

    // First pass: find the method, since -w, -M, -kclusters, -nprobe are shared
    // between methods and their meaning depends on it. The method flag may appear
    // anywhere on the command line, even after its own parameters
    for (int i = 1; i < argc; ++i) {
        Method m = methodFromFlag(argv[i]);
        if (m == Method::None) continue;
        if (a.method != Method::None && a.method != m)
            throw runtime_error("only one search method may be given (" +
                                     methodFlag(a.method) + " and " + argv[i] + ")");
        a.method = m;
    }
    if (a.method == Method::None)
        throw runtime_error(
            "no search method given (-exact, -lsh, -hypercube, -ivfflat or -ivfpq)");

    // Second pass: every other option is a "-name value" pair
    for (int i = 1; i < argc; ++i) {
        const string f = argv[i];
        if (methodFromFlag(f) != Method::None) continue;  // already handled above
        // The value must exist and must not be a method flag (e.g. "-k -lsh")
        if (i + 1 >= argc || methodFromFlag(argv[i + 1]) != Method::None)
            throw runtime_error("missing value for " + f);
        const string v = argv[++i];
        const Method m = a.method;
        // Error for a known option used with the wrong method (e.g. -k with -ivfflat)
        auto notFor = [&]() {
            return runtime_error("option " + f + " does not apply to " + methodFlag(m));
        };

        // Common options
        if (f == "-hp") a.hpatchesDir = v;
        else if (f == "-mir") a.mirflickrDir = v;
        else if (f == "-split") a.splitFile = v;
        else if (f == "-o") a.outputFile = v;
        else if (f == "-vocab") a.vocab = toInt(f, v);
        else if (f == "-S") a.S = toInt(f, v);
        else if (f == "-D") a.D = toInt(f, v);
        else if (f == "-set") a.set = v;
        else if (f == "-cache") a.cacheDir = v;
        else if (f == "-seed") {
            int s = toInt(f, v);
            require(s >= 0, "-seed must be >= 0");
            a.seed = static_cast<unsigned>(s);
        }
        // LSH only
        else if (f == "-k") {
            if (m != Method::LSH) throw notFor();
            a.lsh.k = toInt(f, v);
        }
        else if (f == "-L") {
            if (m != Method::LSH) throw notFor();
            a.lsh.L = toInt(f, v);
        }
        // -w: LSH or Hypercube
        else if (f == "-w") {
            if (m == Method::LSH) a.lsh.w = toDouble(f, v);
            else if (m == Method::Hypercube) a.cube.w = toDouble(f, v);
            else throw notFor();
        }
        // Hypercube only
        else if (f == "-kproj") {
            if (m != Method::Hypercube) throw notFor();
            a.cube.kproj = toInt(f, v);
        }
        else if (f == "-probes") {
            if (m != Method::Hypercube) throw notFor();
            a.cube.probes = toInt(f, v);
        }
        // -M: max candidates for Hypercube, number of sub-vectors for IVFPQ
        else if (f == "-M") {
            if (m == Method::Hypercube) a.cube.M = toInt(f, v);
            else if (m == Method::IVFPQ) a.ivfpq.M = toInt(f, v);
            else throw notFor();
        }
        // -kclusters / -nprobe: IVFFlat or IVFPQ
        else if (f == "-kclusters") {
            int x = toInt(f, v);
            require(x >= 1, "-kclusters must be >= 1");  // 0 is reserved for "not given"
            if (m == Method::IVFFlat) a.ivfflat.kclusters = x;
            else if (m == Method::IVFPQ) a.ivfpq.kclusters = x;
            else throw notFor();
        }
        else if (f == "-nprobe") {
            if (m == Method::IVFFlat) a.ivfflat.nprobe = toInt(f, v);
            else if (m == Method::IVFPQ) a.ivfpq.nprobe = toInt(f, v);
            else throw notFor();
        }
        // IVFPQ only
        else if (f == "-nbits") {
            if (m != Method::IVFPQ) throw notFor();
            a.ivfpq.nbits = toInt(f, v);
        }
        else throw runtime_error("unknown option " + f);
    }

    validate(a);
    return a;
}

string usage(const char* prog) {
    ostringstream os;
    os << "Usage: " << prog
       << " -hp <path> -mir <path> -split <file> -o <output file>\n"
          "       [-vocab <int>] [-S <int>] [-D <int>] [-set validation|test] [-seed <int>]\n"
          "       [-cache <dir>]\n"
          "       <method> [method options]\n"
          "\n"
          "Methods and their options:\n"
          "  -exact\n"
          "  -lsh        [-k <int>=4] [-L <int>=5] [-w <double>=0.05]\n"
          "  -hypercube  [-kproj <int>=10] [-w <double>=0.05] [-M <int>=500] [-probes <int>=10]\n"
          "  -ivfflat    [-kclusters <int>=ceil(sqrt(n))] [-nprobe <int>=5]\n"
          "  -ivfpq      [-kclusters <int>=ceil(sqrt(n))] [-nprobe <int>=5] [-M <int>=16]"
          " [-nbits <int>=8]\n"
          "\n"
          "Defaults: -vocab 256 -S 500 -D 1000 -set validation -seed 1 -cache cache\n";
    return os.str();
}

void printArgs(ostream& os, const Args& a) {
    os << "HPatches:   " << a.hpatchesDir << "\n"
       << "MIRFlickr:  " << a.mirflickrDir << "\n"
       << "Split file: " << a.splitFile << "\n"
       << "Output:     " << a.outputFile << "\n"
       << "Cache:      " << a.cacheDir << "\n"
       << "vocab=" << a.vocab << " S=" << a.S << " D=" << a.D << " set=" << a.set
       << " seed=" << a.seed << "\n"
       << "Method:     " << methodName(a.method);
    // kclusters is shown as "auto" while it still holds the ceil(sqrt(n)) placeholder
    auto kc = [](int k) { return k == 0 ? string("auto") : to_string(k); };
    switch (a.method) {
        case Method::LSH:
            os << " (k=" << a.lsh.k << " L=" << a.lsh.L << " w=" << a.lsh.w << ")";
            break;
        case Method::Hypercube:
            os << " (kproj=" << a.cube.kproj << " w=" << a.cube.w << " M=" << a.cube.M
               << " probes=" << a.cube.probes << ")";
            break;
        case Method::IVFFlat:
            os << " (kclusters=" << kc(a.ivfflat.kclusters) << " nprobe=" << a.ivfflat.nprobe
               << ")";
            break;
        case Method::IVFPQ:
            os << " (kclusters=" << kc(a.ivfpq.kclusters) << " nprobe=" << a.ivfpq.nprobe
               << " M=" << a.ivfpq.M << " nbits=" << a.ivfpq.nbits << ")";
            break;
        default:
            break;
    }
    os << "\n";
}
