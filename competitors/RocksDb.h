#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/utilities/memory_util.h>
#include <iostream>
#include <random>
#include "../StoreComparisonItem.h"

class RocksDBComparisonItem : public StoreComparisonItem {
    public:
        rocksdb::DB* db = nullptr;
        std::string filePath = "/data02/hplehmann/rocksdb-test";
        rocksdb::Options options;

        RocksDBComparisonItem(size_t N, size_t numQueries, bool directIo) :
                StoreComparisonItem(directIo ? "rocksdb_direct" : "rocksdb", N, numQueries) {
            this->directIo = directIo;
            options.create_if_missing = true;
            rocksdb::BlockBasedTableOptions table_options;
            table_options.no_block_cache = true;
            //table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
            options.table_factory.reset(NewBlockBasedTableFactory(table_options));
            options.use_direct_reads = directIo;
            options.allow_mmap_reads = false;
            options.env = rocksdb::Env::Default();
            std::vector<rocksdb::ColumnFamilyDescriptor> x;
            rocksdb::DestroyDB(filePath, options, x);

            rocksdb::Status status = rocksdb::DB::Open(options, filePath, &db);
            if (!status.ok()) {
                std::cout<<status.ToString()<<std::endl;
                return;
            }
        }

        bool supportsVariableSize() override {
            return true;
        }

        ~RocksDBComparisonItem() override {
            /*std::map<rocksdb::MemoryUtil::UsageType, uint64_t> usage;
            std::vector<rocksdb::DB *> dbs;
            dbs.push_back(db);
            std::unordered_set<const rocksdb::Cache*> cache_set;
            rocksdb::MemoryUtil::GetApproximateMemoryUsageByType(dbs, cache_set, &usage);
            std::cout<<"Total memory: "<<usage[rocksdb::MemoryUtil::UsageType::kMemTableTotal]<<std::endl;*/
            db->Close();
            delete db;
            std::vector<rocksdb::ColumnFamilyDescriptor> x;
            rocksdb::DestroyDB(filePath, options, x);
        }

        size_t externalSpaceUsage() override {
            return directorySize(filePath.c_str());
        }

        void construct(std::vector<Object> &objects) override {
            rocksdb::WriteOptions writeOptions;
            writeOptions.disableWAL = true;
            rocksdb::WriteBatch writeBatch;
            for (Object &object : objects) {
                writeBatch.Put(object.key, rocksdb::Slice(emptyValuePointer, object.length));
            }
            db->Write(writeOptions, &writeBatch);
            db->Flush(rocksdb::FlushOptions());
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
