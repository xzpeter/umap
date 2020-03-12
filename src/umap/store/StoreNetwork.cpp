//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2020 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>
#include "StoreNetwork.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include <cassert>

#include "umap/store/Store.hpp"
#include "umap/util/Macros.hpp"

#include <mpi.h>
#include "rpc_server.hpp"
#include "rpc_client.hpp"


namespace Umap {

  int StoreNetwork::client_id =-1;
  int StoreNetwork::server_id =-1;
  std::map<const char*, RemoteMemoryObject> StoreNetwork::remote_memory_pool;
  bool has_server_setup = false;
  
  StoreNetworkServer::StoreNetworkServer(const char* _id,
					 void* _ptr,
					 std::size_t _rsize_,
					 std::size_t _num_clients)
    :StoreNetwork(_rsize_, true),id(_id)
  {
    /* setup Margo connect */
    /* is done once only */
    if( !has_server_setup ){
      init_servers();
      has_server_setup = true;
    }

    /* Register the remote memory object to the pool */
    if( remote_memory_pool.find(id)!=remote_memory_pool.end() ){
      UMAP_ERROR("Cannot create datastore with duplicated name: "<< id);
    }
    remote_memory_pool.emplace(id, RemoteMemoryObject(_ptr,_rsize_));

    print_memory_pool();
  }

  StoreNetworkServer::~StoreNetworkServer()
  {
    UMAP_LOG(Info, "Deleting: " << id);
    assert(remote_memory_pool.find(id)!=remote_memory_pool.end());
    remote_memory_pool.erase(id);
    print_memory_pool();

    if(remote_memory_pool.size()==0){
      UMAP_LOG(Info, "shuting down...");
      fini_servers();
    }
  }
  
  StoreNetworkClient::StoreNetworkClient(const char* id, std::size_t _rsize_)
    :StoreNetwork(_rsize_, false)
  {

    /* setup Margo connect */
    /* is done once only */
    init_client();
    /* Register the remote memory object to the pool */
    //setup_server_buffer(_ptr, _rsize_);
    if( remote_memory_pool.find(id)!=remote_memory_pool.end() ){
      UMAP_ERROR("Cannot create datastore with duplicated name: "<< id);
    }
    remote_memory_pool.emplace(id, RemoteMemoryObject(NULL,_rsize_));
    
  }

  StoreNetworkClient::~StoreNetworkClient()
  {
    UMAP_LOG(Info, "Client Destructor");
    /* send a request of 0 byte to the server to signal termination */
    int   server_id = 0;      
    read_from_server(server_id, NULL, 0, 0);

    /* Free resouces */
    fini_client();
  }
    
  StoreNetwork::StoreNetwork( std::size_t _rsize_ , bool _is_server)
    :rsize(_rsize_), is_server(_is_server)
  {
    
    /* bootstraping to determine server and clients usnig MPI */
    /* not needed if MPI protocol is not used */
    int flag_mpi_initialized;
    MPI_Initialized(&flag_mpi_initialized);
    if( !flag_mpi_initialized )
      MPI_Init(NULL, NULL);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    /* Lookup the server address */
    if(is_server){
      server_id = rank;
      //init_servers(rsize, _num_clients);
      
      /* Ensure that client setup after the server has */
      /* published their addresses */
      //MPI_Barrier(MPI_COMM_WORLD);
      //UMAP_LOG(Info, "Server is setup");
      
    }else{
      client_id = rank;
      /* Ensure that client setup after the server has */
      /* published their addresses */
      //MPI_Barrier(MPI_COMM_WORLD);
      //init_client();
      //UMAP_LOG(Info, "Client is setup");
    }

  }

  StoreNetwork::~StoreNetwork()
  {
    
    UMAP_LOG(Info, "Base Destructor ...");
    
  }

  void StoreNetwork::print_memory_pool()
  {
    for(auto it : remote_memory_pool)
      UMAP_LOG(Info, "remote_memory_pool["<<it.first<<"] :: "<<
	       (it.second).ptr << ", " <<(it.second).rsize );
  }
  
  ssize_t StoreNetwork::read_from_store(char* buf, size_t nbytes, off_t offset)
  {
    /* Only client should receive filler work items*/
    assert( !is_server);
    
    size_t rval = 0;

    void* buf_ptr = (void*) buf;
    int   server_id = 0;
    read_from_server(server_id, buf_ptr, nbytes, offset);
    
    return rval;
  }

  ssize_t  StoreNetwork::write_to_store(char* buf, size_t nbytes, off_t offset)
  {
    /* TODO: coexist server and client */
    assert( !is_server);
    
    size_t rval = 0;

    void* buf_ptr = (void*) buf;
    int   server_id = 0;
    write_to_server(server_id, buf_ptr, nbytes, offset);
    
    return rval;

  }

}
