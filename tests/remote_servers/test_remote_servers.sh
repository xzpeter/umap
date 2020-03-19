#!/bin/bash

EXE=./tests/remote_servers/remote

KB=1024
MB=$((1024*KB))
GB=$((1024*MB))

numPeriods=3

serverNode="flash2,flash3,flash4,flash5"
clientNode=flash6
#"flash3,flash4,flash5,flash6,flash7,flash8,flash9,flash10,flash11,flash12,flash13"

for g in  1 8 16 64 
do
    regionSize=$(( GB * g ))

    for numNodes in 4 2 1 #1 2 3 4 5 6 7 8 9 10 11
    do
	for numProcPerNode in 1 2 4
	do
	    # start the server on N compute node
	    numThreads=1
	    regionSize_per_server=$(( regionSize / numProcPerNode / numNodes ))
	    cmd="OMP_NUM_THREADS=$numThreads srun --nodelist=${serverNode} --ntasks-per-node=$numProcPerNode -N $numNodes ${EXE}_servers $regionSize_per_server &"
	    rm -rf serverfile
	    echo $cmd
	    eval $cmd

	    
	    # start clients on a separate compute node
	    while [ ! -f serverfile ]; do
		sleep 1
	    done

	    for cacheRatio in 1 2 4
	    do
		bufSize=$(( 2 * regionSize / cacheRatio ))
    	    
		for k in  256 64 16 #256 128 64 32 16 8 4
		do
		    psize=$(( KB * k ))
		    bufPages=$(( bufSize/psize ))
	    
		    for numProcPerNode in 1 2 4
		    do
			for numThreads in 1 2 4 #$(( 24/numProcPerNode ))
			do
			    cmd="UMAP_PAGESIZE=$psize UMAP_BUFSIZE=$bufPages OMP_NUM_THREADS=$numThreads srun --nodelist=${clientNode} --ntasks-per-node=$numProcPerNode -N 1 ${EXE}_clients $regionSize $numPeriods"
			    echo $cmd
			    eval $cmd
			done
		    done
		done
	    done

	    pkill srun
	    sleep 3
	    
	done
    done
done
echo 'Done'
