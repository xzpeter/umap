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
  if( argc != 3 ){
    printf("Usage: %s [memory_region_bytes] [num_clients]\n",argv[0]);
    return 0;
  }
  
  const uint64_t umap_region_length =  atoll(argv[1]);
  const int num_clients =  atoi(argv[2]);

  cout << "umap_region_length = "  << umap_region_length << " bytes \n";
  cout << "umap clients = "  << num_clients << "\n";
  
  char hostname[256];
  if( gethostname(hostname, sizeof(hostname)) ==0 ) 
      cout << "hostname " << hostname << "\n";
  
  /* bootstraping to determine server and clients usnig MPI */
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  /* Prepare memory resources on the server */
  void* server_buffer = malloc(umap_region_length);
  if(!server_buffer){
    std::cerr<<" Unable to allocate " << umap_region_length << " bytes on the server";
    return 0;
  }
  
  /* initialization function should be user defined*/
  uint64_t *arr = (uint64_t*) server_buffer;
  size_t num = umap_region_length/sizeof(uint64_t);
#pragma omp parallel for
  for(size_t i=0;i<num;i++)
    arr[i]=i;

  /* Create a network-based datastore */
  /* 0 num_clients leaves the server on */
  Umap::Store* datastore  = new Umap::StoreNetworkServer(server_buffer,
							 umap_region_length,
							 num_clients);
  

  /* Free the network dastore */
  delete datastore;
    
  free(server_buffer);
  
  return 0;
}
