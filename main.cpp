#include <tlx/cmdline_parser.hpp>
#include "competitors/LevelDb.h"
#include "competitors/RocksDb.h"
#include "competitors/Silt.h"
#include "competitors/PaCHash.h"
#include "competitors/RecSplit.h"
#include "competitors/std_unordered_map.h"
#include "competitors/Separator.h"
#include "competitors/Chd.h"
#include "competitors/ParallelCuckoo.h"

int main(int argc, char** argv) {
    BenchmarkConfig benchmarkConfig;

    tlx::CmdlineParser cmd;
    cmd.add_string('p', "path", benchmarkConfig.basePath, "Path to directory to store files in");
    cmd.add_flag('v', "variable_size", benchmarkConfig.variableSize, "Use variable size objects");
    if (!cmd.process(argc, argv)) {
        return 1;
    }
    benchmarkConfig.basePath = "/data02/hplehmann/";

    for (benchmarkConfig.N = 4e5; benchmarkConfig.N <= 52e5; benchmarkConfig.N += 4e5) {
        for (size_t i = 1; i <= 3; i++) {
            benchmarkConfig.numQueries = 1e6;

            // Full data store
            {SiltComparisonItem(benchmarkConfig, false).performBenchmark();}
            {SiltComparisonItem(benchmarkConfig, true).performBenchmark();}
            {LevelDBComparisonItem(benchmarkConfig).performBenchmark();}
            {RocksDBComparisonItem(benchmarkConfig, false).performBenchmark();}
            {RocksDBComparisonItem(benchmarkConfig, true).performBenchmark();}
            {PaCHashComparisonItem(benchmarkConfig, false).performBenchmark();}
            {PaCHashComparisonItem(benchmarkConfig, true).performBenchmark();}
            {SeparatorComparisonItem(benchmarkConfig, false).performBenchmark();}
            {SeparatorComparisonItem(benchmarkConfig, true).performBenchmark();}
            {CuckooComparisonItem(benchmarkConfig, true).performBenchmark();}
            {CuckooComparisonItem(benchmarkConfig, false).performBenchmark();}

            // Static part of dynamic stores
            {LevelDBSingleTableComparisonItem(benchmarkConfig).performBenchmark();}
            {SiltComparisonItemSortedStore(benchmarkConfig, false).performBenchmark();}
            {SiltComparisonItemSortedStore(benchmarkConfig, true).performBenchmark();}

            // Index microbenchmark
            {SiltComparisonItemSortedStoreMicro(benchmarkConfig).performBenchmark();}
            benchmarkConfig.numQueries *= 5;
            {PaCHashMicroIndexComparisonItem(benchmarkConfig).performBenchmark();}
            {LevelDBSingleTableMicroIndexComparisonItem(benchmarkConfig).performBenchmark();}
            {RecSplitComparisonItem(benchmarkConfig).performBenchmark();}
            {ChdComparisonItem(benchmarkConfig).performBenchmark();}
            {StdUnorderedMapComparisonItem(benchmarkConfig).performBenchmark();}
            {SeparatorMicroIndexComparisonItem(benchmarkConfig).performBenchmark();}
        }
    }
    return 0;
}
