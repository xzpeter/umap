#!/bin/bash

EXE=./tests/remote_lookup/remote_lookup

KB=1024
MB=$((1024*KB))
GB=$((1024*MB))

numUpdates=100
numPeriods=100

for g in 128 #16 64 128
do
    regionSize=$(( GB * g ))

    # start the server on one compute node
    # start clients on a separate compute node
    for numServerNodes in 2 1 #4 3 2 1
    do
	for numServerProcPerNode in 1 2 4 8
	do
	    rm -rf serverfile
	    export OMP_NUM_THREADS=$(( 24/numServerProcPerNode ))
	    cmd="srun --ntasks-per-node=$numServerProcPerNode -N $numServerNodes ${EXE}_server $regionSize & "
	    echo $cmd
	    eval $cmd

	    # start clients after the server has published their ports
	    while [ ! -f serverfile ]; do
		sleep 3
	    done

	    for k in 256 4
	    do
		psize=$(( KB * k ))
		    
		for numClientNodes in 1 #{1..9..1}
		do
		for numClientProcPerNode in 1 2 4 8
		do
		    numClientThreads=$(( 24/numClientProcPerNode ))
		    cmd="UMAP_PAGESIZE=$psize OMP_NUM_THREADS=$numClientThreads srun --ntasks-per-node=$numClientProcPerNode -N $numClientNodes ${EXE}_client $regionSize $numUpdates $numPeriods"
		    echo ""
		    echo $cmd
		    eval $cmd
			
	#	    for cacheRatio in 4 #2 1 
	#	    do
	#		pages=$(( regionSize / psize))
	#		bufPages=$(( pages/cacheRatio ))
	#		cmd="UMAP_PAGESIZE=$psize UMAP_BUFSIZE=$bufPages OMP_NUM_THREADS=$numThreads srun --ntasks-per-node=$numClientProcPerNode -N $numClientNodes ${EXE}_client $regionSize $numUpdates $numPeriods"
	#		echo $cmd
		#	eval $cmd
		    #done
		done
		done
	    done
	done
    done
    pkill srun
    sleep 3
    
done
echo 'Done'
