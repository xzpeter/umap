#!/bin/bash

EXE=./tests/remote_stream/remote_stream

KB=1024
MB=$((1024*KB))
GB=$((1024*MB))

numNodes=2
numUpdates=100
numPeriods=10

serverNode=flash1 #flash2
clientNode=flash1 #"flash3,flash5,flash6,flash7,flash8,flash9,flash10,flash11,flash12,flash13,flash14"

for g in 1 #16 64 128
do
    regionSize=$(( GB * g ))

    # start the server on one compute node
    # start clients on a separate compute node
    rm -rf serverfile
    export OMP_NUM_THREADS=24
    srun --nodelist=${serverNode} -n 1 ${EXE}_server $regionSize &

    while [ ! -f serverfile ]; do
	sleep 2
    done

    for cacheRatio in 100 #25 50 100
    do
	bufSize=$(( 2 * regionSize * cacheRatio / 100 ))
    
	for k in  256 #128 64 32 16 8 4
	do
	    psize=$(( KB * k ))
	    pages=$(( regionSize / psize))
	    bufPages=$(( bufSize/psize ))

	    for numNodes in 1 #2 3 4 5 6 7 8 9 10 11
	    do
		numProcPerNode=1 #3 6 12 24
		numThreads=4 #$(( 24/numProcPerNode ))
		#cmd="UMAP_PAGESIZE=$psize UMAP_BUFSIZE=$bufPages OMP_NUM_THREADS=$numThreads srun --nodelist=${clientNode} --ntasks-per-node=$numProcPerNode -N $numNodes ${EXE}_client $regionSize $numPeriods"
		cmd="UMAP_PAGESIZE=$psize OMP_NUM_THREADS=$numThreads srun --nodelist=${clientNode} --ntasks-per-node=$numProcPerNode -N $numNodes ${EXE}_client $regionSize $numPeriods"
		echo $cmd
		eval $cmd
	    done
	done
    done
    
    pkill srun
    sleep 3
    
done
echo 'Done'
