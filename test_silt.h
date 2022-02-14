#include <fawnds_combi.h>
#include "fawnds.h"
#include "configuration.h"
#include "fawnds_factory.h"

class SiltComparisonItem : public StoreComparisonItem {
    public:
        std::vector<std::uint64_t> keys;
        fawn::FawnDS_Combi* store;

        SiltComparisonItem(size_t N, size_t averageLength, size_t numQueries) :
                StoreComparisonItem("silt", N, averageLength, numQueries) {
            keys = generateRandomKeys(N);

            system("mkdir -p /tmp/silt-test");
            auto* config = new fawn::Configuration("../siltConfig.xml");
            config->SetStringValue("data-len", std::to_string(averageLength));
            store = dynamic_cast<fawn::FawnDS_Combi *>(fawn::FawnDS_Factory::New(config));
            store->Destroy();
            fawn::FawnDS_Return res = store->Create();
            assert(res == fawn::FawnDS_Return::OK);
        }

        void construct() override {
            char *content = static_cast<char *>(malloc(averageLength));
            for (uint64_t key : keys) {
                fawn::ConstRefValue value(content, averageLength);
                fawn::FawnDS_Return res = store->Put(fawn::ConstRefValue(&key), value);
                assert(res == fawn::FawnDS_Return::OK);
                //std::cout<<"Put("<<key<<") = "<<res<<std::endl;
            }
            store->Flush();
        }

        void query() override {
            size_t handled = 0;
            while (handled < numQueries) {
                fawn::Value valueRead;
                fawn::FawnDS_Return res = store->Get(fawn::ConstRefValue(&keys[rand() % keys.size()]), valueRead);
                assert(res == fawn::FawnDS_Return::OK);
                //std::cout<<"Get("<<key<<") = "<<res<<" "<<valueRead.str()<<std::endl;
                handled++;
            }
        }
};
