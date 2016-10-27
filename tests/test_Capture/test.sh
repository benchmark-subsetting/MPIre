#!/bin/bash

function do_test()
{
    cd BT/
    make clean
    rm -rf .mpire/
    #The rank to capture
    export MPIRE_RANK=0
    make CLASS=W NPROCS=4
    LD_PRELOAD=/usr/local/lib/libmpire_capture.so mpirun -n 4 ./bt.W.4
    nbfiles=`ls .mpire/dumps/0/log/ | wc -l`
    echo ${nbfiles}
    if [[ ${nbfiles} -eq 2422 ]]; then
      exit 0
    else
      exit 1
    fi
}

source ../source.sh
