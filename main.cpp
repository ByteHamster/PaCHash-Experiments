#include "utils.h"
#include "test_leveldb.h"
#include "test_rocksdb.h"
#include "test_silt.h"
#include "test_pachash.h"

int main() {
    size_t N = 1e5;
    size_t averageLength = 256;
    size_t numQueries = 1e5;

    SiltComparisonItem(N, averageLength, numQueries).performBenchmark();
    LevelDBComparisonItem(N, averageLength, numQueries).performBenchmark();
    RocksDBComparisonItem(N, averageLength, numQueries).performBenchmark();
    PaCHashComparisonItem(N, averageLength, numQueries).performBenchmark();
    return 0;
}
