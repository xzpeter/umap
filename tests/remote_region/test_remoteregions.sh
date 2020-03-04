#!/bin/bash

EXE=./tests/remote_region/remote_region

KB=1024
MB=$((1024*KB))
GB=$((1024*MB))

regionsize=$(( GB * 1))
dsize=8


for p in 256 128 64 32 16 8 4
do
    psize=$(( KB * p))
    pages=$(( regionsize / psize))

    for numProc in 2 4 8 16
    do
	for numThreads in 1 #2 4
	do
	    echo "OMP_NUM_THREADS=$numThreads"
	    export OMP_NUM_THREADS=$numThreads
	    #cmd="UMAP_PAGESIZE=$psize srun -n $numProc $EXE $pages $((psize / dsize)) $pages"
	    cmd="UMAP_PAGESIZE=$psize srun -n $numProc $EXE $pages $((psize / dsize)) $pages"
	    echo $cmd
	    eval $cmd
	done
    done
done
