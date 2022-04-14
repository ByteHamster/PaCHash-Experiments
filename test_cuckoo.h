#include <ParallelCuckooObjectStore.h>

#include <utility>

class CuckooComparisonItemBase : public StoreComparisonItem {
    public:
        const char* filename = "/data02/hplehmann/cuckoo-test";
        pachash::ParallelCuckooObjectStore objectStore;

        CuckooComparisonItemBase(std::string method, size_t N, size_t numQueries, bool directIo)
            : StoreComparisonItem(std::move(method), N, numQueries),
                objectStore(0.95, filename, directIo ? O_DIRECT : 0) {
            this->directIo = directIo;
        }

        ~CuckooComparisonItemBase() override {
            unlink(filename);
        }

        size_t externalSpaceUsage() override {
            return fileSize(filename);
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
            objectStore.buildIndex();
        }
};

class CuckooComparisonItem : public CuckooComparisonItemBase {
    public:
        using ObjectStoreView = pachash::ObjectStoreView<pachash::ParallelCuckooObjectStore, pachash::UringIO>;
        ObjectStoreView *objectStoreView = nullptr;
        std::vector<pachash::QueryHandle*> queryHandles;
        size_t depth = 128;

        CuckooComparisonItem(size_t N, size_t numQueries, bool directIo)
            : CuckooComparisonItemBase(directIo ? "cuckoo_direct" : "cuckoo", N, numQueries, directIo) {
        }

        void beforeQuery() override {
            objectStoreView = new ObjectStoreView(objectStore, directIo ? O_DIRECT : 0, depth);
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
