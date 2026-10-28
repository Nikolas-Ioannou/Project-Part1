// Creates the search method chosen on the command line

#include "searcher.hpp"

#include <stdexcept>

#include "exact_search.hpp"

using namespace std;

unique_ptr<Searcher> makeSearcher(const Args& args) {
    switch (args.method) {
        case Method::Exact:
            return make_unique<ExactSearch>();
        default:
            // Methods without an implementation yet
            throw runtime_error(methodName(args.method) + " is not implemented yet");
    }
}
