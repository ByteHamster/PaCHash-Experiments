#include "rocksdb/db.h"
#include "rocksdb/table.h"
#include <iostream>
#include <random>
#include "external/elias-fano/benchmark/RandomObjectProvider.h"

static std::vector<uint64_t> generateRandomKeys(size_t N) {
    uint64_t seed = std::random_device{}();
    std::cout<<"# Seed for input keys: "<<seed<<std::endl;
    std::mt19937_64 generator(seed);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    std::vector<uint64_t> keys;
    keys.reserve(N);
    for (size_t i = 0; i < N; i++) {
        uint64_t key = dist(generator);
        keys.emplace_back(key);
    }
    return keys;
}

void testRocksDb() {
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::BlockBasedTableOptions table_options;
    table_options.no_block_cache = true;
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    options.use_direct_reads = true;
    options.allow_mmap_reads = false;
    options.env = rocksdb::Env::Default();
    std::string filePath = "/home/hans-peter/testdb";

    std::vector<rocksdb::ColumnFamilyDescriptor> x;
    rocksdb::DestroyDB(filePath, options, x);

    rocksdb::DB* db;
    rocksdb::Status status = rocksdb::DB::Open(options, filePath, &db);
    if (!status.ok()) {
        std::cout<<status.ToString()<<std::endl;
        return;
    }

    size_t numKeys = 1e5;
    std::vector<std::uint64_t> keys = generateRandomKeys(numKeys);
    RandomObjectProvider randomObjectProvider(EQUAL_DISTRIBUTION, 512);
    std::cout<<"Inserting"<<std::endl;
    rocksdb::WriteOptions writeOptions;
    writeOptions.disableWAL = true;
    for (uint64_t keyUint : keys) {
        rocksdb::Slice key(reinterpret_cast<const char *>(&keyUint), sizeof(uint64_t));
        size_t length = randomObjectProvider.getLength(keyUint);
        const char *value = randomObjectProvider.getValue(keyUint);
        db->Put(writeOptions, key, rocksdb::Slice(value, length));
    }
    std::cout<<"Flushing"<<std::endl;
    db->Flush(rocksdb::FlushOptions());

    size_t numBatches = 4000;
    size_t batchSize = 20;
    std::cout<<"Querying"<<std::endl;
    auto queryStart = std::chrono::high_resolution_clock::now();
    std::vector<std::string> values(batchSize);
    std::vector<uint64_t> queryKeys(batchSize);
    std::vector<rocksdb::Slice> querySlices(batchSize);

    for (size_t i = 0; i < numBatches; i++) {
        for (int k = 0; k < batchSize; k++) {
            values.at(k) = "42";
            queryKeys.at(k) = keys.at(rand() % numKeys);
            querySlices.at(k) = rocksdb::Slice(reinterpret_cast<const char *>(queryKeys.data() + k), sizeof(uint64_t));
        }
        std::vector<rocksdb::Status> status = db->MultiGet(rocksdb::ReadOptions(), querySlices, &values);

        for (int k = 0; k < batchSize; k++) {
            //uint64_t keyUint = queryKeys.at(k);
            //std::string expected(randomObjectProvider.getValue(keyUint), randomObjectProvider.getLength(keyUint));
            //if (values.at(k) != expected) {
            //    std::cerr<<"got: "<<values.at(k)<<std::endl<<"exp: "<<expected<<std::endl;
            //    std::cerr<<status.at(k).ToString()<<std::endl;
            //}
            if (!status.at(k).ok()) {
                std::cerr<<status.at(k).ToString()<<std::endl;
                exit(1);
            }
        }
    }
    auto queryEnd = std::chrono::high_resolution_clock::now();
    long timeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();
    std::cout<<"Queries: "<<numBatches*batchSize<<", Microseconds: "<<timeMicroseconds<<std::endl;
    std::cout<<"kQueries/s: "<<(double)numBatches*(double)batchSize*1000.0/((double)timeMicroseconds)<<std::endl;

    db->Close();
}
