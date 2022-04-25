#include <PaCHashObjectStore.h>
#include <utility>
#include "../StoreComparisonItem.h"

class PaCHashComparisonItemBase : public StoreComparisonItem {
    public:
        const char* filename = "/data02/hplehmann/pachash-test";
        pachash::PaCHashObjectStore<8> objectStore;

        PaCHashComparisonItemBase(std::string method, size_t N, size_t numQueries, bool directIo)
            : StoreComparisonItem(std::move(method), N, numQueries),
                objectStore(1, filename, directIo ? O_DIRECT : 0) {
            this->directIo = directIo;
        }

        bool supportsVariableSize() override {
            return true;
        }

        ~PaCHashComparisonItemBase() override {
            unlink(filename);
        }

        size_t externalSpaceUsage() override {
            return fileSize(filename);
        }

        void construct(std::vector<Object> &objects) override {
            auto hashFunction = [](const Object &object) -> pachash::StoreConfig::key_t {
                return pachash::MurmurHash64(object.key);
            };
            auto lengthEx = [&](const Object &object) -> size_t {
                return object.length;
            };
            auto valueEx = [&](const Object &object) -> const char * {
                (void) object;
                return emptyValuePointer;
            };
            objectStore.writeToFile(objects.begin(), objects.end(), hashFunction, lengthEx, valueEx);
            objectStore.buildIndex();
        }
};

class PaCHashMicroIndexComparisonItem : public PaCHashComparisonItemBase {
    public:
        PaCHashMicroIndexComparisonItem(size_t N, size_t numQueries)
                : PaCHashComparisonItemBase("pachash_micro_index", N, numQueries, false) {
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            std::tuple<size_t, size_t> accessDetails;
            for (size_t i = 0; i < numQueries; i++) {
                pachash::StoreConfig::key_t key = pachash::MurmurHash64(keysQueryOrder[i]);
                objectStore.index->locate(objectStore.key2bin(key), accessDetails);
                DO_NOT_OPTIMIZE(accessDetails);
            }
        }
};

class PaCHashComparisonItem : public PaCHashComparisonItemBase {
    public:
        using ObjectStoreView = pachash::ObjectStoreView<pachash::PaCHashObjectStore<8>, pachash::UringIO>;
        ObjectStoreView *objectStoreView = nullptr;
        std::vector<pachash::QueryHandle*> queryHandles;
        size_t depth = 128;

        PaCHashComparisonItem(size_t N, size_t numQueries, bool directIo)
            : PaCHashComparisonItemBase(directIo ? "pachash_direct" : "pachash", N, numQueries, directIo) {
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
            }
        }

        void afterQuery() override {
            delete objectStoreView;
            for (pachash::QueryHandle *handle : queryHandles) {
                delete handle;
            }
        }
};
