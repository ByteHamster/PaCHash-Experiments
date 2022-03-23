#include <SeparatorObjectStore.h>

#include <utility>
#include "StoreComparisonItem.h"

class SeparatorComparisonItemBase : public StoreComparisonItem {
    public:
        const char* filename = "/data02/hplehmann/separator-test";
        pachash::SeparatorObjectStore<6> objectStore;

        SeparatorComparisonItemBase(std::string method, size_t N, size_t objectSize, size_t numQueries)
            : StoreComparisonItem(std::move(method), N, objectSize, numQueries),
                objectStore(0.95, filename, 0) {
        }

        ~SeparatorComparisonItemBase() override {
            unlink(filename);
        }

        void construct(std::vector<std::string> &keys) override {
            auto hashFunction = [](const std::string &key) -> pachash::StoreConfig::key_t {
                return pachash::MurmurHash64(key);
            };
            auto lengthEx = [&](const std::string &key) -> size_t {
                (void) key;
                return objectSize;
            };
            auto valueEx = [&](const std::string &key) -> const char * {
                (void) key;
                return emptyValuePointer;
            };
            objectStore.writeToFile(keys.begin(), keys.end(), hashFunction, lengthEx, valueEx);
            objectStore.reloadFromFile();
        }
};

class SeparatorMicroIndexComparisonItem : public SeparatorComparisonItemBase {
    public:
        SeparatorMicroIndexComparisonItem(size_t N, size_t objectSize, size_t numQueries)
                : SeparatorComparisonItemBase("separator_micro_index", N, objectSize, numQueries) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < numQueries; i++) {
                pachash::StoreConfig::key_t key = pachash::MurmurHash64(keysQueryOrder[i]);
                size_t block = objectStore.findBlockToAccess(key);
                DO_NOT_OPTIMIZE(block);
            }
        }
};

class SeparatorComparisonItem : public SeparatorComparisonItemBase {
    public:
        using ObjectStoreView = pachash::ObjectStoreView<pachash::SeparatorObjectStore<6>, pachash::UringIO>;
        ObjectStoreView *objectStoreView = nullptr;
        std::vector<pachash::QueryHandle*> queryHandles;
        size_t depth = 128;

        SeparatorComparisonItem(size_t N, size_t objectSize, size_t numQueries)
            : SeparatorComparisonItemBase("separator", N, objectSize, numQueries) {
        }

        void beforeQuery() override {
            objectStoreView = new ObjectStoreView(objectStore, 0, depth);
            for (size_t i = 0; i < depth; i++) {
                queryHandles.emplace_back(new pachash::QueryHandle(objectStore));
            }
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            size_t handled = 0;
            // Fill in-flight queue
            for (size_t i = 0; i < depth; i++) {
                queryHandles[i]->prepare(keysQueryOrder[i]);
                objectStoreView->enqueueQuery(queryHandles[i]);
                handled++;
            }
            objectStoreView->submit();

            // Submit new queries as old ones complete
            while (handled < numQueries) {
                pachash::QueryHandle *handle = objectStoreView->awaitAny();
                do {
                    assert(handle->resultPtr != nullptr);
                    DO_NOT_OPTIMIZE(handle->resultPtr);
                    handle->prepare(keysQueryOrder[handled]);
                    objectStoreView->enqueueQuery(handle);
                    handle = objectStoreView->peekAny();
                    handled++;
                } while (handle != nullptr);
                objectStoreView->submit();
            }

            // Collect remaining in-flight queries
            for (size_t i = 0; i < depth; i++) {
                pachash::QueryHandle *handle = objectStoreView->awaitAny();
                DO_NOT_OPTIMIZE(handle->resultPtr);
                assert(handle->resultPtr != nullptr);
                handled++;
            }
        }

        void afterQuery() override {
            delete objectStoreView;
            for (pachash::QueryHandle *handle : queryHandles) {
                delete handle;
            }
        }
};
