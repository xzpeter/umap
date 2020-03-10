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

  StoreNetworkServer::StoreNetworkServer(void* _ptr, std::size_t _rsize_, std::size_t _num_clients)
    :StoreNetwork(_rsize_, true, _num_clients)
  {
    init_servers();
    
    setup_server_buffer(_ptr, _rsize_);

    start_server(_num_clients);

    UMAP_LOG(Info, "Server is setup");
  }

  StoreNetworkServer::~StoreNetworkServer()
  {
    fini_servers();
  }
  
  StoreNetworkClient::StoreNetworkClient(std::size_t _rsize_)
    :StoreNetwork(_rsize_, false)
  {
    init_client();
    
    UMAP_LOG(Info, "Client is setup");
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
    
  StoreNetwork::StoreNetwork( std::size_t _rsize_ , bool _is_server, std::size_t _num_clients)
    :rsize(_rsize_), is_server(_is_server), num_clients(_num_clients)
  {
    
    /* bootstraping to determine server and clients usnig MPI */
    /* not needed if MPI protocol is not used */
    int flag_mpi_initialized;
    MPI_Initialized(&flag_mpi_initialized);
    if( !flag_mpi_initialized )
      MPI_Init(NULL, NULL);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    UMAP_LOG(Info, "MPI rank " << rank << " is_server=" << is_server);

    
    /* Lookup the server address */
    if(is_server){

      //init_servers(rsize, _num_clients);
      
      /* Ensure that client setup after the server has */
      /* published their addresses */
      //MPI_Barrier(MPI_COMM_WORLD);
      //UMAP_LOG(Info, "Server is setup");
      
    }else{

      /* Ensure that client setup after the server has */
      /* published their addresses */
      //MPI_Barrier(MPI_COMM_WORLD);
      //init_client();
      //UMAP_LOG(Info, "Client is setup");
    }

  }

  StoreNetwork::~StoreNetwork(){
    
    UMAP_LOG(Info, "Base Destructor ...");
    
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

  ssize_t  StoreNetwork::write_to_store(char* buf, size_t nb, off_t off)
  {
    size_t rval = 0;
    std::exit(EXIT_SUCCESS);

    return rval;
  }

}
