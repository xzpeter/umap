//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2019 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////

/*
 * An example showing UMap remote memory regions over network
 */
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <omp.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <chrono>
#include <random>
#include "mpi.h"
#include "errno.h"
#include "umap/umap.h"
#include "umap/store/StoreNetwork.h"

#define ELEMENT_TYPE uint64_t

using namespace std;
using namespace std::chrono;

void reset_index(size_t *arr, size_t len, size_t range);

int main(int argc, char **argv)
{
  if( argc != 4 ){
    printf("Usage: %s [num_pages] [updates_per_period] [num_periods]\n",argv[0]);
    return 0;
  }
  
  const uint64_t umap_pagesize = umapcfg_get_umap_page_size();
  const size_t num_umap_pages = atol(argv[1]);
  const size_t num_updates  = (size_t)(atol(argv[2]));
  const int num_periods = atoi(argv[3]);
  const uint64_t umap_region_length = num_umap_pages * umap_pagesize;
  assert(umap_region_length % umap_pagesize == 0);
  cout << "umap_pagesize "  << umap_pagesize << "\n";
  cout << "umap_region_length "  << umap_region_length << "\n";
  char hostname[256];
  if( gethostname(hostname, sizeof(hostname)) ==0 ) 
      cout << "hostname " << hostname << "\n";

  
  /* bootstraping to determine server and clients usnig MPI */
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
  /*Create a network-based datastore*/
  Umap::Store* datastore  = new Umap::StoreNetwork(umap_region_length);

  /* assume rank 0 is used for RPC server for now */
  /* TODO */
  if( rank !=0 ){
    
    /* map to the remote memory region */
    auto timing_map_st = high_resolution_clock::now();
    void* region_addr = NULL; //to be set by umap
    int   prot        = PROT_READ;
    int   flags       = UMAP_PRIVATE;
    int   fd          = -1;
    off_t offset      = 0;
    void* base_addr = umap_ex(region_addr, umap_region_length, prot, flags, fd, offset, datastore);
    auto timing_map_end = high_resolution_clock::now();
    if ( base_addr == UMAP_FAILED ) {
      int eno = errno;
      std::cerr << "Failed to umap network" << ": " << strerror(eno) << std::endl;
      return 0;
    }
    auto timing_map = duration_cast<microseconds>(timing_map_end - timing_map_st);
    cout << "umap base_addr at "<< base_addr << ", Time taken [us]: "<< timing_map.count() <<"\n"<<std::flush;

  
    /* Main loop: update num_updates times to the buffer for num_periods times */
    const size_t num_elements = umap_region_length/sizeof(ELEMENT_TYPE);
    cout << "Start reading ["<< (umap_region_length/1024.0/1024.0) <<" MB ] array, "
	 << num_updates <<" reads per period x "<< num_periods <<std::endl;

    ELEMENT_TYPE sum = 0;
    double rates[num_periods];
    size_t idx[num_updates];
    ELEMENT_TYPE *arr = (ELEMENT_TYPE *) base_addr;
    
    for( int p=0; p<num_periods; p++ ){

      reset_index(&idx[0], num_updates, num_elements);
      //size_t offset = p*num_updates;
    
      auto timing_update_st = high_resolution_clock::now();
#pragma omp parallel for
      for(size_t i=0; i < num_updates; i++){
	//random read
	size_t id = idx[i];
	sum += arr[id];

	//sequential
	//sum += arr[offset+i]; 
      }
      auto timing_update_end = high_resolution_clock::now();
      auto timing_update = duration_cast<microseconds>(timing_update_end - timing_update_st);
      rates[p]=num_updates*1000000.0/timing_update.count();
      cout << "Period["<< p<<"] Time : "<< timing_update.count() <<" [us], " <<rates[p]<<" updates per second\n"<<std::flush;
    }
    /* End of Main Loop */


    /* Unmap file */
    if (uunmap(base_addr, umap_region_length) < 0) {
      int eno = errno;
      std::cerr << "Failed to unmap network datastore: " << strerror(eno) << endl;
      return -1;
    }
  }
  

  MPI_Barrier(MPI_COMM_WORLD);
  /* Free the network dastore */
  //delete datastore;
  
  return 0;
}

/* prepare LEN random index in [0, range] */
void reset_index(size_t *arr, size_t len, size_t range){

    std::random_device rd; //seed
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned long> dis(0, range-1); //[a, b]
 
    for (size_t i = 0; i < len; i++) {
      arr[i] = dis(gen); 
      //cout << arr[i] <<endl;
    }
}
