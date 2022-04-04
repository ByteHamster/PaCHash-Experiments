#include "test_leveldb.h"
#include "test_rocksdb.h"
#include "test_silt.h"
#include "test_pachash.h"
#include "test_recsplit.h"
#include "test_std_unordered_map.h"
#include "test_separator.h"
#include "test_chd.h"

int main() {
    for (size_t N = 1e5; N <= 4e6; N += 2e5) {
        for (size_t i = 0; i < 4; i++) {
            size_t objectSize = 256;
            size_t numQueries = 1e6;

            // Full data store
            {SiltComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {LevelDBComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {RocksDBComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {PaCHashComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {SeparatorComparisonItem(N, objectSize, numQueries).performBenchmark();}

            // Partial benchmark
            {LevelDBSingleTableComparisonItem(N, objectSize, numQueries).performBenchmark();}
            {SiltComparisonItemSortedStore(N, objectSize, numQueries).performBenchmark();}

            // Microbenchmark
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
