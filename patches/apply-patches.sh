#!/bin/bash

cd ../external
for d in * ; do
    cd $d
    patchfile="../../patches/$d.patch"
    if [[ -f "$patchfile" ]]; then
        git restore --staged .
        git restore .
        git apply $patchfile
    fi
    cd ..
done
