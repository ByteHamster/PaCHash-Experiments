#!/bin/bash

# Run benchmark
cd /opt/pachash/build
./ObjectStoreComparison --path /opt/testDirectory --delta_step 1M --repetitions 1 | tee comparisonPlot.txt

# Build plot
cd /opt/pachash/scripts
cp /opt/pachash/build/comparisonPlot.txt comparisonPlot.txt
/opt/sqlplot-tools/build/src/sqlplot-tools comparisonPlot.tex
pdflatex comparisonPlot.tex
cp comparisonPlot.pdf /opt/dockerVolume
cp comparisonPlot.txt /opt/dockerVolume

