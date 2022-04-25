# PaCHash-Experiments

This repository contains benchmarks for [PaCHash](https://github.com/ByteHamster/PaCHash).

### PaCHash Configurations
Compares different configuration parameters of PaCHash.
The benchmarks consist of shell scripts that call examples from the main PaCHash repository.
To compile and test, execute something like the following:

```
mkdir build-pachash
cmake -DCMAKE_BUILD_TYPE=Release ../external/pachash
make
../scripts/objectSizeBlocksFetched.sh
```

### Comparison with Competitors
Compares PaCHash to competitors from the literature.
Included competitors:

- RocksDB
- LevelDB
- SILT
- RecSplit
- CHD

You need to apply the patches in the `patches` directory to add modify the object stores,
so that we can benchmark their internal index data structures.
You can use the included helper script for that.

To compile and test, execute something like the following:

```
mkdir build
cd build 
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./ObjectStoreComparison --path /path/to/folder/on/ssd
```
