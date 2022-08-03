# PaCHash-Experiments

This repository contains benchmarks for [PaCHash](https://github.com/ByteHamster/PaCHash).

### PaCHash Configurations
Compares different configuration parameters of PaCHash.
The benchmarks consist of shell scripts that call examples from the main PaCHash repository.
To compile and test, execute something like the following:

```
mkdir build-pachash
cd build-pachash
cmake -DCMAKE_BUILD_TYPE=Release ../external/pachash
cmake --build . -j 8
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

You need to apply the patches in the `patches` directory to modify the object stores,
so that we can benchmark their internal index data structures.
You can use the included helper script for that.

```
cd patches
./apply-patches.sh
cd ..
```

The following dependencies are needed to compile the competitors.
Please install them before continuing:

- cblas
- xerces-c
- googletest (libgtest-dev)

To compile, execute something like the following:

```
mkdir build
cd build 
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j 8
```

To run the full comparison plot from our paper you can use the following command.
On our machine with a very fast SSD, this takes about 3 hours.

```
./ObjectStoreComparison --path /path/to/folder/on/ssd  --delta_step 400k | tee comparisonPlot.txt
```

To only run a smaller experiment for reproducibility, you can also run the following command.
On our machine with a very fast SSD, this takes about 10 minutes.

```
./ObjectStoreComparison --path /path/to/folder/on/ssd --delta_step 1M --repetitions 1 | tee comparisonPlot.txt
```

From the command output stored in `comparisonPlot.txt`, you can generate the main comparison plot using the following commands.

```
cd scripts
cp ../build/comparisonPlot.txt comparisonPlot.txt
sqlplot-tools largeComparisonPlot.tex
pdflatex largeComparisonPlot.tex
```

As our script relies on an sqlplot-tools feature that is not merged yet, you can get the tool from this branch: https://github.com/lorenzhs/sqlplot-tools/tree/feature/attribute_mark

### License
The competitors in the `external` folder are licensed under their respective license.
The benchmark code is licensed under the [GPLv3](/LICENSE).
If you use the project in an academic context or publication, please cite our paper.
