#include <unordered_map>

class StdUnorderedMapComparisonItem : public StoreComparisonItem {
    public:
        std::unordered_map<uint64_t, size_t> map;

        StdUnorderedMapComparisonItem(size_t N, size_t numQueries) :
                StoreComparisonItem("unordered_map", N, numQueries) {
        }

        size_t externalSpaceUsage() override {
            return 0;
        }

        void construct(std::vector<std::string> &keys) override {
            map.reserve(keys.size());
            for (std::string &key : keys) {
                map.insert(std::make_pair(pachash::MurmurHash64(key), 42));
            }
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < numQueries; i++) {
                size_t result = map.at(pachash::MurmurHash64(keysQueryOrder[i]));
                DO_NOT_OPTIMIZE(result);
            }
        }
};
