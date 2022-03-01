#include <leveldb/env.h>
#include <leveldb/table.h>
#include "leveldb/table_builder.h"
#include "StoreComparisonItem.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include <memory>
#include <utility>

class LevelDBComparisonItem : public StoreComparisonItem {
    public:
        leveldb::Options options;
        leveldb::DB *db = nullptr;
        const std::string filename = "/tmp/leveldb-test";

        LevelDBComparisonItem(size_t N, size_t objectSize, size_t numQueries) :
                StoreComparisonItem("leveldb", N, objectSize, numQueries) {
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

        void construct(std::vector<std::string> &keys) override {
            leveldb::WriteBatch batch;
            leveldb::Slice valueSlice = leveldb::Slice(emptyValuePointer, objectSize);
            for (std::string &key : keys) {
                batch.Put(key, valueSlice);
            }
            leveldb::WriteOptions writeOptions;
            writeOptions.sync = true;
            db->Write(writeOptions, &batch);
            delete db;
            leveldb::Status status = leveldb::DB::Open(options, filename, &db);
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            leveldb::ReadOptions readOptions;
            std::string result;
            leveldb::Status status;
            for (size_t i = 0; i < numQueries; i++) {
                status = db->Get(readOptions, keysQueryOrder[i], &result);
                DO_NOT_OPTIMIZE(status);
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

        LevelDBSingleTableComparisonItemBase(std::string name, size_t N, size_t objectSize, size_t numQueries) :
                StoreComparisonItem(std::move(name), N, objectSize, numQueries) {
            options.block_size = 4096 - 150; // Headers etc. Ensures that pread calls are limited to <4096
            options.compression = leveldb::CompressionType::kNoCompression;
        }

        ~LevelDBSingleTableComparisonItemBase() override {
            leveldb::Env::Default()->DeleteFile(filename);
        }

        void construct(std::vector<std::string> &keys) override {
            std::sort(keys.begin(), keys.end());
            leveldb::WritableFile *file = nullptr;
            leveldb::Env::Default()->DeleteFile(filename);
            leveldb::Env::Default()->NewWritableFile(filename, &file);
            leveldb::TableBuilder tableBuilder(options, file);
            leveldb::Slice valueSlice = leveldb::Slice(emptyValuePointer, objectSize);
            for (std::string &key : keys) {
                tableBuilder.Add(key, valueSlice);
            }
            leveldb::Status status = tableBuilder.Finish();
            file->Close();
            delete file;
            size = tableBuilder.FileSize();
        }
};

class LevelDBSingleTableComparisonItem : public LevelDBSingleTableComparisonItemBase {
    public:
        LevelDBSingleTableComparisonItem(size_t N, size_t objectSize, size_t numQueries)
            : LevelDBSingleTableComparisonItemBase("leveldb_singletable", N, objectSize, numQueries) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            leveldb::Status status;

            leveldb::Table *table = nullptr;
            leveldb::RandomAccessFile *raFile = nullptr;
            leveldb::Env::Default()->NewRandomAccessFile(filename, &raFile);
            status = leveldb::Table::Open(options, raFile, size, &table);

            leveldb::ReadOptions readOptions;
            for (size_t i = 0; i < numQueries; i++) {
                status = table->InternalGet(readOptions, keysQueryOrder[i], nullptr, handleResult);
                DO_NOT_OPTIMIZE(status);
                assert(status.ok());
            }
            delete raFile;
            delete table;
        }
};

class LevelDBSingleTableMicroIndexComparisonItem : public LevelDBSingleTableComparisonItemBase {
    public:
        LevelDBSingleTableMicroIndexComparisonItem(size_t N, size_t objectSize, size_t numQueries)
            : LevelDBSingleTableComparisonItemBase("leveldb_singletable_index_only", N, objectSize, numQueries) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            leveldb::Status status;

            leveldb::Table *table = nullptr;
            leveldb::RandomAccessFile *raFile = nullptr;
            leveldb::Env::Default()->NewRandomAccessFile(filename, &raFile);
            status = leveldb::Table::Open(options, raFile, size, &table);

            leveldb::ReadOptions readOptions;
            for (size_t i = 0; i < numQueries; i++) {
                status = table->InternalGetIndexOnly(readOptions, keysQueryOrder[i]);
                DO_NOT_OPTIMIZE(status);
            }
            delete table;
            delete raFile;
        }
};
