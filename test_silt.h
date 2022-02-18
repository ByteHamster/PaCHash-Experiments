#include <fawnds_combi.h>
#include <fawnds_sf_ordered_trie.h>

#include <utility>
#include "fawnds.h"
#include "configuration.h"
#include "fawnds_factory.h"

class SiltComparisonItemBase : public StoreComparisonItem {
    public:
        std::vector<std::uint64_t> keys;
        fawn::FawnDS_Combi* store;

        SiltComparisonItemBase(std::string name, size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem(std::move(name), N, averageLength, numQueries) {
            keys = generateRandomKeys(N);

            system("rm -rf /tmp/silt-test");
            system("mkdir -p /tmp/silt-test");
            auto* config = new fawn::Configuration("../siltConfig.xml");
            config->SetStringValue("data-len", std::to_string(averageLength));
            char buf[1024];
            snprintf(buf, sizeof(buf), "%zu", N);
            config->SetStringValue("size", buf);
            store = dynamic_cast<fawn::FawnDS_Combi *>(fawn::FawnDS_Factory::New(config));
            fawn::FawnDS_Return res = store->Create();
            assert(res == fawn::FawnDS_Return::OK);
        }

        ~SiltComparisonItemBase() override {
            store->Destroy();
            delete store;
            system("rm -r /tmp/silt-test");
        }

        void construct() override {
            for (uint64_t key : keys) {
                fawn::ConstRefValue value(emptyValuePointer, averageLength);
                fawn::FawnDS_Return res = store->Put(fawn::ConstRefValue(&key), value);
                assert(res == fawn::FawnDS_Return::OK);
                //std::cout<<"Put("<<key<<") = "<<res<<std::endl;
            }
            store->Flush();
        }
};

class SiltComparisonItem : public SiltComparisonItemBase {
    public:
        SiltComparisonItem(size_t N, size_t averageLength, size_t numQueries) :
                SiltComparisonItemBase("silt", N, averageLength, numQueries) {
        }

        void query() override {
            size_t handled = 0;
            while (handled < numQueries) {
                fawn::Value valueRead;
                fawn::FawnDS_Return res = store->Get(fawn::ConstRefValue(&keys[rand() % keys.size()]), valueRead);
                assert(res == fawn::FawnDS_Return::OK);
                //std::cout<<"Get("<<key<<") = "<<res<<" "<<valueRead.str()<<std::endl;
                handled++;
            }
        }
};

class SiltComparisonItemSortedStoreBase : public StoreComparisonItem {
    public:
        fawn::FawnDS_SF_Ordered_Trie* sortedStore;
        std::vector<std::uint64_t> keys;

        SiltComparisonItemSortedStoreBase(std::string name, size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem(std::move(name), N, averageLength, numQueries) {
            keys = generateRandomKeys(N);

            system("rm -rf /tmp/silt-test-sorted");
            system("mkdir -p /tmp/silt-test-sorted");
            auto* config = new fawn::Configuration("../siltConfigSorted.xml");
            config->SetStringValue("data-len", std::to_string(averageLength));
            char buf[1024];
            snprintf(buf, sizeof(buf), "%zu", N);
            config->SetStringValue("size", buf);
            sortedStore = dynamic_cast<fawn::FawnDS_SF_Ordered_Trie *>(fawn::FawnDS_Factory::New(config));
            fawn::FawnDS_Return res = sortedStore->Create();
            assert(res == fawn::FawnDS_Return::OK);
        }

        ~SiltComparisonItemSortedStoreBase() override {
            sortedStore->Destroy();
            delete sortedStore;
            system("rm -rf /tmp/silt-test-sorted");
        }

        void construct() override {
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
            snprintf(buf, sizeof(buf), "%zu", averageLength);
            if (sorter_config->SetStringValue("data-len", buf) != 0)
                assert(false);

            if (sorter_config->CreateNodeAndAppend("size", ".") != 0)
                assert(false);
            snprintf(buf, sizeof(buf), "%zu", N);
            if (sorter_config->SetStringValue("size", buf) != 0)
                assert(false);

            if (sorter_config->CreateNodeAndAppend("temp-file", ".") != 0)
                assert(false);
            if (sorter_config->SetStringValue("temp-file", "/tmp/silt-test-sorted") != 0)
                assert(false);

            fawn::FawnDS* sorter = fawn::FawnDS_Factory::New(sorter_config);
            assert(sorter);
            fawn::FawnDS_Return res = sorter->Create();
            assert(res == fawn::OK);

            for (uint64_t key : keys) {
                fawn::ConstRefValue value(emptyValuePointer, averageLength);
                res = sorter->Put(fawn::ConstRefValue(&key), value);
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
            sorter->Destroy();
            delete sorter;
            sortedStore->Flush();
        }
};

class SiltComparisonItemSortedStore : public SiltComparisonItemSortedStoreBase {
    public:
        SiltComparisonItemSortedStore(size_t N, size_t averageLength, size_t numQueries) :
                SiltComparisonItemSortedStoreBase("silt_sorted", N, averageLength, numQueries) {
        }

        void query() override {
            size_t handled = 0;
            while (handled < numQueries) {
                fawn::Value valueRead;
                fawn::FawnDS_Return res = sortedStore->Get(fawn::ConstRefValue(&keys[rand() % keys.size()]), valueRead);
                assert(res == fawn::FawnDS_Return::OK);
                //std::cout<<"Get("<<key<<") = "<<res<<" "<<valueRead.str()<<std::endl;
                handled++;
            }
        }
};

class SiltComparisonItemSortedStoreMicro : public SiltComparisonItemSortedStoreBase {
    public:
        SiltComparisonItemSortedStoreMicro(size_t N, size_t averageLength, size_t numQueries) :
                SiltComparisonItemSortedStoreBase("silt_sorted_micro", N, averageLength, numQueries) {
        }

        void query() override {
            size_t handled = 0;
            while (handled < numQueries) {
                fawn::Value valueRead;
                fawn::FawnDS_Return res = sortedStore->GetIndexOnly(fawn::ConstRefValue(&keys[rand() % keys.size()]));
                assert(res == fawn::FawnDS_Return::KEY_DELETED || res == fawn::KEY_NOT_FOUND);
                //std::cout<<"Get("<<key<<") = "<<res<<" "<<valueRead.str()<<std::endl;
                handled++;
            }
        }
};
