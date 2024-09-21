#!/bin/bash -eu

cd ./scripts/..

for f in ./*/*/*/Makefile; do
    dir=$(dirname $f)
    hyphen60=------------------------------------------------------------
    echo -e "\e[36mtesting $dir $hyphen60$hyphen60\e[0m"
    if [ $(cd $dir && make exists) = "true" ]; then
        continue
    fi
    cd $dir && make run &
    sleep 0.5
    qemu_pid=$(ps -e -o pid,cmd | grep qemu | grep -v grep | awk '{ print $1 }')
    while [ -z $qemu_pid ]; do
        sleep 0.2
        qemu_pid=$(ps -e -o pid,cmd | grep qemu | grep -v grep | awk '{ print $1 }')
    done
    sleep 1.5
    kill $qemu_pid
done
