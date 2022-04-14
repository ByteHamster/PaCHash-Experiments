#include "StoreComparisonItem.h"
#include "sux/function/RecSplit.hpp"

class RecSplitComparisonItem : public StoreComparisonItem {
    public:
        sux::function::RecSplit<8> *recSplit = nullptr;
        std::vector<std::string> keysOnly;

        RecSplitComparisonItem(size_t N, size_t numQueries) :
                StoreComparisonItem("recsplit", N, numQueries) {
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
                assert(object.length == objectSize);
            }
        }

        void construct(std::vector<Object> &objects) override {
            recSplit = new sux::function::RecSplit<8>(keysOnly, 2000);
        }

        void afterConstruct() override {
            keysOnly.clear();
            keysOnly.shrink_to_fit();
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < numQueries; i++) {
                size_t result = recSplit->operator()(keysQueryOrder[i]);
                DO_NOT_OPTIMIZE(result);
            }
        }
};
