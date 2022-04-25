#include <sux/function/RecSplit.hpp>
#include "../StoreComparisonItem.h"

class RecSplitComparisonItem : public StoreComparisonItem {
    public:
        sux::function::RecSplit<8> *recSplit = nullptr;
        std::vector<std::string> keysOnly;

        explicit RecSplitComparisonItem(const BenchmarkConfig& benchmarkConfig)
                : StoreComparisonItem("recsplit", benchmarkConfig) {
        }

        bool supportsVariableSize() override {
            return false;
        }

        ~RecSplitComparisonItem() override {
            delete recSplit;
        }

        size_t externalSpaceUsage() override {
            return 0;
        }

        void beforeConstruct(std::vector<Object> &objects) override {
            keysOnly.reserve(objects.size());
            for (const Object &object : objects) {
                keysOnly.emplace_back(object.key);
                assert(object.length == benchmarkConfig.objectSize);
            }
        }

        void construct(std::vector<Object> &objects) override {
            (void) objects;
            recSplit = new sux::function::RecSplit<8>(keysOnly, 2000);
        }

        void afterConstruct() override {
            keysOnly.clear();
            keysOnly.shrink_to_fit();
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < benchmarkConfig.numQueries; i++) {
                size_t result = recSplit->operator()(keysQueryOrder[i]);
                DO_NOT_OPTIMIZE(result);
            }
        }
};
