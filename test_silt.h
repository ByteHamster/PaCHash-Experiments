#include <fawnds_combi.h>
#include <fawnds_sf_ordered_trie.h>

#include <utility>
#include "fawnds.h"
#include "configuration.h"
#include "fawnds_factory.h"

/**
 * Even though SILT implements an Open() call, that call does not actually open an existing database.
 * It simply deletes all data and starts from scratch, which makes it effectively the same as Create().
 * Even the Open() calls of the internal stages either delete the database or crash.
 * That defeats the whole point of a database and makes SILT kind of useless...
 */
class SiltComparisonItemBase : public StoreComparisonItem {
    public:
        fawn::FawnDS_Combi* store;
        std::string filename = "/data02/hplehmann/silt-test";

        SiltComparisonItemBase(std::string name, size_t N, size_t objectSize, size_t numQueries) :
                StoreComparisonItem(std::move(name), N, objectSize, numQueries) {
            system(("rm -rf " + filename).c_str());
            system(("mkdir -p " + filename).c_str());
            auto* config = new fawn::Configuration("../siltConfig.xml");
            config->SetStringValue("data-len", std::to_string(objectSize));
            char buf[1024];
            snprintf(buf, sizeof(buf), "%zu", N);
            config->SetStringValue("size", buf);
            config->SetStringValue("/fawnds/store0/hashtable/use-buffered-io-only", directIo ? "1" : "0");
            config->SetStringValue("/fawnds/store0/datastore/use-buffered-io-only", directIo ? "1" : "0");
            config->SetStringValue("/fawnds/store1/hashtable/use-buffered-io-only", directIo ? "1" : "0");
            config->SetStringValue("/fawnds/store1/datastore/use-buffered-io-only", directIo ? "1" : "0");
            config->SetStringValue("/fawnds/store2/datastore/use-buffered-io-only", directIo ? "1" : "0");
            // config->WriteConfigFile("../siltConfig.out.xml"); // For checking
            store = dynamic_cast<fawn::FawnDS_Combi *>(fawn::FawnDS_Factory::New(config));
            fawn::FawnDS_Return res = store->Create();
            assert(res == fawn::FawnDS_Return::OK);
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

        void construct(std::vector<std::string> &keys) override {
            for (std::string &key : keys) {
                fawn::ConstRefValue value(emptyValuePointer, objectSize);
                fawn::FawnDS_Return res = store->Put(fawn::ConstRefValue(key), value);
                assert(res == fawn::FawnDS_Return::OK);
            }
            store->Flush();
        }
};

class SiltComparisonItem : public SiltComparisonItemBase {
    public:
        SiltComparisonItem(size_t N, size_t objectSize, size_t numQueries) :
                SiltComparisonItemBase("silt", N, objectSize, numQueries) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < numQueries; i++) {
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
        std::string filename = "/data02/hplehmann/silt-test-sorted";
        fawn::FawnDS* sorter = nullptr;

        SiltComparisonItemSortedStoreBase(std::string name, size_t N, size_t objectSize, size_t numQueries) :
                StoreComparisonItem(std::move(name), N, objectSize, numQueries) {
            system(("rm -rf " + filename).c_str());
            system(("mkdir -p " + filename).c_str());
            auto* config = new fawn::Configuration("../siltConfigSorted.xml");
            config->SetStringValue("data-len", std::to_string(objectSize));
            char buf[1024];
            snprintf(buf, sizeof(buf), "%zu", N);
            config->SetStringValue("size", buf);
            config->SetStringValue("/fawnds/datastore/use-buffered-io-only", directIo ? "1" : "0");
            //config->WriteConfigFile("../siltConfigSorted.out.xml"); // For checking
            sortedStore = dynamic_cast<fawn::FawnDS_SF_Ordered_Trie *>(fawn::FawnDS_Factory::New(config));
            fawn::FawnDS_Return res = sortedStore->Create();
            assert(res == fawn::FawnDS_Return::OK);
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

        void beforeConstruct(std::vector<std::string> &keys) override {
            fawn::Configuration *sorter_config = new fawn::Configuration();

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
            snprintf(buf, sizeof(buf), "%zu", objectSize);
            if (sorter_config->SetStringValue("data-len", buf) != 0)
                assert(false);

            if (sorter_config->CreateNodeAndAppend("size", ".") != 0)
                assert(false);
            snprintf(buf, sizeof(buf), "%zu", N);
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

        void construct(std::vector<std::string> &keys) override {
            fawn::FawnDS_Return res;
            for (std::string &key : keys) {
                fawn::ConstRefValue value(emptyValuePointer, objectSize);
                res = sorter->Put(fawn::ConstRefValue(key), value);
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
        SiltComparisonItemSortedStore(size_t N, size_t objectSize, size_t numQueries) :
                SiltComparisonItemSortedStoreBase("silt_sorted", N, objectSize, numQueries) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < numQueries; i++) {
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
        SiltComparisonItemSortedStoreMicro(size_t N, size_t objectSize, size_t numQueries) :
                SiltComparisonItemSortedStoreBase("silt_sorted_micro", N, objectSize, numQueries) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < numQueries; i++) {
                fawn::Value valueRead;
                size_t res = sortedStore->GetIndexOnly(fawn::ConstRefValue(keysQueryOrder[i]));
                DO_NOT_OPTIMIZE(res);
            }
        }
};
