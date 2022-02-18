#include <PaCHashObjectStore.h>

#include <utility>
#include "StoreComparisonItem.h"

class PaCHashComparisonItemBase : public StoreComparisonItem {
    public:
        const char* filename = "/tmp/pachash-test";
        pachash::PaCHashObjectStore<8> objectStore;
        std::vector<std::uint64_t> keys;

        PaCHashComparisonItemBase(std::string method, size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem(std::move(method), N, averageLength, numQueries),
                objectStore(1, filename, 0) {
            keys = generateRandomKeys(N);
        }

        ~PaCHashComparisonItemBase() override {
            unlink(filename);
        }

        void construct() override {
            auto hashFunction = [](const std::uint64_t &key) -> pachash::StoreConfig::key_t {
                return key;
            };
            auto lengthEx = [&](const std::uint64_t &key) -> size_t {
                (void) key;
                return averageLength;
            };
            auto valueEx = [&](const std::uint64_t &key) -> const char * {
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
                objectStore.findBlocksToAccess(&accessDetails, keys[rand() % N]);
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
                queryHandles[i]->key = keys[rand() % N];
                objectStoreView->enqueueQuery(queryHandles[i]);
                handled++;
            }
            objectStoreView->submit();

            // Submit new queries as old ones complete
            while (handled < numQueries) {
                pachash::QueryHandle *handle = objectStoreView->awaitAny();
                do {
                    if (handle->resultPtr == nullptr) {
                        throw std::logic_error("Did not find item");
                    }
                    handle->key = keys[rand() % N];
                    objectStoreView->enqueueQuery(handle);
                    handle = objectStoreView->peekAny();
                    handled++;
                } while (handle != nullptr);
                objectStoreView->submit();
            }

            // Collect remaining in-flight queries
            for (size_t i = 0; i < depth; i++) {
                pachash::QueryHandle *handle = objectStoreView->awaitAny();
                if (handle->resultPtr == nullptr) {
                    throw std::logic_error("Did not find item");
                }
                handled++;
            }
        }

        void afterQuery() override {
            for (pachash::QueryHandle *handle : queryHandles) {
                delete handle;
            }
        }
};
