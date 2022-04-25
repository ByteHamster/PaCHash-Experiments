#include <cmph.h>
#include "../StoreComparisonItem.h"

class ChdComparisonItem : public StoreComparisonItem {
    public:
        cmph_t *mphf = nullptr;
        char **convertedInput = nullptr;
        size_t kPerfect;

        explicit ChdComparisonItem(const BenchmarkConfig& benchmarkConfig)
                : StoreComparisonItem("cmph", benchmarkConfig) {
            kPerfect = 4096 / benchmarkConfig.objectSize;
        }

        bool supportsVariableSize() override {
            return false;
        }

        ~ChdComparisonItem() override {
            if (mphf != nullptr) {
                cmph_destroy(mphf);
            }
        }

        size_t externalSpaceUsage() override {
            return 0;
        }

        void beforeConstruct(std::vector<Object> &objects) override {
            convertedInput = new char*[benchmarkConfig.N];
            for (size_t i = 0; i < objects.size(); i++) {
                convertedInput[i] = objects.at(i).key.data();
                assert(objects.at(i).length == benchmarkConfig.objectSize);
            }
        }

        void construct(std::vector<Object> &objects) override {
            (void) objects;
            cmph_io_adapter_t *source = cmph_io_vector_adapter(convertedInput, benchmarkConfig.N);
            cmph_config_t *config = cmph_config_new(source);
            cmph_config_set_algo(config, CMPH_CHD_PH);
            cmph_config_set_verbosity(config, 0);
            cmph_config_set_graphsize(config, 0.98);
            cmph_config_set_keys_per_bin(config, kPerfect);
            cmph_config_set_b(config, 12); // Needs about 7.8 bits/block, while PaCHash(a=8) needs 5 bits
            mphf = cmph_new(config);
        }

        void afterConstruct() override {
            delete[] convertedInput;
            if (mphf == nullptr) {
                throw std::logic_error("Could not construct");
            }
            float size_bits = 8.0f * (float)cmph_packed_size(mphf);
            std::cout << "Size: "<< size_bits / (double)benchmarkConfig.N << "/object, "
                      << size_bits / (double)(benchmarkConfig.N / kPerfect) << "/block"<<std::endl;
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < benchmarkConfig.numQueries; i++) {
                cmph_uint32 result = cmph_search(mphf, keysQueryOrder[i].data(), keysQueryOrder[i].length());
                DO_NOT_OPTIMIZE(result);
            }
        }
};
