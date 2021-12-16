#include "utils.h"
#include "test_leveldb.h"
#include "test_rocksdb.h"
#include "test_silt.h"
#include "test_pachash.h"

int main() {
    size_t N = 5e6;
    size_t averageLength = 256;
    testLeveldb(N, averageLength);
    testRocksDb(N, averageLength);
    testSilt(N, averageLength);
    testPaCHash(N, averageLength);
    return 0;
}
