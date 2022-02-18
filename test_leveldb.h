#include <leveldb/env.h>
#include <leveldb/table.h>
#include "leveldb/table_builder.h"
#include "StoreComparisonItem.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include <memory>

class LevelDBComparisonItem : public StoreComparisonItem {
    public:
        leveldb::Options options;
        leveldb::DB *db = nullptr;
        const std::string filename = "/tmp/leveldb-test";

        LevelDBComparisonItem(size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem("leveldb", N, averageLength, numQueries) {
            options.block_size = 4096 - 150; // Headers etc. Ensures that pread calls are limited to <4096
            options.compression = leveldb::CompressionType::kNoCompression;
            options.create_if_missing = true;
            leveldb::DestroyDB(filename, options);
            leveldb::Status status = leveldb::DB::Open(options, filename, &db);
            if (!status.ok()) {
                std::cerr<<status.ToString()<<std::endl;
                exit(1);
            }
        }

        ~LevelDBComparisonItem() override {
            delete db;
            leveldb::DestroyDB(filename, options);
        }

        void construct() override {
            leveldb::WriteBatch batch;
            leveldb::Slice valueSlice = leveldb::Slice(emptyValuePointer, averageLength);
            for (std::string &key : keys) {
                batch.Put(key, valueSlice);
            }
            leveldb::WriteOptions writeOptions;
            writeOptions.sync = true;
            db->Write(writeOptions, &batch);
            delete db;
            leveldb::Status status = leveldb::DB::Open(options, filename, &db);
        }

        void query() override {
            leveldb::ReadOptions readOptions;
            std::string result;
            leveldb::Status status;
            for (size_t i = 0; i < numQueries; i++) {
                status = db->Get(readOptions, keys[rand() % N], &result);
                assert(status.ok());
            }
        }
};

class LevelDBSingleTableComparisonItemBase : public StoreComparisonItem {
    public:
        static void handleResult(void *arg, const leveldb::Slice &key, const leveldb::Slice &value) {
            //std::cout<<std::string(key.data(), key.size())<<": "<<std::string(value.data(), value.size())<<std::endl;
            (void) arg;
            (void) key;
            (void) value;
        }

        std::string filename = "/tmp/leveldb-test-single";
        size_t size = 0;
        leveldb::Options options;

        LevelDBSingleTableComparisonItemBase(std::string name, size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem(name, N, averageLength, numQueries) {
            options.block_size = 4096 - 150; // Headers etc. Ensures that pread calls are limited to <4096
            options.compression = leveldb::CompressionType::kNoCompression;
        }

        ~LevelDBSingleTableComparisonItemBase() override {
            leveldb::Env::Default()->DeleteFile(filename);
        }

        void construct() override {
            std::sort(keys.begin(), keys.end());
            leveldb::WritableFile *file = nullptr;
            leveldb::Env::Default()->DeleteFile(filename);
            leveldb::Env::Default()->NewWritableFile(filename, &file);
            leveldb::TableBuilder tableBuilder(options, file);
            leveldb::Slice valueSlice = leveldb::Slice(emptyValuePointer, averageLength);
            for (std::string &key : keys) {
                tableBuilder.Add(key, valueSlice);
            }
            leveldb::Status status = tableBuilder.Finish();
            file->Close();
            size = tableBuilder.FileSize();
        }
};

class LevelDBSingleTableComparisonItem : public LevelDBSingleTableComparisonItemBase {
    public:
        LevelDBSingleTableComparisonItem(size_t N, size_t averageLength, size_t numQueries) :
                LevelDBSingleTableComparisonItemBase("leveldb_singletable", N, averageLength, numQueries) {
        }

        void query() override {
            leveldb::Status status;

            leveldb::Table *table = nullptr;
            leveldb::RandomAccessFile *raFile = nullptr;
            leveldb::Env::Default()->NewRandomAccessFile(filename, &raFile);
            status = leveldb::Table::Open(options, raFile, size, &table);

            leveldb::ReadOptions readOptions;
            for (size_t i = 0; i < numQueries; i++) {
                status = table->InternalGet(readOptions, keys[rand() % N], nullptr, handleResult);
                assert(status.ok());
            }
        }
};

class LevelDBSingleTableMicroIndexComparisonItem : public LevelDBSingleTableComparisonItemBase {
    public:
        LevelDBSingleTableMicroIndexComparisonItem(size_t N, size_t averageLength, size_t numQueries) :
                LevelDBSingleTableComparisonItemBase("leveldb_singletable_index_only", N, averageLength, numQueries) {
        }

        void query() override {
            leveldb::Status status;

            leveldb::Table *table = nullptr;
            leveldb::RandomAccessFile *raFile = nullptr;
            leveldb::Env::Default()->NewRandomAccessFile(filename, &raFile);
            status = leveldb::Table::Open(options, raFile, size, &table);

            leveldb::ReadOptions readOptions;
            for (size_t i = 0; i < numQueries; i++) {
                table->InternalGetIndexOnly(readOptions, keys[rand() % N]);
            }
        }
};
