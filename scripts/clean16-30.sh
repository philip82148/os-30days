#!/bin/bash -eu

cd ./scripts/..

for f in ./day16-30/*/*/Makefile; do
    cd $(dirname $f) &&
        make clean &
done
