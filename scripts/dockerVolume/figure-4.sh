#!/bin/bash

# Run benchmark
cd /opt/pachash/build
./ObjectStoreComparison --path /opt/testDirectory --delta_step 1500k --repetitions 1 | tee figure-4.txt

# Build plot
cd /opt/pachash/scripts
cp /opt/pachash/build/figure-4.txt figure-4.txt
/opt/sqlplot-tools/build/src/sqlplot-tools figure-4.tex
pdflatex figure-4.tex
pdflatex figure-4.tex
cp figure-4.pdf /opt/dockerVolume
cp figure-4.txt /opt/dockerVolume

