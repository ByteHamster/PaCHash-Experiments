#pragma once

#include <utility>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <random>

class StoreComparisonItem {
    public:
        std::string method;
        size_t N;
        size_t averageLength;
        size_t numQueries;
        const char *emptyValuePointer;
        std::vector<std::string> keys;

        StoreComparisonItem(std::string method, size_t N, size_t averageLength, size_t numQueries)
                : method(std::move(method)), N(N), averageLength(averageLength), numQueries(numQueries) {
            keys = generateRandomKeys(N);
            emptyValuePointer = new char[averageLength];
        }

        virtual ~StoreComparisonItem() {
            delete emptyValuePointer;
        }

        virtual void beforeConstruct() { };
        virtual void construct() = 0;
        virtual void afterConstruct() { };

        virtual void beforeQuery() { };
        virtual void query() = 0;
        virtual void afterQuery() { };

        void performBenchmark() {
            std::cout<<method<<": Construction"<<std::endl;
            beforeConstruct();
            auto constructStart = std::chrono::high_resolution_clock::now();
            construct();
            auto constructEnd = std::chrono::high_resolution_clock::now();
            afterConstruct();
            long constructTimeMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(constructEnd - constructStart).count();

            usleep(500*1000);

            std::cout<<method<<": Query"<<std::endl;
            beforeQuery();
            auto queryStart = std::chrono::high_resolution_clock::now();
            query();
            auto queryEnd = std::chrono::high_resolution_clock::now();
            afterQuery();
            long queryTimeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();

            usleep(500*1000);

            std::cout << "RESULT"
                      << " method=" << method
                      << " objectSize=" << averageLength
                      << " numObjects=" << N
                      << " numQueries=" <<numQueries
                      << " timeMs=" << queryTimeMicroseconds / 1000
                      << " queriesPerSecond=" <<(double)numQueries*1000000.0/((double)queryTimeMicroseconds)
                      << " perObject=" << (((double)queryTimeMicroseconds / (double)numQueries) * 1000)
                      << " construction=" << constructTimeMilliseconds << std::endl;
        }

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
