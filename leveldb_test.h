#include <leveldb/env.h>
#include <leveldb/table.h>
#include "leveldb/table_builder.h"
#include <memory>
#include "external/elias-fano/benchmark/RandomObjectProvider.h"

static std::vector<std::string> generateRandomStringKeys(size_t N) {
    uint64_t seed = std::random_device{}();
    std::cout<<"# Seed for input keys: "<<seed<<std::endl;
    std::mt19937_64 generator(seed);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    std::vector<std::string> keys;
    keys.reserve(N);
    for (size_t i = 0; i < N; i++) {
        uint64_t key = dist(generator);
        keys.emplace_back(std::string((char *)&key, sizeof(uint64_t)));
    }
    return keys;
}

static std::vector<std::string> randomKeys;
static RandomObjectProvider randomObjectProvider(EQUAL_DISTRIBUTION, 256);

static void handleResult(void *arg, const leveldb::Slice &key, const leveldb::Slice &value) {
    //std::cout<<std::string(key.data(), key.size())<<": "<<std::string(value.data(), value.size())<<std::endl;
    uint64_t keyUint = *reinterpret_cast<const uint64_t *>(key.data());
    size_t length = randomObjectProvider.getLength(keyUint);
    assert(value.size() == length);
}


static void testLeveldb(int N) {
    randomKeys = generateRandomStringKeys(N);
    std::sort(randomKeys.begin(), randomKeys.end());

    std::string filename = "/tmp/test.leveldb";
    leveldb::Options options;
    options.compression = leveldb::CompressionType::kNoCompression;
    leveldb::WritableFile *file = nullptr;
    leveldb::Env::Default()->NewWritableFile(filename, &file);
    leveldb::TableBuilder tableBuilder(options, file);

    std::cout<<"Inserting"<<std::endl;
    for (std::string &key : randomKeys) {
        uint64_t keyUint = *reinterpret_cast<const uint64_t *>(key.data());
        leveldb::Slice keySlice(key.data(), sizeof(uint64_t));
        size_t length = randomObjectProvider.getLength(keyUint);
        const char *value = randomObjectProvider.getValue(keyUint);
        leveldb::Slice valueSlice = leveldb::Slice(value, length);
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
    size_t numQueries = 5000000;
    for (size_t i = 0; i < numQueries; i++) {
        leveldb::Slice keySlice(randomKeys[rand() % randomKeys.size()].data(), sizeof(uint64_t));
        status = table->InternalGet(readOptions, keySlice, nullptr, handleResult);
    }
    auto queryEnd = std::chrono::high_resolution_clock::now();
    long timeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();
    std::cout<<"Queries: "<<numQueries<<", ms: "<<timeMicroseconds/1000<<std::endl;
    std::cout<<"kQueries/s: "<<(double)numQueries*1000.0/((double)timeMicroseconds)<<std::endl;
    std::cout<<"ns per query (with cached IO): "<<(((double)timeMicroseconds/(double)numQueries)*1000)<<std::endl;
}

