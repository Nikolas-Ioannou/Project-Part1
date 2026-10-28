// Query loop, timing, evaluation and output file

#include "experiment.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <stdexcept>

using namespace std;

namespace {

using Clock = chrono::steady_clock;

double msSince(Clock::time_point t0) {
    return chrono::duration<double, milli>(Clock::now() - t0).count();
}

}  // namespace

ExperimentResult runExperiment(Searcher& searcher, const string& methodName,
                               const SearchData& sd, const string& outputFile) {
    ofstream out(outputFile);
    if (!out) throw runtime_error("cannot write output file " + outputFile);
    out << fixed << setprecision(6);
    ExperimentResult res;
    // Index construction is timed separately and is not part of the query time
    auto t0 = Clock::now();
    searcher.build(sd.db);
    res.buildMs = msSince(t0);
    out << methodName << "\n";
    for (size_t q = 0; q < sd.queries.size(); ++q) {
        // Query time: from the moment x_Q is available until the results are returned
        t0 = Clock::now();
        const vector<int> result = searcher.search(sd.queries.row(q), kTopN);
        const double ms = msSince(t0);
        // A result is relevant if it is one of I2..I6 of the query's own sequence
        vector<bool> relevant;
        for (int idx : result) relevant.push_back(sd.dbQuery[idx] == static_cast<int>(q));
        const QueryMetrics m = evaluateRanking(relevant, kRelevantPerQuery);

        const bool incomplete = static_cast<int>(result.size()) < kTopN;
        res.all.add(m, ms, incomplete);
        (sd.queryType[q] == SeqType::Illumination ? res.illumination : res.viewpoint)
            .add(m, ms, incomplete);
        // Per-query block. Missing positions (fewer than 10 results) are printed as "-"
        out << "Query: " << sd.queries.ids[q] << "\n";
        for (int r = 0; r < kTopN; ++r)
            out << "Rank-" << r + 1 << ": "
                << (r < static_cast<int>(result.size()) ? sd.db.ids[result[r]] : "-") << "\n";
        out << "Recall@5: " << m.recall5 << "\n"
            << "Recall@10: " << m.recall10 << "\n"
            << "AP@10: " << m.ap10 << "\n\n";
    }

    out << "Mean Recall@5: " << res.all.meanRecall5() << "\n"
        << "Mean Recall@10: " << res.all.meanRecall10() << "\n"
        << "mAP@10: " << res.all.mAP10() << "\n"
        << "Average query time: " << res.all.meanTimeMs() << "\n";
    if (!out) throw runtime_error("error while writing " + outputFile);
    return res;
}

void printSummary(ostream& os, const ExperimentResult& r) {
    os << left << setw(14) << "" << right << setw(8) << "queries" << setw(11) << "Recall@5"
       << setw(11) << "Recall@10" << setw(9) << "mAP@10" << setw(12) << "time (ms)"
       << setw(10) << "<10 res" << "\n";
    auto row = [&](const string& name, const MetricsSum& m) {
        os << left << setw(14) << name << right << setw(8) << m.queries << fixed
           << setprecision(4) << setw(11) << m.meanRecall5() << setw(11) << m.meanRecall10()
           << setw(9) << m.mAP10() << setw(12) << m.meanTimeMs() << setw(10) << m.fewerThan10
           << "\n";
    };
    row("all", r.all);
    row("illumination", r.illumination);
    row("viewpoint", r.viewpoint);
    os << "Index build time: " << fixed << setprecision(1) << r.buildMs << " ms\n"
       << defaultfloat;
}
