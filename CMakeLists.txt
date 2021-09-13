cmake_minimum_required(VERSION 3.21)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
project(ObjectStoreComparison)

set(CMAKE_CXX_STANDARD 17)

add_executable(ObjectStoreComparison main.cpp external/elias-fano/benchmark/RandomObjectProvider.h)

find_package(LibUring REQUIRED)
target_link_libraries(ObjectStoreComparison PUBLIC ${LIBURING_LIBRARIES})

set(WITH_TESTS OFF)
set(WITH_BENCHMARK_TOOLS OFF)
set(WITH_CORE_TOOLS OFF)
set(WITH_TOOLS OFF)
set(WITH_GFLAGS OFF)
set(WITH_LIBURING ON)
set(ROCKSDB_BUILD_SHARED OFF)
add_subdirectory(external/rocksdb)
target_link_libraries(ObjectStoreComparison PUBLIC rocksdb)
target_include_directories(ObjectStoreComparison PUBLIC external/rocksdb/include)

add_subdirectory(external/elias-fano)
target_link_libraries(ObjectStoreComparison PUBLIC VariableSizeObjectStore)
