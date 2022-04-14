#include "StoreComparisonItem.h"
#include "sux/function/RecSplit.hpp"

class RecSplitComparisonItem : public StoreComparisonItem {
    public:
        sux::function::RecSplit<8> *recSplit = nullptr;

        RecSplitComparisonItem(size_t N, size_t numQueries) :
                StoreComparisonItem("recsplit", N, numQueries) {
        }

        ~RecSplitComparisonItem() override {
            delete recSplit;
        }

        size_t externalSpaceUsage() override {
            return 0;
        }

        void construct(std::vector<std::string> &keys) override {
            recSplit = new sux::function::RecSplit<8>(keys, 2000);
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < numQueries; i++) {
                size_t result = recSplit->operator()(keysQueryOrder[i]);
                DO_NOT_OPTIMIZE(result);
            }
        }
};
