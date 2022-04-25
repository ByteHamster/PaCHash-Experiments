#include <unordered_map>
#include "../StoreComparisonItem.h"

class StdUnorderedMapComparisonItem : public StoreComparisonItem {
    public:
        std::unordered_map<uint64_t, char*> map;

        StdUnorderedMapComparisonItem(size_t N, size_t numQueries) :
                StoreComparisonItem("unordered_map", N, numQueries) {
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
                assert(object.length == objectSize);
                map.insert(std::make_pair(pachash::MurmurHash64(object.key), nullptr));
            }
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < numQueries; i++) {
                char *result = map.at(pachash::MurmurHash64(keysQueryOrder[i]));
                DO_NOT_OPTIMIZE(result);
            }
        }
};
