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
#include "rpc_util.hpp"
#include "rpc_server.hpp"
#include "rpc_client.hpp"


namespace Umap {
  bool has_server_setup = false;
  bool has_client_setup = false;
  
  /*
   * Constructor
   * create a remote memory object on the server 
   * init the server connection if not setup
   */
  StoreNetworkServer::StoreNetworkServer(const char* _id,
					 void* _ptr,
					 std::size_t _rsize_,
					 std::size_t _num_clients)
    :StoreNetwork(_id, _rsize_, true)
  {
    /* setup Margo connect */
    /* is done once only */
    if( !has_server_setup ){
          
      /* bootstraping to determine server and clients usnig MPI */
      /* not needed if MPI protocol is not used */
      int flag_mpi_initialized;
      MPI_Initialized(&flag_mpi_initialized);
      if( !flag_mpi_initialized )
	MPI_Init(NULL, NULL);
      MPI_Comm_rank(MPI_COMM_WORLD, &server_id);
      
      init_servers();
      has_server_setup = true;
    }

    /* Try to register the new resource */
    int ret = server_add_resource(id, _ptr, rsize);
    assert( ret==0 );
  }

  StoreNetworkServer::~StoreNetworkServer()
  {
    UMAP_LOG(Info, "Server "<< server_id << " is deleting: " << id);


    /* Try to remove the new resource */
    int ret = server_delete_resource(id);
    assert( ret==0 );

  }
  
  StoreNetworkClient::StoreNetworkClient(const char* _id, std::size_t _rsize_)
    :StoreNetwork(_id, _rsize_, false)
  {
    
    /* setup Margo connect */
    /* is done once only */
    if( !has_client_setup ){
      int flag_mpi_initialized;
      MPI_Initialized(&flag_mpi_initialized);
      if( !flag_mpi_initialized )
        MPI_Init(NULL, NULL);
      MPI_Comm_rank(MPI_COMM_WORLD, &client_id);
      
      init_client();
      has_client_setup = true;
    }

    /* Try to register the new resource */
    if( client_check_resource(id) ){

      /* Get request approval from the server */
      bool has_server_accepted = client_request_resource(id, rsize);
      if( !has_server_accepted ){
	UMAP_ERROR("Cannot request "<< id << ", rejected by the server ");
      }
      
      client_add_resource(id, _ptr, rsize);

    }else{
      UMAP_ERROR("Cannot create datastore with duplicated name: "<< id);
    }
  }

  StoreNetworkClient::~StoreNetworkClient()
  {

    UMAP_LOG(Info, "Client "<< client_id <<" deleting: " << id);
    
    /* send a request of 0 byte to the server to signal termination */
    int   server_id = 0;      
    client_read_from_server(server_id, NULL, 0, 0);

    /* Try to remove the new resource */
    int ret = client_delete_resource(id);
    assert( ret==0 );

  }
    
  StoreNetwork::StoreNetwork( const char* _id,
			      std::size_t _rsize_,
			      bool _is_on_server)
    :id(_id),
     rsize(_rsize_),
     is_on_server(_is_on_server)
  {
    
    /* Lookup the server address */
    if(is_on_server){

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

  StoreNetwork::~StoreNetwork()
  {    
  }
  
  ssize_t StoreNetwork::read_from_store(char* buf, size_t nbytes, off_t offset)
  {
    /* Only client should receive filler work items*/
    assert( !is_on_server);
    
    size_t rval = 0;

    void* buf_ptr = (void*) buf;
    int   server_id = 0;
    read_from_server(server_id, buf_ptr, nbytes, offset);
    
    return rval;
  }

  ssize_t  StoreNetwork::write_to_store(char* buf, size_t nbytes, off_t offset)
  {
    /* TODO: coexist server and client */
    assert( !is_on_server);
    
    size_t rval = 0;

    void* buf_ptr = (void*) buf;
    int   server_id = 0;
    write_to_server(server_id, buf_ptr, nbytes, offset);
    
    return rval;

  }

}
