#include <PaCHashObjectStore.h>

void testPaCHash(size_t N, size_t averageLength) {
    pachash::PaCHashObjectStore<8> objectStore(1, "/tmp/pachash-test", 0);
    std::vector<std::uint64_t> keys = generateRandomKeys(N);

    auto hashFunction = [](const std::uint64_t &key) -> pachash::StoreConfig::key_t {
        return key;
    };
    auto lengthEx = [averageLength](const std::uint64_t &key) -> size_t {
        (void) key;
        return averageLength;
    };
    char *content = static_cast<char *>(malloc(averageLength));
    auto valueEx = [content](const std::uint64_t &key) -> const char * {
        (void) key;
        return content;
    };
    objectStore.writeToFile(keys.begin(), keys.end(), hashFunction, lengthEx, valueEx);
    objectStore.reloadFromFile();

    size_t depth = 128;
    pachash::ObjectStoreView<pachash::PaCHashObjectStore<8>, pachash::UringIO> objectStoreView(objectStore, 0, depth);
    std::vector<pachash::QueryHandle> queryHandles;
    for (size_t i = 0; i < depth; i++) {
        queryHandles.emplace_back(objectStore);
    }

    auto queryStart = std::chrono::high_resolution_clock::now();
    size_t handled = 0;

    // Fill in-flight queue
    for (size_t i = 0; i < depth; i++) {
        queryHandles[i].key = keys[rand() % N];
        objectStoreView.enqueueQuery(&queryHandles[i]);
        handled++;
    }
    objectStoreView.submit();

    // Submit new queries as old ones complete
    while (handled < 2500000) {
        pachash::QueryHandle *handle = objectStoreView.awaitAny();
        do {
            if (handle->resultPtr == nullptr) {
                throw std::logic_error("Did not find item");
            }
            handle->key = keys[rand() % N];
            objectStoreView.enqueueQuery(handle);
            handle = objectStoreView.peekAny();
            handled++;
        } while (handle != nullptr);
        objectStoreView.submit();
    }

    // Collect remaining in-flight queries
    for (size_t i = 0; i < depth; i++) {
        pachash::QueryHandle *handle = objectStoreView.awaitAny();
        if (handle->resultPtr == nullptr) {
            throw std::logic_error("Did not find item");
        }
        handled++;
    }

    auto queryEnd = std::chrono::high_resolution_clock::now();
    long timeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();

    std::cout<<"RESULT"
             <<" objectSize="<<averageLength
             <<" method=pachash"
             <<" numObjects="<<N
             <<" numQueries="<<handled
             <<" timeMs="<<timeMicroseconds/1000
             <<" queriesPerSecond="<<(double)handled*1000000.0/((double)timeMicroseconds)
             <<" perObject="<<(((double)timeMicroseconds/(double)handled)*1000)<<std::endl;
}