#include <leveldb/env.h>
#include <leveldb/table.h>
#include "leveldb/table_builder.h"
#include "StoreComparisonItem.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include <memory>

class LevelDBComparisonItem : public StoreComparisonItem {
    public:
        std::vector<std::string> keys;
        leveldb::Options options;
        leveldb::DB *db = nullptr;
        const std::string filename = "/tmp/leveldb-test";

        LevelDBComparisonItem(size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem("leveldb", N, averageLength, numQueries) {
            keys = generateRandomStringKeys(N);
            options.block_size = 4096 - 150; // Headers etc. Ensures that pread calls are limited to <4096
            options.compression = leveldb::CompressionType::kNoCompression;
            options.create_if_missing = true;
            leveldb::DestroyDB(filename, options);
            leveldb::Status status = leveldb::DB::Open(options, filename, &db);
            assert(status.ok());
        }

        ~LevelDBComparisonItem() {
            leveldb::DestroyDB(filename, options);
        }

        void construct() override {
            leveldb::WriteBatch batch;
            const char *value = static_cast<const char *>(malloc(averageLength));
            for (std::string &key : keys) {
                leveldb::Slice keySlice(key.data(), sizeof(uint64_t));
                leveldb::Slice valueSlice = leveldb::Slice(value, averageLength);
                batch.Put(keySlice, valueSlice);
            }
            leveldb::WriteOptions writeOptions;
            writeOptions.sync = true;
            db->Write(writeOptions, &batch);
            // TODO: Compact?
        }

        void query() override {
            leveldb::ReadOptions readOptions;
            std::string result;
            for (size_t i = 0; i < numQueries; i++) {
                leveldb::Slice keySlice(keys[rand() % N].data(), sizeof(uint64_t));
                db->Get(readOptions, keySlice, &result);
            }
        }
};

class LevelDBSingleTableComparisonItem : public StoreComparisonItem {
    public:
        static void handleResult(void *arg, const leveldb::Slice &key, const leveldb::Slice &value) {
            //std::cout<<std::string(key.data(), key.size())<<": "<<std::string(value.data(), value.size())<<std::endl;
            (void) arg;
            (void) key;
            (void) value;
        }

        std::vector<std::string> keys;
        std::string filename = "/tmp/leveldb-test-single";
        size_t size = 0;
        leveldb::Options options;

        LevelDBSingleTableComparisonItem(size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem("leveldb_singletable", N, averageLength, numQueries) {
            keys = generateRandomStringKeys(N);
            options.block_size = 4096 - 150; // Headers etc. Ensures that pread calls are limited to <4096
            options.compression = leveldb::CompressionType::kNoCompression;
        }

        ~LevelDBSingleTableComparisonItem() {
            leveldb::Env::Default()->DeleteFile(filename);
        }

        void construct() override {
            std::sort(keys.begin(), keys.end());
            leveldb::WritableFile *file = nullptr;
            leveldb::Env::Default()->DeleteFile(filename);
            leveldb::Env::Default()->NewWritableFile(filename, &file);
            leveldb::TableBuilder tableBuilder(options, file);

            const char *value = static_cast<const char *>(malloc(averageLength));
            for (std::string &key : keys) {
                leveldb::Slice keySlice(key.data(), sizeof(uint64_t));
                leveldb::Slice valueSlice = leveldb::Slice(value, averageLength);
                tableBuilder.Add(keySlice, valueSlice);
            }
            leveldb::Status status = tableBuilder.Finish();
            file->Close();
            size = tableBuilder.FileSize();
        }

        void query() override {
            leveldb::Status status;

            leveldb::Table *table = nullptr;
            leveldb::RandomAccessFile *raFile = nullptr;
            leveldb::Env::Default()->NewRandomAccessFile(filename, &raFile);
            status = leveldb::Table::Open(options, raFile, size, &table);

            leveldb::ReadOptions readOptions;
            for (size_t i = 0; i < numQueries; i++) {
                leveldb::Slice keySlice(keys[rand() % N].data(), sizeof(uint64_t));
                status = table->InternalGet(readOptions, keySlice, nullptr, handleResult);
            }
        }
};
