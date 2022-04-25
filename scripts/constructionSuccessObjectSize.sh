#!/bin/bash
hostname
strings Benchmark | grep fPIC

tryMaxFillDegree() {
    method=$1
    objectSize=$2
    distribution=$3
    fillDegreeInt=1000
    failed=1
    while [ $failed -ne 0 ]; do
        fillDegree=`echo "0.001 * $fillDegreeInt" | bc`
        ./Benchmark --$method \
            --load_factor $fillDegree \
            --store_file /tmp/key_value_store.txt \
            --num_queries 0 --num_objects 10k \
            --object_size $objectSize \
            --object_size_distribution $distribution \
            --iterations 3 --cached_io --uring_io
        failed=$?
        fillDegreeInt=$(($fillDegreeInt-3))
    done
}

for objectSize in $(seq 250 3 650); do
    tryMaxFillDegree "separator 6" $objectSize "equal"
    tryMaxFillDegree "separator 6" $objectSize "normal"
    tryMaxFillDegree "separator 6" $objectSize "uniform"

    tryMaxFillDegree "cuckoo" $objectSize "equal"
    tryMaxFillDegree "cuckoo" $objectSize "normal"
    tryMaxFillDegree "cuckoo" $objectSize "uniform"
done

