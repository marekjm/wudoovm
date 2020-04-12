#!/usr/bin/env bash

set -e

PT_PATH=misc/platformy_technologiczne

# export VIUA_PROC_SCHEDULERS=1
# export VIUA_FFI_SCHEDULERS=1
export VIUA_IO_SCHEDULERS=1
export VIUA_LIBRARY_PATH=./build/stdlib:$PT_PATH

set -x

touch $PT_PATH/libviuapq.cpp.hash
if [[ ! -f $PT_PATH/viuapq.so ]]; then
    echo '0' > $PT_PATH/libviuapq.cpp.hash
fi
if [[ $(cat $PT_PATH/libviuapq.cpp.hash) != $(sha384sum $PT_PATH/libviuapq.cpp) ]]; then
    gxx \
        -fPIC \
        -shared \
        -I/home/maelkum/.local/include \
        -o $PT_PATH/viuapq.so \
        $PT_PATH/libviuapq.cpp \
        -lpq
    sha384sum $PT_PATH/libviuapq.cpp > $PT_PATH/libviuapq.cpp.hash
fi

cat $PT_PATH/pt.sql | psql pt_lab2
./build/bin/vm/asm -o ./pq.bin $PT_PATH/pq.asm
./build/bin/vm/kernel ./pq.bin
