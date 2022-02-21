#pragma once

#include <utility>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <random>

#define DO_NOT_OPTIMIZE(value) asm volatile ("" : : "r,m"(value) : "memory")

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
            delete emptyValuePointer;
        }

        virtual void beforeConstruct() { };
        virtual void construct(std::vector<std::string> &keys) = 0;
        virtual void afterConstruct() { };

        virtual void beforeQuery() { };
        virtual void query(std::vector<std::string> &keysQueryOrder) = 0;
        virtual void afterQuery() { };

        void performBenchmark() {
            std::vector<std::string> keys = generateRandomKeys(N);
            std::cout<<method<<": Construction"<<std::endl;
            beforeConstruct();
            auto constructStart = std::chrono::high_resolution_clock::now();
            construct(keys);
            auto constructEnd = std::chrono::high_resolution_clock::now();
            afterConstruct();
            long constructTimeMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(constructEnd - constructStart).count();

            std::cout<<method<<": Preparing Query"<<std::endl;
            std::vector<std::string> keysQueryOrder;
            keysQueryOrder.reserve(numQueries);
            std::mt19937_64 generator(std::random_device{}());
            std::uniform_int_distribution<uint64_t> dist(0, N - 1);
            for (size_t i = 0; i < numQueries + 200; i++) {
                keysQueryOrder.push_back(keys.at(dist(generator)));
            }
            usleep(500*1000);

            std::cout<<method<<": Query"<<std::endl;
            beforeQuery();
            auto queryStart = std::chrono::high_resolution_clock::now();
            query(keysQueryOrder);
            auto queryEnd = std::chrono::high_resolution_clock::now();
            afterQuery();
            long queryTimeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();

            usleep(500*1000);

            std::cout << "RESULT"
                      << " method=" << method
                      << " objectSize=" << objectSize
                      << " numObjects=" << N
                      << " numQueries=" <<numQueries
                      << " timeMs=" << queryTimeMicroseconds / 1000
                      << " queriesPerSecond=" <<(double)numQueries*1000000.0/((double)queryTimeMicroseconds)
                      << " perObject=" << (((double)queryTimeMicroseconds / (double)numQueries) * 1000)
                      << " construction=" << constructTimeMilliseconds << std::endl;
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
                keys.emplace_back(std::string((char *)&key, sizeof(uint64_t)));
            }
            return keys;
        }
};
