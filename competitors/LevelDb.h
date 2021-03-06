#include <leveldb/env.h>
#include <leveldb/table.h>
#include <leveldb/table_builder.h>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <memory>
#include <utility>
#include "../StoreComparisonItem.h"

class LevelDBComparisonItem : public StoreComparisonItem {
    public:
        leveldb::Options options;
        leveldb::DB *db = nullptr;
        const std::string filename;

        explicit LevelDBComparisonItem(const BenchmarkConfig& benchmarkConfig)
                : StoreComparisonItem("leveldb", benchmarkConfig), filename(benchmarkConfig.basePath + "leveldb-test") {
            // Does not support direct IO
            options.compression = leveldb::CompressionType::kNoCompression;
            options.create_if_missing = true;
            leveldb::DestroyDB(filename, options);
            leveldb::Status status = leveldb::DB::Open(options, filename, &db);
            if (!status.ok()) {
                std::cerr<<status.ToString()<<std::endl;
                exit(1);
            }
        }

        bool supportsVariableSize() override {
            return true;
        }

        ~LevelDBComparisonItem() override {
            delete db;
            leveldb::DestroyDB(filename, options);
        }

        size_t externalSpaceUsage() override {
            return directorySize(filename.c_str());
        }

        void construct(std::vector<Object> &objects) override {
            leveldb::WriteBatch batch;
            for (Object &object : objects) {
                batch.Put(object.key, leveldb::Slice(emptyValuePointer, object.length));
            }
            leveldb::WriteOptions writeOptions;
            writeOptions.sync = true;
            db->Write(writeOptions, &batch);
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            leveldb::ReadOptions readOptions;
            std::string result;
            leveldb::Status status;
            for (size_t i = 0; i < benchmarkConfig.numQueries; i++) {
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

        const std::string filename;
        size_t size = 0;
        leveldb::Options options;
        leveldb::Table *table = nullptr;
        leveldb::RandomAccessFile *raFile = nullptr;
        leveldb::ReadOptions readOptions;

        LevelDBSingleTableComparisonItemBase(std::string name, const BenchmarkConfig& benchmarkConfig)
                : StoreComparisonItem(std::move(name), benchmarkConfig),
                    filename(benchmarkConfig.basePath + "leveldb-test-single") {
            options.compression = leveldb::CompressionType::kNoCompression;
        }

        bool supportsVariableSize() override {
            return true;
        }

        ~LevelDBSingleTableComparisonItemBase() override {
            delete raFile;
            delete table;
            leveldb::Env::Default()->DeleteFile(filename);
        }

        size_t externalSpaceUsage() override {
            return fileSize(filename.c_str());
        }

        void construct(std::vector<Object> &objects) override {
            std::sort(objects.begin(), objects.end(), [](const Object &o1, const Object &o2) {
                return o1.key < o2.key;
            });
            leveldb::WritableFile *file = nullptr;
            leveldb::Env::Default()->DeleteFile(filename);
            leveldb::Env::Default()->NewWritableFile(filename, &file);
            leveldb::TableBuilder tableBuilder(options, file);
            for (Object &object : objects) {
                tableBuilder.Add(object.key, leveldb::Slice(emptyValuePointer, object.length));
            }
            leveldb::Status status = tableBuilder.Finish();
            file->Close();
            delete file;
            size = tableBuilder.FileSize();
        }

        void afterConstruct() override {
            leveldb::Env::Default()->NewRandomAccessFile(filename, &raFile);
            leveldb::Status status = leveldb::Table::Open(options, raFile, size, &table);
            assert(status.ok());
        }
};

class LevelDBSingleTableComparisonItem : public LevelDBSingleTableComparisonItemBase {
    public:
        explicit LevelDBSingleTableComparisonItem(const BenchmarkConfig& benchmarkConfig)
                : LevelDBSingleTableComparisonItemBase("leveldb_singletable", benchmarkConfig) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            leveldb::Status status;
            for (size_t i = 0; i < benchmarkConfig.numQueries; i++) {
                status = table->InternalGet(readOptions, keysQueryOrder[i], nullptr, handleResult);
                DO_NOT_OPTIMIZE(status);
                assert(status.ok());
            }
        }
};

class LevelDBSingleTableMicroIndexComparisonItem : public LevelDBSingleTableComparisonItemBase {
    public:
        explicit LevelDBSingleTableMicroIndexComparisonItem(const BenchmarkConfig& benchmarkConfig)
            : LevelDBSingleTableComparisonItemBase("leveldb_singletable_index_only", benchmarkConfig) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            leveldb::Status status;
            for (size_t i = 0; i < benchmarkConfig.numQueries; i++) {
                status = table->InternalGetIndexOnly(readOptions, keysQueryOrder[i]);
                DO_NOT_OPTIMIZE(status);
            }
        }
};
