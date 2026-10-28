// Recall@5, Recall@10 and AP@10 of one ranking, and their means

#include "metrics.hpp"

#include <algorithm>

using namespace std;

QueryMetrics evaluateRanking(const vector<bool>& relevant, int numRelevant) {
    QueryMetrics m;
    int found = 0;  // relevant images seen so far (j in the AP formula)
    const size_t n = min<size_t>(relevant.size(), 10);
    for (size_t r = 0; r < n; ++r) {
        if (!relevant[r]) continue;
        ++found;
        if (r < 5) m.recall5 += 1;
        m.recall10 += 1;
        // j / r_j with 1-based rank r_j = r + 1
        m.ap10 += static_cast<double>(found) / static_cast<double>(r + 1);
    }
    m.recall5 /= numRelevant;
    m.recall10 /= numRelevant;
    m.ap10 /= numRelevant;
    return m;
}

void MetricsSum::add(const QueryMetrics& m, double ms, bool incomplete) {
    ++queries;
    fewerThan10 += incomplete;
    recall5 += m.recall5;
    recall10 += m.recall10;
    ap10 += m.ap10;
    timeMs += ms;
}
