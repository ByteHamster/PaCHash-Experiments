#!/bin/bash

cd ../external
for d in * ; do
    cd $d
    patch=`git diff HEAD`
    if [[ "$patch" != "" ]]; then
        echo "$patch" > ../../patches/$d.patch
    fi
    cd ..
done

