#!/bin/bash
hostname
strings Query | grep fPIC

function testMethod() {
    for a in 1 2 4 8 16 32 64 128; do
        #for i in $(seq 1 2); do
            ./Query --input_file $1 --num_queries 3M --index_type eliasFano -a $a
            #./Query --input_file $1 --num_queries 3M --index_type uncompressedBitVector -a $a
            ./Query --input_file $1 --num_queries 1M --index_type compressedBitVector -a $a
            ./Query --input_file $1 --num_queries 1M --index_type laVector -a $a
        #done
    done
}

if [[ "$1" == "generate" ]]; then
    ./Twitter --type pachash --output_file /data02/hplehmann/twitter.dat --input_file /data01/hplehmann/datasets/twitter-stream-2021-08-01-to-05.txt
    ./Uniprot --type pachash --output_file /data02/hplehmann/uniprot.dat --input_file /data01/hplehmann/datasets/uniref50.fasta
    ./Wikipedia --type pachash --output_file /data02/hplehmann/wikipedia.dat --input_file /data01/hplehmann/datasets/enwiki-20211101-pages-articles-multistream.xml
    params="--pachash 8 --uring_io --num_queries 0 --num_objects 10M --object_size 256"
    ./Benchmark $params --store_file /data02/hplehmann/equal_s256.dat --object_size_distribution equal
    ./Benchmark $params --store_file /data02/hplehmann/uniform_s256.dat --object_size_distribution uniform
    ./Benchmark $params --store_file /data02/hplehmann/normal_s256.dat --object_size_distribution normal
else
    testMethod /data02/hplehmann/twitter.dat
    testMethod /data02/hplehmann/uniprot.dat
    testMethod /data02/hplehmann/wikipedia.dat
    #testMethod /data02/hplehmann/equal_s256.dat
    #testMethod /data02/hplehmann/normal_s256.dat
    #testMethod /data02/hplehmann/uniform_s256.dat
fi

