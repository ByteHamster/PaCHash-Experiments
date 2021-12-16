#include "rocksdb/db.h"
#include "rocksdb/table.h"
#include <iostream>
#include <random>

void testRocksDb(size_t numKeys, size_t averageLength) {
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::BlockBasedTableOptions table_options;
    table_options.no_block_cache = true;
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    options.use_direct_reads = false;
    options.allow_mmap_reads = true;
    options.env = rocksdb::Env::Default();
    std::string filePath = "/tmp/rocksdb-test";

    std::vector<rocksdb::ColumnFamilyDescriptor> x;
    rocksdb::DestroyDB(filePath, options, x);

    rocksdb::DB* db;
    rocksdb::Status status = rocksdb::DB::Open(options, filePath, &db);
    if (!status.ok()) {
        std::cout<<status.ToString()<<std::endl;
        return;
    }

    std::vector<std::uint64_t> keys = generateRandomKeys(numKeys);
    std::cout<<"Inserting"<<std::endl;
    rocksdb::WriteOptions writeOptions;
    writeOptions.disableWAL = true;
    const char *value = static_cast<const char *>(malloc(averageLength));
    for (uint64_t keyUint : keys) {
        rocksdb::Slice key(reinterpret_cast<const char *>(&keyUint), sizeof(uint64_t));
        db->Put(writeOptions, key, rocksdb::Slice(value, averageLength));
    }
    std::cout<<"Flushing"<<std::endl;
    db->Flush(rocksdb::FlushOptions());

    size_t numBatches = 20000;
    size_t batchSize = 64;
    std::cout<<"Querying"<<std::endl;
    auto queryStart = std::chrono::high_resolution_clock::now();
    std::vector<rocksdb::PinnableSlice> values(batchSize);
    std::vector<uint64_t> queryKeys(batchSize);
    std::vector<rocksdb::Slice> querySlices(batchSize);
    std::vector<rocksdb::Status> statuses(batchSize);

    for (size_t i = 0; i < numBatches; i++) {
        for (size_t k = 0; k < batchSize; k++) {
            queryKeys.at(k) = keys.at(rand() % numKeys);
            querySlices.at(k) = rocksdb::Slice(reinterpret_cast<const char *>(queryKeys.data() + k), sizeof(uint64_t));
        }
        db->MultiGet(rocksdb::ReadOptions(),
                     db->DefaultColumnFamily(),
                     batchSize,
                     querySlices.data(),
                     values.data(),
                     statuses.data());

        for (size_t k = 0; k < batchSize; k++) {
            //uint64_t keyUint = queryKeys.at(k);
            //std::string expected(randomObjectProvider.getValue(keyUint), randomObjectProvider.getLength(keyUint));
            //if (values.at(k).ToString() != expected) {
            //    std::cerr<<"got: "<<values.at(k).ToString()<<std::endl<<"exp: "<<expected<<std::endl;
            //    std::cerr<<statuses.at(k).ToString()<<std::endl;
            //}
            if (!statuses.at(k).ok()) {
                std::cerr<<statuses.at(k).ToString()<<std::endl;
                exit(1);
            }
        }
    }
    auto queryEnd = std::chrono::high_resolution_clock::now();
    long timeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();
    size_t queries = numBatches * batchSize;
    std::cout<<"RESULT"
             <<" objectSize="<<averageLength
             <<" method=rocksdb"
             <<" numObjects="<<numKeys
             <<" numQueries="<<queries
             <<" timeMs="<<timeMicroseconds/1000
             <<" queriesPerSecond="<<(double)queries*1000000.0/((double)timeMicroseconds)
             <<" perObject="<<(((double)timeMicroseconds/(double)queries)*1000)<<std::endl;
    db->Close();
}
