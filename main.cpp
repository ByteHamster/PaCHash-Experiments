#include "test_leveldb.h"
#include "test_rocksdb.h"
#include "test_silt.h"
#include "test_pachash.h"
#include "test_recsplit.h"
#include "test_std_unordered_map.h"
#include "test_separator.h"
#include "test_chd.h"

int main() {
    for (size_t N = 3e5; N <= 45e5; N += 3e5) {
        for (size_t i = 1; i <= 3; i++) {
            size_t objectSize = 256;
            size_t numQueries = 1e6;

            // Full data store
            {SiltComparisonItem(N, objectSize, numQueries, false).performBenchmark();}
            {SiltComparisonItem(N, objectSize, numQueries, true).performBenchmark();}
            {LevelDBComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {RocksDBComparisonItem(N, objectSize, numQueries, false).performBenchmark();}
            {RocksDBComparisonItem(N, objectSize, numQueries, true).performBenchmark();}
            {PaCHashComparisonItem(N, objectSize, numQueries, false).performBenchmark();}
            {PaCHashComparisonItem(N, objectSize, numQueries, true).performBenchmark();}
            {SeparatorComparisonItem(N, objectSize, numQueries, false).performBenchmark();}
            {SeparatorComparisonItem(N, objectSize, numQueries, true).performBenchmark();}

            // Static part of dynamic stores
            {LevelDBSingleTableComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {SiltComparisonItemSortedStore(N, objectSize, numQueries, false).performBenchmark();}
            {SiltComparisonItemSortedStore(N, objectSize, numQueries, true).performBenchmark();}

            // Index microbenchmark
            {SiltComparisonItemSortedStoreMicro(N, objectSize, numQueries).performBenchmark();}
            numQueries = numQueries * 5;
            {PaCHashMicroIndexComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {LevelDBSingleTableMicroIndexComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {RecSplitComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {ChdComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {StdUnorderedMapComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {SeparatorMicroIndexComparisonItem(N, objectSize, numQueries).performBenchmark();}
        }
    }
    return 0;
}
