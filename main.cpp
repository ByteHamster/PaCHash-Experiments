#include "utils.h"
#include "test_leveldb.h"
#include "test_rocksdb.h"
#include "test_silt.h"
#include "test_pachash.h"

int main() {
    size_t N = 5e6;
    size_t averageLength = 256;
    size_t numQueries = 1e6;
    //testLeveldb(N, averageLength);
    //testRocksDb(N, averageLength);
    //testSilt(N, averageLength);
    PaCHashComparisonItem(N, averageLength, numQueries).performBenchmark();
    return 0;
}
