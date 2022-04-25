#include <fawnds_combi.h>
#include <fawnds_sf_ordered_trie.h>
#include <utility>
#include <fawnds.h>
#include <configuration.h>
#include <fawnds_factory.h>
#include "../StoreComparisonItem.h"

/**
 * Even though SILT implements an Open() call, that call does not actually open an existing database.
 * It simply deletes all data and starts from scratch, which makes it effectively the same as Create().
 * Even the Open() calls of the internal stages either delete the database or crash.
 * That defeats the whole point of a database and makes SILT kind of useless...
 */
class SiltComparisonItemBase : public StoreComparisonItem {
    public:
        fawn::FawnDS_Combi* store;
        const std::string filename;

        SiltComparisonItemBase(std::string name, const BenchmarkConfig& benchmarkConfig, bool directIo)
                : StoreComparisonItem(std::move(name), benchmarkConfig),
                    filename(benchmarkConfig.basePath + "silt-test") {
            this->directIo = directIo;
            system(("rm -rf " + filename).c_str());
            system(("mkdir -p " + filename).c_str());
            auto* config = new fawn::Configuration("../competitors/siltConfig.xml");
            config->SetStringValue("data-len", std::to_string(benchmarkConfig.objectSize));
            char buf[1024];
            snprintf(buf, sizeof(buf), "%zu", benchmarkConfig.N);
            config->SetStringValue("size", buf);
            config->SetStringValue("/fawnds/store0/hashtable/use-buffered-io-only", directIo ? "0" : "1");
            config->SetStringValue("/fawnds/store0/datastore/use-buffered-io-only", directIo ? "0" : "1");
            config->SetStringValue("/fawnds/store1/hashtable/use-buffered-io-only", directIo ? "0" : "1");
            config->SetStringValue("/fawnds/store1/datastore/use-buffered-io-only", directIo ? "0" : "1");
            config->SetStringValue("/fawnds/store2/datastore/use-buffered-io-only", directIo ? "0" : "1");
            config->SetStringValue("/fawnds/store0/file", filename + "/front_file_header");
            config->SetStringValue("/fawnds/store0/hashtable/file", filename + "/front_hashtable");
            config->SetStringValue("/fawnds/store0/datastore/file", filename + "/front_datastore");
            config->SetStringValue("/fawnds/store1/file", filename + "/middle_file_header");
            config->SetStringValue("/fawnds/store1/hashtable/file", filename + "/middle_hashtable");
            config->SetStringValue("/fawnds/store1/datastore/file", filename + "/middle_datastore");
            config->SetStringValue("/fawnds/store2/datastore/file", filename + "/back_datastore");
            store = dynamic_cast<fawn::FawnDS_Combi *>(fawn::FawnDS_Factory::New(config));
            fawn::FawnDS_Return res = store->Create();
            assert(res == fawn::FawnDS_Return::OK);
        }

        bool supportsVariableSize() override {
            return false;
        }

        ~SiltComparisonItemBase() override {
            store->Close();
            store->Destroy();
            delete store;
            system(("rm -r " + filename).c_str());
        }

        size_t externalSpaceUsage() override {
            return directorySize(filename.c_str());
        }

        void construct(std::vector<Object> &objects) override {
            for (const Object &object : objects) {
                assert(object.length == benchmarkConfig.objectSize);
                fawn::ConstRefValue value(emptyValuePointer, benchmarkConfig.objectSize);
                fawn::FawnDS_Return res = store->Put(fawn::ConstRefValue(object.key), value);
                assert(res == fawn::FawnDS_Return::OK);
            }
            store->Flush();
        }
};

class SiltComparisonItem : public SiltComparisonItemBase {
    public:
        SiltComparisonItem(const BenchmarkConfig& benchmarkConfig, bool directIo)
                : SiltComparisonItemBase(directIo ? "silt_direct" : "silt", benchmarkConfig, directIo) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < benchmarkConfig.numQueries; i++) {
                fawn::Value valueRead;
                fawn::FawnDS_Return res = store->Get(fawn::ConstRefValue(keysQueryOrder[i]), valueRead);
                DO_NOT_OPTIMIZE(res);
                assert(res == fawn::FawnDS_Return::OK);
            }
        }
};

class SiltComparisonItemSortedStoreBase : public StoreComparisonItem {
    public:
        fawn::FawnDS_SF_Ordered_Trie* sortedStore;
        const std::string filename;
        fawn::FawnDS* sorter = nullptr;

