#!/bin/bash
hostname
strings Query | grep fPIC

baseFolder="$1"
if [ $# -eq 0 ]; then
  echo "No base folder given"
fi

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
    ./Twitter --type pachash --output_file "$baseFolder/twitter.dat" --input_file "$baseFolder/datasets/twitter-stream-2021-08-01-to-05.txt"
    ./Uniprot --type pachash --output_file "$baseFolder/uniprot.dat" --input_file "$baseFolder/datasets/uniref50.fasta"
    ./Wikipedia --type pachash --output_file "$baseFolder/wikipedia.dat" --input_file "$baseFolder/datasets/enwiki-20211101-pages-articles-multistream.xml"
    params="--pachash 8 --uring_io --num_queries 0 --num_objects 10M --object_size 256"
    ./Benchmark $params --store_file "$baseFolder/equal_s256.dat" --object_size_distribution equal
    ./Benchmark $params --store_file "$baseFolder/uniform_s256.dat" --object_size_distribution uniform
    ./Benchmark $params --store_file "$baseFolder/normal_s256.dat" --object_size_distribution normal
else
    testMethod "$baseFolder/twitter.dat"
    testMethod "$baseFolder/uniprot.dat"
    testMethod "$baseFolder/wikipedia.dat"
    #testMethod "$baseFolder/equal_s256.dat"
    #testMethod "$baseFolder/normal_s256.dat"
    #testMethod "$baseFolder/uniform_s256.dat"
fi
