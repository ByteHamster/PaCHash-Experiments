#include "competitors/LevelDb.h"
#include "competitors/RocksDb.h"
#include "competitors/Silt.h"
#include "competitors/PaCHash.h"
#include "competitors/RecSplit.h"
#include "competitors/std_unordered_map.h"
#include "competitors/Separator.h"
#include "competitors/Chd.h"
#include "competitors/ParallelCuckoo.h"

int main() {
    for (size_t N = 4e5; N <= 52e5; N += 4e5) {
        for (size_t i = 1; i <= 3; i++) {
            size_t numQueries = 1e6;

            // Full data store
            {SiltComparisonItem(N, numQueries, false).performBenchmark();}
            {SiltComparisonItem(N, numQueries, true).performBenchmark();}
            {LevelDBComparisonItem(N, numQueries).performBenchmark();}
            {RocksDBComparisonItem(N, numQueries, false).performBenchmark();}
            {RocksDBComparisonItem(N, numQueries, true).performBenchmark();}
            {PaCHashComparisonItem(N, numQueries, false).performBenchmark();}
            {PaCHashComparisonItem(N, numQueries, true).performBenchmark();}
            {SeparatorComparisonItem(N, numQueries, false).performBenchmark();}
            {SeparatorComparisonItem(N, numQueries, true).performBenchmark();}
            {CuckooComparisonItem(N, numQueries, true).performBenchmark();}
            {CuckooComparisonItem(N, numQueries, false).performBenchmark();}

            // Static part of dynamic stores
            {LevelDBSingleTableComparisonItem(N, numQueries).performBenchmark();}
            {SiltComparisonItemSortedStore(N, numQueries, false).performBenchmark();}
            {SiltComparisonItemSortedStore(N, numQueries, true).performBenchmark();}

            // Index microbenchmark
            {SiltComparisonItemSortedStoreMicro(N, numQueries).performBenchmark();}
            numQueries = numQueries * 5;
            {PaCHashMicroIndexComparisonItem(N, numQueries).performBenchmark();}
            {LevelDBSingleTableMicroIndexComparisonItem(N, numQueries).performBenchmark();}
            {RecSplitComparisonItem(N, numQueries).performBenchmark();}
            {ChdComparisonItem(N, numQueries).performBenchmark();}
            {StdUnorderedMapComparisonItem(N, numQueries).performBenchmark();}
            {SeparatorMicroIndexComparisonItem(N, numQueries).performBenchmark();}
        }
    }
    return 0;
}
