#!/bin/bash

EXE=./tests/remote_region/remote_region

KB=1024
MB=$((1024*KB))
GB=$((1024*MB))

numUpdates=100
numPeriods=100

serverNode=flash3
clientNode=flash2
#"flash3,flash5,flash6,flash7,flash8,flash9,flash10,flash11,flash12,flash13,flash14"

for g in 128 #16 64 128
do
    regionSize=$(( GB * g ))

    # start the server on one compute node
    # start clients on a separate compute node
    rm -rf serverfile
    export OMP_NUM_THREADS=24
    srun --nodelist=${serverNode} -n 1 ${EXE}_server $regionSize 0 &

    while [ ! -f serverfile ]; do
	sleep 3
    done

    for cacheRatio in 4 #2 1 
    do
	bufSize=$(( regionSize / cacheRatio ))
    
	for k in  512 1024 2048 #256 128 64 32 16 8 4
	do
	    psize=$(( KB * k ))
	    pages=$(( regionSize / psize))
	    bufPages=$(( bufSize/psize ))

	    for numNodes in 1 #2 3 4 5 6 7 8 9 10 11
	    do
		for numProcPerNode in 3 6 12 24
		do
		    numThreads=$(( 24/numProcPerNode ))
		    cmd="UMAP_PAGESIZE=$psize UMAP_BUFSIZE=$bufPages OMP_NUM_THREADS=$numThreads srun --nodelist=${clientNode} --ntasks-per-node=$numProcPerNode -N $numNodes ${EXE}_client $pages $numUpdates $numPeriods"
		    echo $cmd
		    eval $cmd
		done
	    done
	done
    done
    
    pkill srun
    sleep 3
    
done
echo 'Done'
