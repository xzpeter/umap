//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2019 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////

/*
 * It is a simple example showing flush_cache ensures that 
 * modified pages in buffer is persisted into back stores.
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
#include "errno.h"
#include "umap/umap.h"

#define FLUSH_BUF 1
#define ELEMENT_TYPE uint64_t
//#define UMAP 1
//#define VERIFICATION

using namespace std;
using namespace std::chrono;

int open_prealloc_file( const char* fname, uint64_t totalbytes);
void reset_index(size_t *arr, size_t len, size_t range);

int main(int argc, char **argv)
{
  if( argc != 5 ){
    printf("Usage: %s [filename] [num_pages] [updates_per_period] [num_periods]\n",argv[0]);
    return 0;
  }
  
  const char* filename = argv[1];
  const size_t num_umap_pages = atol(argv[2]);
  const size_t num_updates  = (size_t)(atol(argv[3]));
  const int num_periods = atoi(argv[4]);
  
  uint64_t umap_pagesize = umapcfg_get_umap_page_size();
  const uint64_t umap_region_length = num_umap_pages * umap_pagesize;
  assert(umap_region_length % umap_pagesize == 0);
  cout << "umap_pagesize "  << umap_pagesize << "\n";
  cout << "umap_region_length "  << umap_region_length << "\n";
  
  int fd = open_prealloc_file(filename, umap_region_length);

  /* map the file */
  auto timing_map_st = high_resolution_clock::now();
