#include <unordered_map>

class StdUnorderedMapComparisonItem : public StoreComparisonItem {
    public:
        std::unordered_map<std::string, size_t> map;

        StdUnorderedMapComparisonItem(size_t N, size_t objectSize, size_t numQueries) :
                StoreComparisonItem("unordered_map", N, objectSize, numQueries) {
        }

        size_t externalSpaceUsage() override {
            return 0;
        }

        void construct(std::vector<std::string> &keys) override {
            map.reserve(keys.size());
            for (std::string &key : keys) {
                map.insert(std::make_pair(key, 42));
            }
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < numQueries; i++) {
                size_t result = map.at(keysQueryOrder[i]);
                DO_NOT_OPTIMIZE(result);
            }
        }
};
