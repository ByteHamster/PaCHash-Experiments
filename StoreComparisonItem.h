#pragma once

#include <utility>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <random>
#include <filesystem>
#include <malloc.h>

#define DO_NOT_OPTIMIZE(value) asm volatile ("" : : "r,m"(value) : "memory")

std::size_t directorySize(const char *name) {
    const std::filesystem::path& directory(name);
    std::size_t size{ 0 };
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file() && !entry.is_symlink()) {
            size += entry.file_size();
        }
    }
    return size;
}

std::size_t fileSize(const char *name) {
    std::filesystem::path file(name);
    return std::filesystem::file_size(file);
}

struct Object {
    std::string key;
    size_t length;

    Object(std::string key, size_t length) : key(std::move(key)), length(length) {
    }
};

class StoreComparisonItem {
    private:
        std::string method;
    public:
        size_t N;
        size_t objectSize = 256;
        size_t numQueries;
        char *emptyValuePointer;
        size_t allocationsBeginning = 0;
        bool directIo = true;
        bool variableSize = true;

        StoreComparisonItem(std::string method, size_t N, size_t numQueries)
                : method(std::move(method)), N(N), numQueries(numQueries) {
            emptyValuePointer = new char[objectSize];
            memset(emptyValuePointer, 42, objectSize * sizeof(char));
            allocationsBeginning = mallinfo2().uordblks;
        }

        virtual ~StoreComparisonItem() {
            delete[] emptyValuePointer;
        }

        virtual bool supportsVariableSize() = 0;

        virtual void beforeConstruct(std::vector<Object> &objects) {
            (void) objects;
        }
        virtual void construct(std::vector<Object> &objects) = 0;
        virtual void afterConstruct() { };

        virtual void beforeQuery() { };
        virtual void query(std::vector<std::string> &keysQueryOrder) = 0;
        virtual void afterQuery() { };

        virtual size_t externalSpaceUsage() = 0;

        void performBenchmark() {
            if (variableSize && !supportsVariableSize()) {
                std::cout<<method<<" does not support variable size objects. Skipping."<<std::endl;
                return;
            }
            std::vector<Object> objects = generateRandomObjects(N);
            std::cout<<method<<": Construction"<<std::endl;
            beforeConstruct(objects);
            auto constructStart = std::chrono::high_resolution_clock::now();
            construct(objects);
            auto constructEnd = std::chrono::high_resolution_clock::now();
            afterConstruct();
            long constructTimeMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(constructEnd - constructStart).count();

            std::vector<std::string> keysQueryOrder;
            keysQueryOrder.reserve(numQueries);
            uint64_t seed = std::random_device{}();
            std::cout<<method<<": Preparing Query (seed: "<<seed<<")"<<std::endl;
            std::mt19937_64 generator(seed);
            std::uniform_int_distribution<uint64_t> dist(0, N - 1);
            for (size_t i = 0; i < numQueries + 200; i++) {
                keysQueryOrder.push_back(objects.at(dist(generator)).key);
            }
            usleep(1000*1000);

            std::cout<<method<<": Query"<<std::endl;
            beforeQuery();
            auto queryStart = std::chrono::high_resolution_clock::now();
            query(keysQueryOrder);
            auto queryEnd = std::chrono::high_resolution_clock::now();
            long queryTimeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();
            double queryTimeSeconds = (double)queryTimeMicroseconds / (double)1e6;
            afterQuery();

            objects.clear();
            objects.shrink_to_fit();
            keysQueryOrder.clear();
            keysQueryOrder.shrink_to_fit();
            size_t allocationsEnd = mallinfo2().uordblks;

            std::cout << "RESULT"
                      << " method=" << method
                      << " objectSize=" << objectSize
                      << " numObjects=" << N
                      << " numQueries=" << numQueries
                      << " queryTimeMs=" << 1000 * queryTimeSeconds
                      << " queriesPerSecond=" << (double)numQueries / queryTimeSeconds
                      << " constructionTimeMs=" << constructTimeMilliseconds
                      << " externalSpaceBytes=" << externalSpaceUsage()
                      << " internalSpaceBytes=" << (allocationsEnd - allocationsBeginning)
                      << std::endl;
            usleep(1000*1000);
        }

    private:
        std::vector<Object> generateRandomObjects(size_t N) {
            uint64_t seed = std::random_device{}();
            std::cout<<"# Seed for input keys: "<<seed<<std::endl;
            std::mt19937_64 generator(seed);
            std::uniform_int_distribution<uint64_t> keyDist(0, UINT64_MAX);
            std::uniform_int_distribution<size_t> sizeDist(static_cast<size_t>(0.5 * objectSize),
                                                           static_cast<size_t>(1.5 * objectSize));
            std::vector<Object> objects;
            objects.reserve(N);
            for (size_t i = 0; i < N; i++) {
                uint64_t key = keyDist(generator);
                while (strnlen(reinterpret_cast<const char *>(&key), sizeof(uint64_t)) != sizeof(uint64_t)) {
                    key = keyDist(generator); // No null bytes in key
                }
                size_t size = objectSize;
                if (variableSize) {
                    size = sizeDist(generator);
                }
                objects.emplace_back(std::string((char *)&key, sizeof(uint64_t)), size);
            }
            return objects;
        }
};
