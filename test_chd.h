#include "cmph.h"

class ChdComparisonItem : public StoreComparisonItem {
    public:
        cmph_t *mphf = nullptr;
        char **convertedInput;
        size_t kPerfect;

        ChdComparisonItem(size_t N, size_t objectSize, size_t numQueries) :
                StoreComparisonItem("cmph", N, objectSize, numQueries) {
            convertedInput = new char*[N];
            kPerfect = 4096 / objectSize;
        }

        ~ChdComparisonItem() override {
            delete[] convertedInput;
            cmph_destroy(mphf);
        }

        size_t externalSpaceUsage() override {
            return 0;
        }

        void beforeConstruct(std::vector<std::string> &keys) override {
            for (size_t i = 0; i < keys.size(); i++) {
                convertedInput[i] = keys.at(i).data();
            }
        }

        void construct(std::vector<std::string> &keys) override {
            cmph_io_adapter_t *source = cmph_io_vector_adapter(convertedInput, N);
            cmph_config_t *config = cmph_config_new(source);
            cmph_config_set_algo(config, CMPH_CHD_PH);
            cmph_config_set_verbosity(config, 0);
            cmph_config_set_graphsize(config, 0.98);
            cmph_config_set_keys_per_bin(config, kPerfect);
            cmph_config_set_b(config, 12); // Needs about 7.8 bits/block, while PaCHash(a=8) needs 5 bits
            mphf = cmph_new(config);
        }

        void afterConstruct() override {
            if (mphf == nullptr) {
                throw std::logic_error("Could not construct");
            }
            float size_bits = 8.0f * (float)cmph_packed_size(mphf);
            std::cout<<"Size: "<< size_bits / (double)N<<"/object, "
                    << size_bits / (double)(N / kPerfect)<<"/block"<<std::endl;
        }

        void query(std::vector<std::string> &keysQueryOrder) override {
            for (size_t i = 0; i < numQueries; i++) {
                cmph_uint32 result = cmph_search(mphf, keysQueryOrder[i].data(), keysQueryOrder[i].length());
                DO_NOT_OPTIMIZE(result);
            }
        }
};
