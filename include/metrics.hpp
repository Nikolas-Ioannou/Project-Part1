// Evaluation metrics (section VII). For a query Q with R(Q) its 5 relevant images:
//   Recall@n(Q) = |Top_n(Q) ∩ R(Q)| / 5
//   AP@10(Q)    = (1/5) * sum_{j=1..m} j / r_j, where r_1 < ... < r_m are the ranks
//                 (1-based) of the relevant images found in the first 10 positions
//   mAP@10      = mean of AP@10 over all queries
// If a method returns fewer than 10 results, the missing positions count as non relevant

#pragma once

#include <vector>

struct QueryMetrics {
    double recall5 = 0;
    double recall10 = 0;
    double ap10 = 0;
};

// relevant[r] tells whether the result at rank r+1 is relevant (relevant.size() <= 10)
// numRelevant is |R(Q)| (5 in this assignment)
QueryMetrics evaluateRanking(const std::vector<bool>& relevant, int numRelevant);

// Running means of the metrics and of the query time over a group of queries
struct MetricsSum {
    int queries = 0;
    int fewerThan10 = 0;  // queries for which the method returned fewer than 10 results
    double recall5 = 0, recall10 = 0, ap10 = 0, timeMs = 0;

    void add(const QueryMetrics& m, double ms, bool incomplete);
    double meanRecall5() const { return queries ? recall5 / queries : 0; }
    double meanRecall10() const { return queries ? recall10 / queries : 0; }
    double mAP10() const { return queries ? ap10 / queries : 0; }
    double meanTimeMs() const { return queries ? timeMs / queries : 0; }
};
