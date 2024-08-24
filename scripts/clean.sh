#!/bin/bash -eu

cd ./scripts/..

for f in ./*/*/Makefile; do
    cd $(dirname $f) &&
        make clean &
done
