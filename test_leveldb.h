#include <leveldb/env.h>
#include <leveldb/table.h>
#include "leveldb/table_builder.h"
#include <memory>

static std::vector<std::string> randomKeys;
size_t length = 0;

static void handleResult(void *arg, const leveldb::Slice &key, const leveldb::Slice &value) {
    //std::cout<<std::string(key.data(), key.size())<<": "<<std::string(value.data(), value.size())<<std::endl;
    (void) arg;
    (void) key;
    assert(value.size() == length);
}

static void testLeveldb(size_t N, size_t averageLength) {
    length = averageLength;
    randomKeys = generateRandomStringKeys(N);
    std::sort(randomKeys.begin(), randomKeys.end());

    std::string filename = "/tmp/leveldb-test";
    leveldb::Options options;
    options.block_size = 4096 - 150; // Headers etc. Ensures that pread calls are limited to <4096
    options.compression = leveldb::CompressionType::kNoCompression;
    leveldb::WritableFile *file = nullptr;
    leveldb::Env::Default()->DeleteFile(filename);
    leveldb::Env::Default()->NewWritableFile(filename, &file);
    leveldb::TableBuilder tableBuilder(options, file);

    std::cout<<"Inserting"<<std::endl;
    const char *value = static_cast<const char *>(malloc(averageLength));
    for (std::string &key : randomKeys) {
        leveldb::Slice keySlice(key.data(), sizeof(uint64_t));
        leveldb::Slice valueSlice = leveldb::Slice(value, averageLength);
        tableBuilder.Add(keySlice, valueSlice);
    }
    leveldb::Status status = tableBuilder.Finish();
    file->Close();
    size_t size = tableBuilder.FileSize();

    leveldb::Table *table = nullptr;
    leveldb::RandomAccessFile *raFile = nullptr;
    leveldb::Env::Default()->NewRandomAccessFile(filename, &raFile);
    status = leveldb::Table::Open(options, raFile, size, &table);

    std::cout<<"Querying"<<std::endl;
    auto queryStart = std::chrono::high_resolution_clock::now();
    leveldb::ReadOptions readOptions;
    size_t numQueries = 1000000;
    for (size_t i = 0; i < numQueries; i++) {
        leveldb::Slice keySlice(randomKeys[rand() % randomKeys.size()].data(), sizeof(uint64_t));
        status = table->InternalGet(readOptions, keySlice, nullptr, handleResult);
    }
    auto queryEnd = std::chrono::high_resolution_clock::now();
    long timeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();
    std::cout<<"RESULT"
            <<" objectSize="<<averageLength
            <<" method=leveldb_mmap"
            <<" numObjects="<<N
            <<" numQueries="<<numQueries
            <<" timeMs="<<timeMicroseconds/1000
            <<" queriesPerSecond="<<(double)numQueries*1000000.0/((double)timeMicroseconds)
            <<" perObject="<<(((double)timeMicroseconds/(double)numQueries)*1000)<<std::endl;
}

