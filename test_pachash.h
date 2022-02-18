#include <PaCHashObjectStore.h>

#include <utility>
#include "StoreComparisonItem.h"

class PaCHashComparisonItemBase : public StoreComparisonItem {
    public:
        const char* filename = "/tmp/pachash-test";
        pachash::PaCHashObjectStore<8> objectStore;

        PaCHashComparisonItemBase(std::string method, size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem(std::move(method), N, averageLength, numQueries),
                objectStore(1, filename, 0) {
        }

        ~PaCHashComparisonItemBase() override {
            unlink(filename);
        }

        void construct() override {
            auto hashFunction = [](const std::string &key) -> pachash::StoreConfig::key_t {
                return pachash::MurmurHash64(key);
            };
            auto lengthEx = [&](const std::string &key) -> size_t {
                (void) key;
                return averageLength;
            };
            auto valueEx = [&](const std::string &key) -> const char * {
                (void) key;
                return emptyValuePointer;
            };
            objectStore.writeToFile(keys.begin(), keys.end(), hashFunction, lengthEx, valueEx);
            objectStore.reloadFromFile();
        }
};

class PaCHashMicroIndexComparisonItem : public PaCHashComparisonItemBase {
    public:
        PaCHashMicroIndexComparisonItem(size_t N, size_t averageLength, size_t numQueries)
                : PaCHashComparisonItemBase("pachash_micro_index", N, averageLength, numQueries) {
        }

        void query() override {
            std::tuple<size_t, size_t> accessDetails;
            for (size_t handled = 0; handled < numQueries; handled++) {
                pachash::StoreConfig::key_t key = pachash::MurmurHash64(keys[rand() % N]);
                objectStore.findBlocksToAccess(&accessDetails, key);
            }
        }
};

class PaCHashComparisonItem : public PaCHashComparisonItemBase {
    public:
        using ObjectStoreView = pachash::ObjectStoreView<pachash::PaCHashObjectStore<8>, pachash::UringIO>;
        ObjectStoreView *objectStoreView = nullptr;
        std::vector<pachash::QueryHandle*> queryHandles;
        size_t depth = 128;

        PaCHashComparisonItem(size_t N, size_t averageLength, size_t numQueries)
            : PaCHashComparisonItemBase("pachash", N, averageLength, numQueries) {
        }

        void beforeQuery() override {
            objectStoreView = new ObjectStoreView(objectStore, 0, depth);
            for (size_t i = 0; i < depth; i++) {
                queryHandles.emplace_back(new pachash::QueryHandle(objectStore));
            }
        }

        void query() override {
            size_t handled = 0;
            // Fill in-flight queue
            for (size_t i = 0; i < depth; i++) {
                queryHandles[i]->prepare(keys[rand() % N]);
                objectStoreView->enqueueQuery(queryHandles[i]);
                handled++;
            }
            objectStoreView->submit();

            // Submit new queries as old ones complete
            while (handled < numQueries) {
                pachash::QueryHandle *handle = objectStoreView->awaitAny();
                do {
                    assert(handle->resultPtr != nullptr);
                    handle->prepare(keys[rand() % N]);
                    objectStoreView->enqueueQuery(handle);
                    handle = objectStoreView->peekAny();
                    handled++;
                } while (handle != nullptr);
                objectStoreView->submit();
            }

            // Collect remaining in-flight queries
            for (size_t i = 0; i < depth; i++) {
                pachash::QueryHandle *handle = objectStoreView->awaitAny();
                assert(handle->resultPtr != nullptr);
                handled++;
            }
        }

        void afterQuery() override {
            for (pachash::QueryHandle *handle : queryHandles) {
                delete handle;
            }
        }
};
