#!/bin/bash -eu

cd ./scripts/..

for f in ./day01-15/*/*/Makefile; do
    cd $(dirname $f) &&
        make clean &
done
