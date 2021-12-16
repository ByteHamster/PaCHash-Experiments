#include <fawnds_combi.h>
#include "fawnds.h"
#include "configuration.h"
#include "fawnds_factory.h"

void testSilt(size_t N, size_t averageLength) {
    std::vector<uint64_t> keys = generateRandomKeys(N);

    system("mkdir -p /tmp/silt-test");
    auto* config = new fawn::Configuration("../siltConfig.xml");
    config->SetStringValue("data-len", std::to_string(averageLength));
    fawn::FawnDS_Combi* store = dynamic_cast<fawn::FawnDS_Combi *>(fawn::FawnDS_Factory::New(config));
    store->Destroy();
    fawn::FawnDS_Return res = store->Create();
    assert(res == fawn::FawnDS_Return::OK);
    char *content = static_cast<char *>(malloc(averageLength));
    for (uint64_t key : keys) {
        fawn::ConstRefValue value(content, averageLength);
        fawn::FawnDS_Return res = store->Put(fawn::ConstRefValue(&key), value);
        assert(res == fawn::FawnDS_Return::OK);
        //std::cout<<"Put("<<key<<") = "<<res<<std::endl;
    }

    std::cout<<"Waiting"<<std::endl;
    store->Flush();
    sleep(2);
    std::cout<<"Querying"<<std::endl;

    auto queryStart = std::chrono::high_resolution_clock::now();
    size_t handled = 0;
    while (handled < 1e6) {
        fawn::Value valueRead;
        fawn::FawnDS_Return res = store->Get(fawn::ConstRefValue(&keys[rand() % keys.size()]), valueRead);
        assert(res == fawn::FawnDS_Return::OK);
        //std::cout<<"Get("<<key<<") = "<<res<<" "<<valueRead.str()<<std::endl;
        handled++;
    }
    auto queryEnd = std::chrono::high_resolution_clock::now();
    long timeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();

    std::cout<<"RESULT"
             <<" objectSize="<<averageLength
             <<" method=silt"
             <<" numObjects="<<N
             <<" numQueries="<<handled
             <<" timeMs="<<timeMicroseconds/1000
             <<" queriesPerSecond="<<(double)handled*1000000.0/((double)timeMicroseconds)
             <<" perObject="<<(((double)timeMicroseconds/(double)handled)*1000)<<std::endl;
    store->Close();
}