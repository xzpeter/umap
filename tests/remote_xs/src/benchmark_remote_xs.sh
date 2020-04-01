#!/bin/bash

# -------------------------------------------------------- #
# Usage
# -------------------------------------------------------- #
# cd path/to/build
# sh path/to/umap-apps/src/remote_xs/benchmark_remote_xs.sh

# -------------------------------------------------------- #
# Input Configuration
# -------------------------------------------------------- #
EXE="./xs"

# -------------------------------------------------------- #
# Machine Configuration
# -------------------------------------------------------- #
serverNode=flash2
clientNode="flash3,flash4,flash5,flash6"
#"flash3,flash4,flash5,flash6,flash7,flash8,flash9,flash10,flash11,flash12,flash13"

margo_lib_dir="/g/g90/peng8/flash/XSBench/umap/build/lib"
lpath="LD_LIBRARY_PATH=${margo_lib_dir}:/usr/tce/packages/openmpi/openmpi-4.0.0-gcc-8.1.0/lib:/usr/tce/packages/python/python-3.7.2/lib:/g/g90/peng8/cuda/lib64:/usr/tce/packages/cuda/cuda-10.0.130/lib64:/g/g90/peng8/cuda/lib64:/usr/tce/packages/cuda/cuda-10.0.130/lib64"

# -------------------------------------------------------- #
# constants
# -------------------------------------------------------- #

K=1024
M=$((K*K))
G=$((K*K*K))

# -------------------------------------------------------- #

# -------------------------------------------------------- #
# Functions
# -------------------------------------------------------- #
print_gcc_version() {
    echo ""
    ret=$(strings $1 | grep "GCC")
    echo ${ret}
    echo ""
}


# -------------------------------------------------------- #
# Run benchmark varying configuration
# -------------------------------------------------------- #
main() {
    # ---- Print some system information ---- #
    print_gcc_version $EXE"_server"

    lookups=15000000
    
    for gridpoints in 22606 45212 90424 180848 361696;do
	
	# ---- Start the server  ---- #
	rm -rf serverfile
	cmd="env $lpath UMAP_PAGESIZE=$((1*M)) srun --nodelist=${serverNode} --ntasks-per-node=1 -N 1 ${EXE}_server -g $gridpoints -l $lookups &"
	echo $cmd
	eval $cmd

	# -----Start the client ----- #
	while [ ! -f serverfile ]; do
            sleep 1
	done

	for numNodes in {4..1..-1}
	do
	    for numOMPThreads in 24 48
	    do
 		for pSize in $((1*M)) $((256*K)) $((64*K)) $((16*K)) $((4*K)) # $((4*K)) $((1*M)) $((4*M)) #umap_page_size
		do
		    #for cacheratio in 1 2 4
		    #do
			#rsize=$(( (num_vertices*33+1)*8 ))
			#aligned_pages=$(( (rsize-1)/pSize + 1 ))
			#echo "aligned pages "$aligned_pages
			#cachepages=$(( aligned_pages/cacheratio ))
			#echo "cached pages "$cachepages
			#cmd="env $lpath UMAP_PAGESIZE=$pSize  UMAP_BUFSIZE=$cachepages srun --nodelist=${clientNode} --ntasks-per-node=1 -N $numNodes $EXE ${BASE_OPTIONS} -t $numOMPThreads"
			cmd="env $lpath UMAP_PAGESIZE=$pSize srun --nodelist=${clientNode} --ntasks-per-node=1 -N $numNodes ${EXE}_client -g $gridpoints -l $lookups -t $numOMPThreads"
			echo $cmd
			time eval $cmd
			rm -rf *.core
		    #done
		done
	    done
	done
    done

    pkill xs_server
    pkill xs_client
    sleep 3
    echo "Done"
}

main "$@"
