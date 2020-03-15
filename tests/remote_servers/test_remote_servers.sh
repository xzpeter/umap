#!/bin/bash

EXE=./tests/remote_stream/remote_stream

KB=1024
MB=$((1024*KB))
GB=$((1024*MB))

numPeriods=3

serverNode=flash2
clientNode="flash3,flash4,flash5,flash6,flash7,flash8,flash9,flash10,flash11,flash12,flash13"

for g in 64 #128  1 8 16 
do
    regionSize=$(( GB * g ))

    # start the server on one compute node
    # start clients on a separate compute node
    rm -rf serverfile
    export OMP_NUM_THREADS=24
    srun --nodelist=${serverNode} -n 1 ${EXE}_server $regionSize &

    while [ ! -f serverfile ]; do
	sleep 1
    done

    for cacheRatio in 1 2 4
    do
	bufSize=$(( 2 * regionSize / cacheRatio ))
    
	for numNodes in 11 #1 2 3 4 5 6 7 8 9 10 11
	do
	    
	    for k in  256 128 64 32 16 8 4
	    do
		psize=$(( KB * k ))
		pages=$(( regionSize / psize))
		bufPages=$(( bufSize/psize ))
	    
		for numProcPerNode in 1 2 4 #12 24
		do
		    for numThreads in 1 2 4 #$(( 24/numProcPerNode ))
		    do
			cmd="UMAP_PAGESIZE=$psize UMAP_BUFSIZE=$bufPages OMP_NUM_THREADS=$numThreads srun --nodelist=${clientNode} --ntasks-per-node=$numProcPerNode -N $numNodes ${EXE}_client $regionSize $numPeriods"
			#cmd="UMAP_PAGESIZE=$psize OMP_NUM_THREADS=$numThreads srun --nodelist=${clientNode} --ntasks-per-node=$numProcPerNode -N $numNodes ${EXE}_client $regionSize $numPeriods"
			echo $cmd
			eval $cmd
		    done
		done
	    done
	done
    done
    
    pkill srun
    sleep 3
    
done
echo 'Done'
