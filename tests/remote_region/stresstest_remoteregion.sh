#!/bin/bash

EXE=./tests/remote_region/remote_region

KB=1024
MB=$((1024*KB))
GB=$((1024*MB))

regionSize=$(( GB * 1))
numNodes=2
numUpdates=100
numPeriods=10

for p in 256 128 64 32 16 8 4
do
    psize=$(( KB * p))
    pages=$(( regionSize / psize))

    for numProcPerNode in 1
    do
	for numThreads in 4
	do
	    export OMP_NUM_THREADS=$numThreads
	    cmd="UMAP_PAGESIZE=$psize srun --ntasks-per-node=$numProcPerNode -N $numNodes $EXE $pages $numUpdates $numPeriods"
	    echo $cmd
	    eval $cmd
	done
    done
done
