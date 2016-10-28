#!/bin/bash

function do_test()
{
    cd IS/
    make clean
    rm -rf .mpire/
    #The rank to capture
    export MPIRE_RANK=0
    make CLASS=W NPROCS=4
    #Capture
    LD_PRELOAD=/usr/local/lib/libmpire_capture.so mpirun -n 4 ./is.W.4
    #Replay
    LD_PRELOAD=/usr/local/lib/libmpire_replay.so ./is.W.4
}

source ../source.sh
