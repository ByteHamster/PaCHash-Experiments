#pragma once

#include <utility>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <random>
#include <filesystem>
#include <malloc_count.h>

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

class StoreComparisonItem {
    private:
        std::string method;
    public:
        size_t N;
        size_t objectSize;
        size_t numQueries;
        const char *emptyValuePointer;

        StoreComparisonItem(std::string method, size_t N, size_t objectSize, size_t numQueries)
                : method(std::move(method)), N(N), objectSize(objectSize), numQueries(numQueries) {
            emptyValuePointer = new char[objectSize];
        }

        virtual ~StoreComparisonItem() {
            delete[] emptyValuePointer;
        }

        virtual void beforeConstruct(std::vector<std::string> &keys) { };
        virtual void construct(std::vector<std::string> &keys) = 0;
        virtual void afterConstruct() { };

        virtual void beforeQuery() { };
        virtual void query(std::vector<std::string> &keysQueryOrder) = 0;
        virtual void afterQuery() { };

        virtual size_t externalSpaceUsage() = 0;

        void performBenchmark() {
            std::vector<std::string> keys = generateRandomKeys(N);
            std::cout<<method<<": Construction"<<std::endl;
            beforeConstruct(keys);
            long allocationsBeforeConstruction = 0; // static_cast<long>(malloc_count_current());
            auto constructStart = std::chrono::high_resolution_clock::now();
            construct(keys);
            auto constructEnd = std::chrono::high_resolution_clock::now();
            long allocationsAfterConstruction = 0; // static_cast<long>(malloc_count_current());
            afterConstruct();
            long constructTimeMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(constructEnd - constructStart).count();

            std::vector<std::string> keysQueryOrder;
            keysQueryOrder.reserve(numQueries);
            uint64_t seed = std::random_device{}();
            std::cout<<method<<": Preparing Query (seed: "<<seed<<")"<<std::endl;
            std::mt19937_64 generator(seed);
            std::uniform_int_distribution<uint64_t> dist(0, N - 1);
            for (size_t i = 0; i < numQueries + 200; i++) {
                keysQueryOrder.push_back(keys.at(dist(generator)));
            }
            usleep(1000*1000);

            std::cout<<method<<": Query"<<std::endl;
            beforeQuery();
            auto queryStart = std::chrono::high_resolution_clock::now();
            query(keysQueryOrder);
            auto queryEnd = std::chrono::high_resolution_clock::now();
            afterQuery();
            long queryTimeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();

            std::cout << "RESULT"
                      << " method=" << method
                      << " objectSize=" << objectSize
                      << " numObjects=" << N
                      << " numQueries=" <<numQueries
                      << " timeMs=" << queryTimeMicroseconds / 1000
                      << " queriesPerSecond=" <<(double)numQueries*1000000.0/((double)queryTimeMicroseconds)
                      << " perObject=" << (((double)queryTimeMicroseconds / (double)numQueries) * 1000)
                      << " construction=" << constructTimeMilliseconds
                      << " externalSpace=" << externalSpaceUsage()
                      << " internalSpace=" << (allocationsAfterConstruction - allocationsBeforeConstruction)
                      << std::endl;
            usleep(1000*1000);
        }

    private:
        static std::vector<std::string> generateRandomKeys(size_t N) {
            uint64_t seed = std::random_device{}();
            std::cout<<"# Seed for input keys: "<<seed<<std::endl;
            std::mt19937_64 generator(seed);
            std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
            std::vector<std::string> keys;
            keys.reserve(N);
            for (size_t i = 0; i < N; i++) {
                uint64_t key = dist(generator);
                while (strnlen(reinterpret_cast<const char *>(&key), sizeof(uint64_t)) != sizeof(uint64_t)) {
                    key = dist(generator); // No null bytes in key
                }
                keys.emplace_back(std::string((char *)&key, sizeof(uint64_t)));
            }
            return keys;
        }
};
