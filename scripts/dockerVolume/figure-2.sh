#!/bin/bash

# Run benchmark
cd /opt/pachash/external/pachash/build
rm -f figure-2.txt
params="--uring_io --store_file /opt/testDirectory/filename.dat --num_queries 3M --iterations 1 --queue_depth 128 --object_size_distribution normal"
for objectSize in $(seq 128 128 896); do
    numObjects=$((1500000000/$objectSize)) # Always 1.5 GB
    ./Benchmark --pachash 2 --load_factor 1 $params --object_size $objectSize --num_objects $numObjects | tee --append figure-2.txt
    ./Benchmark --pachash 4 --load_factor 1 $params --object_size $objectSize --num_objects $numObjects | tee --append figure-2.txt
    ./Benchmark --pachash 8 --load_factor 1 $params --object_size $objectSize --num_objects $numObjects | tee --append figure-2.txt
    ./Benchmark --pachash 16 --load_factor 1 $params --object_size $objectSize --num_objects $numObjects | tee --append figure-2.txt
done

# Build plot
cd /opt/pachash/scripts
cp /opt/pachash/external/pachash/build/figure-2.txt figure-2.txt
/opt/sqlplot-tools/build/src/sqlplot-tools figure-2.tex
pdflatex figure-2.tex
pdflatex figure-2.tex
cp figure-2.pdf /opt/dockerVolume
cp figure-2.txt /opt/dockerVolume

