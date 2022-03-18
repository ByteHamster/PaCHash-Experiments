#include "rocksdb/db.h"
#include "rocksdb/table.h"
#include "StoreComparisonItem.h"
#include <iostream>
#include <random>

class RocksDBComparisonItem : public StoreComparisonItem {
    public:
        rocksdb::DB* db = nullptr;
        std::string filePath = "/tmp/rocksdb-test";
        rocksdb::Options options;

        RocksDBComparisonItem(size_t N, size_t objectSize, size_t numQueries) :
                StoreComparisonItem("rocksdb", N, objectSize, numQueries) {
            options.create_if_missing = true;
            rocksdb::BlockBasedTableOptions table_options;
            table_options.no_block_cache = true;
            options.table_factory.reset(NewBlockBasedTableFactory(table_options));
            options.use_direct_reads = false;
            options.allow_mmap_reads = true;
            options.env = rocksdb::Env::Default();

            std::vector<rocksdb::ColumnFamilyDescriptor> x;
            rocksdb::DestroyDB(filePath, options, x);

            rocksdb::Status status = rocksdb::DB::Open(options, filePath, &db);
            if (!status.ok()) {
                std::cout<<status.ToString()<<std::endl;
                return;
            }
        }

        ~RocksDBComparisonItem() override {
            db->Flush(rocksdb::FlushOptions());
            db->Close();
            delete db;
            std::vector<rocksdb::ColumnFamilyDescriptor> x;
            rocksdb::DestroyDB(filePath, options, x);
            usleep(2000 * 1000); // Makes other methods segfault otherwise
        }

        void construct(std::vector<std::string> &keys) override {
            rocksdb::WriteOptions writeOptions;
            writeOptions.disableWAL = true;
            rocksdb::WriteBatch writeBatch;
            for (std::string &key : keys) {
                writeBatch.Put(key, rocksdb::Slice(emptyValuePointer, objectSize));
            }
            db->Write(writeOptions, &writeBatch);
            db->Flush(rocksdb::FlushOptions());
            db->Close();
            delete db;
            rocksdb::DB::Open(options, filePath, &db);
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            size_t batchSize = 64;
            std::vector<rocksdb::PinnableSlice> values(batchSize);
            std::vector<rocksdb::Slice> querySlices(batchSize);
            std::vector<rocksdb::Status> statuses(batchSize);

            size_t handled = 0;
            while (handled < numQueries) {
                for (size_t k = 0; k < batchSize; k++) {
                    querySlices.at(k) = keysQueryOrder[handled];
                    handled++;
                }
                db->MultiGet(rocksdb::ReadOptions(),
                             db->DefaultColumnFamily(),
                             batchSize,
                             querySlices.data(),
                             values.data(),
                             statuses.data());
                DO_NOT_OPTIMIZE(statuses);
            }
        }
};
