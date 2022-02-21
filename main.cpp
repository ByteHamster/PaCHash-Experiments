#include "test_leveldb.h"
#include "test_rocksdb.h"
#include "test_silt.h"
#include "test_pachash.h"
#include "test_recsplit.h"
#include "test_std_unordered_map.h"

int main() {
    for (size_t N = 1e5; N <= 3e6; N += 1e5) {
        for (size_t i = 0; i < 6; i++) {
            size_t objectSize = 256;
            size_t numQueries = 1e6;

            // Full data store
            SiltComparisonItem(N, objectSize, numQueries).performBenchmark();
            LevelDBComparisonItem(N, objectSize, numQueries).performBenchmark();
            RocksDBComparisonItem(N, objectSize, numQueries).performBenchmark();
            PaCHashComparisonItem(N, objectSize, numQueries).performBenchmark();

            // Partial benchmark
            LevelDBSingleTableComparisonItem(N, objectSize, numQueries).performBenchmark();
            SiltComparisonItemSortedStore(N, objectSize, numQueries).performBenchmark();

            // Microbenchmark
            SiltComparisonItemSortedStoreMicro(N, objectSize, numQueries).performBenchmark();
            numQueries = 5e6;
            PaCHashMicroIndexComparisonItem(N, objectSize, numQueries).performBenchmark();
            LevelDBSingleTableMicroIndexComparisonItem(N, objectSize, numQueries).performBenchmark();
            RecSplitComparisonItem(N, objectSize, numQueries).performBenchmark();
            StdUnorderedMapComparisonItem(N, objectSize, numQueries).performBenchmark();
        }
    }
    return 0;
}
