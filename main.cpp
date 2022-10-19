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
#include "competitors/PTHash.h"
#include "competitors/FlatHashMap.h"

int main(int argc, char** argv) {
    BenchmarkConfig benchmarkConfig;
    size_t measurementDelta = 4e5;
    size_t baseNumQueries = 1e6;
    size_t repetitions = 3;

    tlx::CmdlineParser cmd;
    cmd.add_string('p', "path", benchmarkConfig.basePath, "Directory to store data structures in. SSD recommended.");
    cmd.add_flag('v', "variable_size", benchmarkConfig.variableSize, "Use variable size objects");
    cmd.add_bytes('d', "delta_step", measurementDelta, "Step size of N between multiple measurements");
    cmd.add_bytes('q', "num_queries", baseNumQueries, "Number of queries to execute by default. Some benchmarks scale this number to account for slower/faster methods.");
    cmd.add_bytes('r', "repetitions", repetitions, "Number of repetitions to execute for each measurement");
    if (!cmd.process(argc, argv)) {
        return 1;
    } else if (benchmarkConfig.basePath.empty()) {
        std::cerr << "No path given" << std::endl;
        return 1;
    }

    for (benchmarkConfig.N = 4e5; benchmarkConfig.N <= 52e5; benchmarkConfig.N += measurementDelta) {
        for (size_t i = 1; i <= repetitions; i++) {
            // Full data store
            benchmarkConfig.numQueries = baseNumQueries;
            {SiltComparisonItem(benchmarkConfig, false).performBenchmark();}
            {LevelDBComparisonItem(benchmarkConfig).performBenchmark();}
            {RocksDBComparisonItem(benchmarkConfig, false).performBenchmark();}
            {PaCHashComparisonItem(benchmarkConfig, false).performBenchmark();}
            {SeparatorComparisonItem(benchmarkConfig, false).performBenchmark();}
            {CuckooComparisonItem(benchmarkConfig, false).performBenchmark();}
            {PaCHashComparisonItem(benchmarkConfig, true).performBenchmark();}
            {SeparatorComparisonItem(benchmarkConfig, true).performBenchmark();}
            {CuckooComparisonItem(benchmarkConfig, true).performBenchmark();}
            benchmarkConfig.numQueries = baseNumQueries / 10;
            {SiltComparisonItem(benchmarkConfig, true).performBenchmark();}
            {RocksDBComparisonItem(benchmarkConfig, true).performBenchmark();}

            // Static part of dynamic stores
            benchmarkConfig.numQueries = baseNumQueries;
            {LevelDBSingleTableComparisonItem(benchmarkConfig).performBenchmark();}
            {SiltComparisonItemSortedStore(benchmarkConfig, false).performBenchmark();}
            benchmarkConfig.numQueries = baseNumQueries / 10;
            {SiltComparisonItemSortedStore(benchmarkConfig, true).performBenchmark();}

            // Index microbenchmark
            {SiltComparisonItemSortedStoreMicro(benchmarkConfig).performBenchmark();}
            benchmarkConfig.numQueries = baseNumQueries * 5;
            {PaCHashMicroIndexComparisonItem(benchmarkConfig).performBenchmark();}
            {LevelDBSingleTableMicroIndexComparisonItem(benchmarkConfig).performBenchmark();}
            {RecSplitComparisonItem(benchmarkConfig).performBenchmark();}
            {PTHashComparisonItem(benchmarkConfig).performBenchmark();}
            {ChdComparisonItem(benchmarkConfig).performBenchmark();}
            {StdUnorderedMapComparisonItem(benchmarkConfig).performBenchmark();}
            {FlatHashMapComparisonItem(benchmarkConfig).performBenchmark();}
            {SeparatorMicroIndexComparisonItem(benchmarkConfig).performBenchmark();}
        }
    }
    return 0;
}
