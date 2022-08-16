#!/bin/bash
hostname
strings Benchmark | grep fPIC

filename="$1"
if [ $# -eq 0 ]; then
  echo "No file name given"
fi

params="--uring_io --store_file $filename --num_queries 1M --iterations 1 --queue_depth 128 --object_size_distribution normal"

for objectSize in $(seq 64 64 960); do
    numObjects=$((1500000000/$objectSize)) # Always 1.5 GB
    ./Benchmark --pachash 2 --load_factor 1 $params --object_size $objectSize --num_objects $numObjects
    ./Benchmark --pachash 4 --load_factor 1 $params --object_size $objectSize --num_objects $numObjects
    ./Benchmark --pachash 8 --load_factor 1 $params --object_size $objectSize --num_objects $numObjects
    ./Benchmark --pachash 16 --load_factor 1 $params --object_size $objectSize --num_objects $numObjects
    ./Benchmark --pachash 32 --load_factor 1 $params --object_size $objectSize --num_objects $numObjects
done

