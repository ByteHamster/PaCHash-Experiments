#include <pthash.hpp>
#include "../StoreComparisonItem.h"

class PTHashComparisonItem : public StoreComparisonItem {
    public:
        pthash::single_phf<pthash::murmurhash2_64, pthash::dictionary_dictionary, true> pthashFunction;
        std::vector<std::string> keysOnly;

        explicit PTHashComparisonItem(const BenchmarkConfig& benchmarkConfig)
                : StoreComparisonItem("pthash", benchmarkConfig) {
        }

        bool supportsVariableSize() override {
            return false;
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
            pthash::build_configuration config;
            config.c = 7;
            config.alpha = 0.94;
            config.num_threads = 1;
            config.minimal_output = true;
            config.verbose_output = false;
            pthashFunction.build_in_internal_memory(keysOnly.begin(), keysOnly.size(), config);
        }

        void afterConstruct() override {
            keysOnly.clear();
            keysOnly.shrink_to_fit();
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < benchmarkConfig.numQueries; i++) {
                size_t result = pthashFunction(keysQueryOrder[i]);
                DO_NOT_OPTIMIZE(result);
            }
        }
};
