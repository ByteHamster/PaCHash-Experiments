#include "rocksdb/db.h"
#include "rocksdb/table.h"
#include "StoreComparisonItem.h"
#include <iostream>
#include <random>

class RocksDBComparisonItem : public StoreComparisonItem {
    public:
        std::vector<std::uint64_t> keys;
        rocksdb::DB* db = nullptr;
        std::string filePath = "/tmp/rocksdb-test";
        rocksdb::Options options;

        RocksDBComparisonItem(size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem("rocksdb", N, averageLength, numQueries) {
            keys = generateRandomKeys(N);

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

        ~RocksDBComparisonItem() {
            std::vector<rocksdb::ColumnFamilyDescriptor> x;
            rocksdb::DestroyDB(filePath, options, x);
        }

        void construct() override {
            rocksdb::WriteOptions writeOptions;
            writeOptions.disableWAL = true;
            const char *value = static_cast<const char *>(malloc(averageLength));
            for (uint64_t keyUint : keys) {
                rocksdb::Slice key(reinterpret_cast<const char *>(&keyUint), sizeof(uint64_t));
                db->Put(writeOptions, key, rocksdb::Slice(value, averageLength));
            }
            db->Flush(rocksdb::FlushOptions());
            db->Close();
            rocksdb::DB::Open(options, filePath, &db);
        }

        void query() override {
            size_t batchSize = 64;
            size_t numBatches = numQueries/batchSize;
            std::vector<rocksdb::PinnableSlice> values(batchSize);
            std::vector<uint64_t> queryKeys(batchSize);
            std::vector<rocksdb::Slice> querySlices(batchSize);
            std::vector<rocksdb::Status> statuses(batchSize);

            for (size_t i = 0; i < numBatches; i++) {
                for (size_t k = 0; k < batchSize; k++) {
                    queryKeys.at(k) = keys.at(rand() % N);
                    querySlices.at(k) = rocksdb::Slice(reinterpret_cast<const char *>(queryKeys.data() + k), sizeof(uint64_t));
                }
                db->MultiGet(rocksdb::ReadOptions(),
                             db->DefaultColumnFamily(),
                             batchSize,
                             querySlices.data(),
                             values.data(),
                             statuses.data());

                for (size_t k = 0; k < batchSize; k++) {
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
        }
};
