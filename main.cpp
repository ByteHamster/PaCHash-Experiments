#include "test_leveldb.h"
#include "test_rocksdb.h"
#include "test_silt.h"
#include "test_pachash.h"
#include "test_recsplit.h"
#include "test_std_unordered_map.h"
#include "test_separator.h"
#include "test_chd.h"
#include "test_cuckoo.h"

int main() {
    for (size_t N = 4e5; N <= 52e5; N += 4e5) {
        for (size_t i = 1; i <= 3; i++) {
            size_t numQueries = 1e2;

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
