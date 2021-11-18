#include "rocksdb/db.h"
#include "rocksdb/table.h"
#include <iostream>
#include <random>
#include "external/elias-fano/benchmark/RandomObjectProvider.h"

void buildTable() {
    /*rocksdb::BlockBasedTableOptions table_options;
    table_options.no_block_cache = true;
    rocksdb::ImmutableOptions immutableOptions;
    rocksdb::MutableCFOptions mutableCfOptions;
    rocksdb::InternalKeyComparator internalKeyComparator;
    rocksdb::IntTblPropCollectorFactories propCollectorFactories;
    rocksdb::CompressionType compressionType;
    rocksdb::CompressionOptions compressionOptions;
    std::string columnFamilyName = "a";
    rocksdb::TableBuilderOptions builderOptions(immutableOptions, mutableCfOptions,
                                                internalKeyComparator,
                                                &propCollectorFactories,
                                                compressionType,
                                                compressionOptions,
                                                0,
                                                columnFamilyName,
                                                0);
    std::string filename = "/tmp/test.txt";
    int fd = open(filename.c_str(), O_RDWR);
    rocksdb::FileOptions options;
    options.use_direct_reads = true;
    rocksdb::WritableFileWriter fileWriter(std::make_unique<rocksdb::PosixWritableFile>(
            filename, fd, 4096, rocksdb::EnvOptions()), filename, options);
    rocksdb::BlockBasedTableBuilder tableBuilder(table_options, builderOptions, &fileWriter);


    for (int i = 0; i < 10; i++) {
        std::string key = "Hallo" + std::to_string(i);
        rocksdb::Slice keySlice = rocksdb::Slice(key.data(), key.length());
        std::string value = "Value" + std::to_string(i);
        rocksdb::Slice valueSlice = rocksdb::Slice(value.data(), value.length());
        tableBuilder.Add(keySlice, valueSlice);
    }
    tableBuilder.Finish();*/
}

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

    size_t numBatches = 1000;
    size_t batchSize = 64;
    std::cout<<"Querying"<<std::endl;
    auto queryStart = std::chrono::high_resolution_clock::now();
    std::vector<rocksdb::PinnableSlice> values(batchSize);
    std::vector<uint64_t> queryKeys(batchSize);
    std::vector<rocksdb::Slice> querySlices(batchSize);
    std::vector<rocksdb::Status> statuses(batchSize);

    for (size_t i = 0; i < numBatches; i++) {
        for (int k = 0; k < batchSize; k++) {
            queryKeys.at(k) = keys.at(rand() % numKeys);
            querySlices.at(k) = rocksdb::Slice(reinterpret_cast<const char *>(queryKeys.data() + k), sizeof(uint64_t));
        }
        db->MultiGet(rocksdb::ReadOptions(),
                     db->DefaultColumnFamily(),
                     batchSize,
                     querySlices.data(),
                     values.data(),
                     statuses.data());

        for (int k = 0; k < batchSize; k++) {
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
    std::cout<<"Queries: "<<numBatches*batchSize<<", Microseconds: "<<timeMicroseconds<<std::endl;
    std::cout<<"kQueries/s: "<<(double)numBatches*(double)batchSize*1000.0/((double)timeMicroseconds)<<std::endl;

    db->Close();
}
