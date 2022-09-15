#include <unordered_map>
#include "../StoreComparisonItem.h"

class StdUnorderedMapComparisonItem : public StoreComparisonItem {
    public:
        std::unordered_map<uint64_t, char*> map;

        explicit StdUnorderedMapComparisonItem(const BenchmarkConfig& benchmarkConfig)
                : StoreComparisonItem("unordered_map", benchmarkConfig) {
        }

        bool supportsVariableSize() override {
            return true;
        }

        size_t externalSpaceUsage() override {
            return 0;
        }

        void construct(std::vector<Object> &objects) override {
            map.reserve(objects.size());
            for (const Object &object : objects) {
                assert(object.length == benchmarkConfig.objectSize);
                map.insert(std::make_pair(util::MurmurHash64(object.key), nullptr));
            }
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < benchmarkConfig.numQueries; i++) {
                char *result = map.at(util::MurmurHash64(keysQueryOrder[i]));
                DO_NOT_OPTIMIZE(result);
            }
        }
};
