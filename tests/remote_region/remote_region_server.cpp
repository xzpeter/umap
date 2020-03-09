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
    
  /*Create a network-based datastore*/
  Umap::Store* datastore  = new Umap::StoreNetwork(umap_region_length, 1, num_clients);
  

  /* Free the network dastore */
  delete datastore;
  
  return 0;
}
