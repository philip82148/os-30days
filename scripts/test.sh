#!/bin/bash -eu

cd ./scripts/..

for f in ./*/*/Makefile; do
    dir=$(dirname $f)
    echo "running $dir..."
    cd $dir &&
        make run &
    sleep 2
    qemu_pid=$(ps -e -o pid,cmd | grep qemu | grep -v grep | awk '{ print $1 }')
    kill $qemu_pid
done