        SiltComparisonItemSortedStoreBase(std::string name, const BenchmarkConfig& benchmarkConfig, bool directIo)
                : StoreComparisonItem(std::move(name), benchmarkConfig),
                    filename(benchmarkConfig.basePath + "silt-test-sorted") {
            this->directIo = directIo;
            system(("rm -rf " + filename).c_str());
            system(("mkdir -p " + filename).c_str());
            auto* config = new fawn::Configuration("../competitors/siltConfigSorted.xml");
            config->SetStringValue("data-len", std::to_string(benchmarkConfig.objectSize));
            char buf[1024];
            snprintf(buf, sizeof(buf), "%zu", benchmarkConfig.N);
            config->SetStringValue("size", buf);
            config->SetStringValue("/fawnds/datastore/use-buffered-io-only", directIo ? "0" : "1");
            config->SetStringValue("/fawnds/datastore/file", filename + "/store");
            sortedStore = dynamic_cast<fawn::FawnDS_SF_Ordered_Trie *>(fawn::FawnDS_Factory::New(config));
            fawn::FawnDS_Return res = sortedStore->Create();
            assert(res == fawn::FawnDS_Return::OK);
        }

        bool supportsVariableSize() override {
            return false;
        }

        ~SiltComparisonItemSortedStoreBase() override {
            sortedStore->Close();
            sortedStore->Destroy();
            delete sortedStore;
            system(("rm -rf " + filename).c_str());
        }

        size_t externalSpaceUsage() override {
            return directorySize(filename.c_str());
        }

        void beforeConstruct(std::vector<Object> &objects) override {
            (void) objects;
            auto *sorter_config = new fawn::Configuration();

            char buf[1024];

            if (sorter_config->CreateNodeAndAppend("type", ".") != 0)
                assert(false);
            if (sorter_config->SetStringValue("type", "sorter") != 0)
                assert(false);

            if (sorter_config->CreateNodeAndAppend("key-len", ".") != 0)
                assert(false);
            snprintf(buf, sizeof(buf), "%zu", 8ul);
            if (sorter_config->SetStringValue("key-len", buf) != 0)
                assert(false);

            if (sorter_config->CreateNodeAndAppend("data-len", ".") != 0)
                assert(false);
            snprintf(buf, sizeof(buf), "%zu", benchmarkConfig.objectSize);
            if (sorter_config->SetStringValue("data-len", buf) != 0)
                assert(false);

            if (sorter_config->CreateNodeAndAppend("size", ".") != 0)
                assert(false);
            snprintf(buf, sizeof(buf), "%zu", benchmarkConfig.N);
            if (sorter_config->SetStringValue("size", buf) != 0)
                assert(false);

            if (sorter_config->CreateNodeAndAppend("temp-file", ".") != 0)
                assert(false);
            if (sorter_config->SetStringValue("temp-file", filename) != 0)
                assert(false);

            sorter = fawn::FawnDS_Factory::New(sorter_config);
            assert(sorter);
            fawn::FawnDS_Return res = sorter->Create();
            assert(res == fawn::OK);
        }

        void construct(std::vector<Object> &objects) override {
            fawn::FawnDS_Return res;
            for (const Object &object : objects) {
                assert(object.length == benchmarkConfig.objectSize);
                fawn::ConstRefValue value(emptyValuePointer, benchmarkConfig.objectSize);
                res = sorter->Put(fawn::ConstRefValue(object.key), value);
                assert(res == fawn::FawnDS_Return::OK);
            }
            sorter->Flush();

            fawn::FawnDS_ConstIterator it_m = sorter->Enumerate();
            while (true) {
                if (it_m.IsEnd()) {
                    break;
                }
                fawn::FawnDS_Return ret = sortedStore->Put( it_m->key, it_m->data);
                assert(ret == fawn::OK);
                it_m++;
            }
            sortedStore->Flush();
        }

        void afterConstruct() override {
            sorter->Destroy();
            delete sorter;
        }
};

class SiltComparisonItemSortedStore : public SiltComparisonItemSortedStoreBase {
    public:
        SiltComparisonItemSortedStore(const BenchmarkConfig& benchmarkConfig, bool directIo)
                : SiltComparisonItemSortedStoreBase(
                    directIo ? "silt_sorted_direct" : "silt_sorted", benchmarkConfig, directIo) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < benchmarkConfig.numQueries; i++) {
                fawn::Value valueRead;
                fawn::FawnDS_Return res = sortedStore->Get(fawn::ConstRefValue(keysQueryOrder[i]), valueRead);
                DO_NOT_OPTIMIZE(res);
                assert(res == fawn::FawnDS_Return::OK);
                //std::cout<<"Get("<<key<<") = "<<res<<" "<<valueRead.str()<<std::endl;
            }
        }
};

class SiltComparisonItemSortedStoreMicro : public SiltComparisonItemSortedStoreBase {
    public:
        explicit SiltComparisonItemSortedStoreMicro(const BenchmarkConfig& benchmarkConfig)
                : SiltComparisonItemSortedStoreBase("silt_sorted_micro", benchmarkConfig, false) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < benchmarkConfig.numQueries; i++) {
                fawn::Value valueRead;
                size_t res = sortedStore->GetIndexOnly(fawn::ConstRefValue(keysQueryOrder[i]));
                DO_NOT_OPTIMIZE(res);
            }
        }
};
