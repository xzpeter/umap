//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2019 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////

/*
 * An example of multiple remote memory objects over network
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
  if( argc != 2 ){
    printf("Usage: %s [per_array_bytes]\n",argv[0]);
    return 0;
  }
  
  const uint64_t array_length =  atoll(argv[1]);
  cout << "remote STREAM benchmark:: array_length = " << array_length << " bytes \n";
  
  char hostname[256];
  if( gethostname(hostname, sizeof(hostname)) ==0 ) 
      cout << "hostname " << hostname << "\n";
  
  /* bootstraping to determine server and clients usnig MPI */
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  /* Prepare memory resources on the server */
  void* arr_a = malloc(array_length);
  void* arr_b = malloc(array_length);
  if( !arr_a || !arr_b ){
    std::cerr<<" Unable to allocate arrays on the server";
    return 0;
  }
  
  /* initialization function should be user defined */
  uint64_t *arr0 = (uint64_t*) arr_a;
  uint64_t *arr1 = (uint64_t*) arr_b;
  size_t num = array_length/sizeof(uint64_t);
#pragma omp parallel for
  for(size_t i=0;i<num;i++){
    arr0[i]=1;
    arr1[i]=2;
  }

  /* Create two network-based datastores */
  int num_clients = 0;
  Umap::Store* ds0  = new Umap::StoreNetworkServer("arr_a", arr0, array_length);
  std::cout << "ds0 is Registed " << std::endl;
  
  Umap::Store* ds1  = new Umap::StoreNetworkServer("arr_b", arr1, array_length);
  std::cout << "ds1 is Registed " << std::endl;

  int periods = 50;
  while((periods)>0){

    std::cout << " Application computing ... ["<< (5-periods) << "/5]" << std::endl;
    sleep(3);
  }

  
  /* Free the network dastore */
  /* wait utill all clients are done */
  delete ds0;
  delete ds1;
  
  free(arr_a);
  free(arr_b);

  MPI_Finalize();
  
  return 0;
}
