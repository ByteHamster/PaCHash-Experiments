#include <ParallelCuckooObjectStore.h>
#include <utility>
#include "../StoreComparisonItem.h"

class CuckooComparisonItemBase : public StoreComparisonItem {
    public:
        const std::string filename;
        pachash::ParallelCuckooObjectStore objectStore;

        CuckooComparisonItemBase(std::string method, const BenchmarkConfig& benchmarkConfig, bool directIo)
                : StoreComparisonItem(std::move(method), benchmarkConfig),
                    filename(benchmarkConfig.basePath + "cuckoo-test"),
                    objectStore(0.95, filename.c_str(), directIo ? O_DIRECT : 0) {
            this->directIo = directIo;
        }

        bool supportsVariableSize() override {
            return true;
        }

        ~CuckooComparisonItemBase() override {
            unlink(filename.c_str());
        }

        size_t externalSpaceUsage() override {
            return fileSize(filename.c_str());
        }

        void construct(std::vector<Object> &objects) override {
            auto hashFunction = [](const Object &object) -> pachash::StoreConfig::key_t {
                return util::MurmurHash64(object.key);
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

class CuckooComparisonItem : public CuckooComparisonItemBase {
    public:
        using ObjectStoreView = pachash::ObjectStoreView<pachash::ParallelCuckooObjectStore, pachash::UringIO>;
        ObjectStoreView *objectStoreView = nullptr;
        std::vector<pachash::QueryHandle*> queryHandles;
        size_t depth = 128;

        CuckooComparisonItem(const BenchmarkConfig& benchmarkConfig, bool directIo)
                : CuckooComparisonItemBase(directIo ? "cuckoo_direct" : "cuckoo", benchmarkConfig, directIo) {
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
            while (handled < benchmarkConfig.numQueries) {
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
