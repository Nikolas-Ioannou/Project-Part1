// Runs all queries of a set with one search method, evaluates them and writes the output
// file in the format of section VI

#pragma once

#include <ostream>
#include <string>

#include "metrics.hpp"
#include "searcher.hpp"
#include "database.hpp"

// Number of results returned per query
constexpr int kTopN = 10;

struct ExperimentResult {
    MetricsSum all;           // every query of the set
    MetricsSum illumination;  // only i_* sequences
    MetricsSum viewpoint;     // only v_* sequences
    double buildMs = 0;       // index construction time (not part of the query time)
    std::string stats;        // method-specific statistics (Searcher::stats)
};

// Builds the searcher's index over sd.db, runs every query, writes the output file
// and returns the summary. Throws std::runtime_error if the output file cannot be written
ExperimentResult runExperiment(Searcher& searcher, const std::string& methodName,
                               const SearchData& sd, const std::string& outputFile);

// Prints the summary table (all / illumination / viewpoint) to os
void printSummary(std::ostream& os, const ExperimentResult& r);
