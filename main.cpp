#include "utils.h"
#include "test_leveldb.h"
#include "test_rocksdb.h"
#include "test_silt.h"
#include "test_pachash.h"

int main() {
    testLeveldb(1e6, 100);
    testRocksDb(1e6, 100);
    testSilt(1e6, 100);
    testPaCHash(1e6, 100);
    return 0;
}
