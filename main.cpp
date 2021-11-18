#include "leveldb_test.h"
#include "rocksdb_test.h"

int main() {
    testLeveldb(100000);
    //testRocksDb();
    return 0;
}
