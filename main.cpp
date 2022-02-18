#include "test_leveldb.h"
#include "test_rocksdb.h"
#include "test_silt.h"
#include "test_pachash.h"
#include "test_recsplit.h"

int main() {
    for (size_t N = 1e5; N <= 3e6; N += 1e5) {
        for (size_t i = 0; i < 6; i++) {
            size_t averageLength = 256;
            size_t numQueries = 1e6;

            // Full data store
            SiltComparisonItem(N, averageLength, numQueries).performBenchmark();
            LevelDBComparisonItem(N, averageLength, numQueries).performBenchmark();
            RocksDBComparisonItem(N, averageLength, numQueries).performBenchmark();
            PaCHashComparisonItem(N, averageLength, numQueries).performBenchmark();

            // Partial benchmark
            LevelDBSingleTableComparisonItem(N, averageLength, numQueries).performBenchmark();
            SiltComparisonItemSortedStore(N, averageLength, numQueries).performBenchmark();

            // Microbenchmark
            SiltComparisonItemSortedStoreMicro(N, averageLength, numQueries).performBenchmark();
            numQueries = 5e6;
            PaCHashMicroIndexComparisonItem(N, averageLength, numQueries).performBenchmark();
            LevelDBSingleTableMicroIndexComparisonItem(N, averageLength, numQueries).performBenchmark();
            RecSplitComparisonItem(N, averageLength, numQueries).performBenchmark();
        }
    }
    return 0;
}
