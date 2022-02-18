#include "StoreComparisonItem.h"
#include "sux/function/RecSplit.hpp"

class RecSplitComparisonItem : public StoreComparisonItem {
    public:
        std::vector<std::string> keys;
        sux::function::RecSplit<8> *recSplit = nullptr;

        RecSplitComparisonItem(size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem("recsplit", N, averageLength, numQueries) {
            keys = generateRandomStringKeys(N);
        }

        ~RecSplitComparisonItem() {
            delete recSplit;
        }

        void construct() override {
            recSplit = new sux::function::RecSplit<8>(keys, 2000);
        }

        void query() override {
            for (size_t i = 0; i < numQueries; i++) {
                recSplit->operator()(keys[rand() % N]);
            }
        }
};