#ifdef UMAP
  void* base_addr = umap(NULL, umap_region_length, PROT_READ|PROT_WRITE, UMAP_PRIVATE, fd, 0);
  if( base_addr == UMAP_FAILED){
#else
  void* base_addr = mmap(NULL, umap_region_length, PROT_READ|PROT_WRITE, MAP_SHARED,  fd, 0);
  if( base_addr == MAP_FAILED){
#endif    
    int eno = errno;
    std::cerr << "Failed to map " << filename << ": " << strerror(eno) << endl;
    return -1;
  }
  auto timing_map_end = high_resolution_clock::now();
  auto timing_map = duration_cast<microseconds>(timing_map_end - timing_map_st);
  cout << "umap base_addr at "<< base_addr << ", Time taken [us]: "<< timing_map.count() <<"\n"<<std::flush;

  
  /* Main loop: update num_updates times to the buffer for num_periods times */
  const size_t num_elements = umap_region_length/sizeof(ELEMENT_TYPE);
  cout << "Start Updating Array of "<< (umap_region_length/1024.0/1024.0/1024.0) 
       <<" GB ("<<num_elements<<", "<< num_updates << " updates per period) x "
       << num_periods <<"\n"<<std::flush;
  
  double* rates = (double*)malloc(sizeof(double)*num_periods);
  size_t*  idx  = (size_t*)malloc(sizeof(size_t)*num_updates);
  ELEMENT_TYPE *arr = (ELEMENT_TYPE *) base_addr;
  for( int p=0; p<num_periods; p++ ){

    reset_index(idx, num_updates, num_elements);
    
    auto timing_update_st = high_resolution_clock::now();
#pragma omp parallel for
    for(size_t i=0; i < num_updates; i++){
      size_t id = idx[i];
      arr[id] = (ELEMENT_TYPE)(id);
    }
    auto timing_update_end = high_resolution_clock::now();
    auto timing_update = duration_cast<microseconds>(timing_update_end - timing_update_st);
    
    rates[p]=num_updates*1000000.0/timing_update.count();
    cout << "Periodic update Time taken [us]: "<< timing_update.count() <<", " <<rates[p]<<" updates per second\n"<<std::flush;

#if false
    auto time_st = high_resolution_clock::now();
#ifdef UMAP
    int ret = umap_flush() ;
#else
    int ret = msync(base_addr, (size_t)umap_region_length, MS_SYNC);
#endif
    auto time_end = high_resolution_clock::now();
    auto time = duration_cast<microseconds>(time_end - time_st);
    cout << " Periodic sync Time taken [us]: " << time.count() << "\n" << std::flush;
#endif
  }
  /* End of Main Loop */


  /* Sync changes to file */
#ifdef  FLUSH_BUF
  auto time_st = high_resolution_clock::now();
#ifdef UMAP
  int ret = umap_flush() ;
#else
  int ret = msync(base_addr, (size_t)umap_region_length, MS_SYNC);
#endif
  auto time_end = high_resolution_clock::now();
  auto time = duration_cast<microseconds>(time_end - time_st);
#ifdef UMAP
  cout << "umap_flush [";
#else
  cout << "msync [";
#endif
  cout << base_addr << ", " << umap_region_length  << "] Time taken [us]: " << time.count() << "\n" << std::flush;
#endif


#ifdef VERIFICATION
  /* Verification */
  cout << "Verify Array in file and in-core\n";
  ifstream rf(filename, ios::in | ios::binary);
  if(!rf) {
    cout << "Cannot open file!" << endl;
    exit(1);
  }
  size_t block = (1UL<<32); //allocate a GB block buffer
  ELEMENT_TYPE* arr_in = (ELEMENT_TYPE*)malloc(block);
  size_t num_blocks = umap_region_length/block;
  stride = block/sizeof(ELEMENT_TYPE);
  cout <<" block "<< block <<" num_blocks "<< num_blocks << " stride "<< stride << endl;
  if(!arr_in){
    cout << "unable to allocate buffer for verification, go to done \n" << std::flush;
    goto fini;
  }  
  for(size_t i = 0; i<num_blocks; i++){
    rf.seekg(i*block, ios::beg);
    rf.read((char *)arr_in, block);
    if (!rf)
      cout << "error: only " << rf.gcount() << " could be read \n" << std::flush;
      
    size_t offset = i*stride;
#pragma omp parallel for
    for(size_t i=0; i < stride; i++) {
      if( arr_in[i]!=arr[i + offset] ){
	cout << "file Array["<< i <<"]: " <<arr_in[i] << ", in-core Array["<< i <<"] = "<< arr[i] << endl;
      }
    }
  }
  cout << "Verification PASSED \n";
  rf.close();
  free(arr_in);
#endif

  /* Unmap file */
#ifdef UMAP
  if (uunmap(base_addr, umap_region_length) < 0) {
#else
  if (munmap(base_addr, umap_region_length) < 0) {
#endif
    int eno = errno;
    std::cerr << "Failed to unmap file " << filename << ": " << strerror(eno) << endl;
    return -1;
  }

  fini:  
  close(fd);
  free(rates);
  free(idx);
  return 0;
}


  int open_prealloc_file( const char* fname, uint64_t totalbytes)
{
  
  int status = unlink(fname);
  int eno = errno;
  if ( status!=0 && eno != ENOENT ) {
    std::cerr << "Failed to unlink " << fname << ": "<< strerror(eno) << " Errno=" << eno <<endl;
    exit(1);
  }
 
  
  int fd = open(fname, O_RDWR | O_LARGEFILE | O_DIRECT | O_CREAT, S_IRUSR | S_IWUSR);
  if ( fd == -1 ) {
    int eno = errno;
    std::cerr << "Failed to create " << fname << ": " << strerror(eno) << endl;
    exit(1);
  }

  /* Pre-allocate disk space for the file.*/
  try {
    int x;
    if ( ( x = posix_fallocate(fd, 0, totalbytes) != 0 ) ) {
      int eno = errno;
      std::cerr << "Failed to pre-allocate " << fname << ": " << strerror(eno) << endl;
      exit(1);
    }
  } catch(const std::exception& e) {
    std::cerr << "posix_fallocate: " << e.what() << endl;
    exit(1);
  } catch(...) {
    int eno = errno;
    std::cerr << "Failed to pre-allocate " << fname << ": " << strerror(eno) << endl;
    exit(1);
  }
 
  return fd;
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
