#include <utility>
#include <unistd.h>

#pragma once

class StoreComparisonItem {
    public:
        std::string method;
        size_t N;
        size_t averageLength;
        size_t numQueries;

        StoreComparisonItem(std::string method, size_t N, size_t averageLength, size_t numQueries)
                : method(std::move(method)), N(N), averageLength(averageLength), numQueries(numQueries) {

        }

        virtual void construct() = 0;
        virtual void query() = 0;

        void performBenchmark() {
            std::cout<<method<<": Construction"<<std::endl;
            auto constructStart = std::chrono::high_resolution_clock::now();
            construct();
            auto constructEnd = std::chrono::high_resolution_clock::now();
            long constructTimeMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(constructEnd - constructStart).count();

            usleep(200*1000);

            std::cout<<method<<": Query"<<std::endl;
            auto queryStart = std::chrono::high_resolution_clock::now();
            query();
            auto queryEnd = std::chrono::high_resolution_clock::now();
            long queryTimeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(queryEnd - queryStart).count();

            usleep(200*1000);

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
};
