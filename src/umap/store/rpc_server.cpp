//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2020 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////
#include <cassert>
#include <unistd.h>
#include "umap/util/Macros.hpp"
#include "rpc_server.hpp"
#include "rpc_util.hpp"

static const char* PROTOCOL_MARGO_SHM   = "na+sm://";
static const char* PROTOCOL_MARGO_VERBS = "ofi+verbs://";
static const char* PROTOCOL_MARGO_TCP   = "bmi+tcp://";
static const char* PROTOCOL_MARGO_MPI   = "mpi+static";

void publish_server_addr(const char* addr)
{
    /* write server address to local file for client to read */
    FILE* fp = fopen(LOCAL_RPC_ADDR_FILE, "w+");
    if (fp != NULL) {
        fprintf(fp, "%s", addr);
        fclose(fp);
    } else {
      UMAP_ERROR("Error writing server rpc addr file "<<LOCAL_RPC_ADDR_FILE);
    }
}


/* 
 * The read rpc is executed on the server 
 * when the client request arrives
 * it starts bulk transfer to the client
 * when it returns, it callls client's rpc complete 
 * callback function if defined in HG_Foward()
 */
static int umap_server_read_rpc(hg_handle_t handle)
{
  UMAP_LOG(Debug, "Entering");

  assert(mid != MARGO_INSTANCE_NULL);
  
  hg_return_t ret;

  /* get Mercury info */
  /* margo instance id is similar to mercury context */
  const struct hg_info* info = margo_get_info(handle);
  assert(info);
  margo_instance_id mid = margo_hg_info_get_instance(info);
  assert(mid != MARGO_INSTANCE_NULL);
  
    
  /* Get input parameter in umap_server_read_rpc */
  umap_read_rpc_in_t input;
  ret = margo_get_input(handle, &input);
  if(ret != HG_SUCCESS){
    UMAP_ERROR("failed to get rpc intput");
  }

  UMAP_LOG(Debug, "request "<<input.size<<" bytes at offset "<< input.offset);
  
  /* the client signal termination
  * there is no built in functon in margo
  * to inform the server that all clients have completed
  */
  bool is_terminating = (input.size==0);
  if (is_terminating){

    num_completed_clients ++;
    goto fini;
    
  }else{

    /* register memeory for bulk transfer */
    /* TODO: multiple bulk handlers might been */
    /*       created on overlapping memory regions */
    /*       Reuse bulk handle or merge multiple buffers into one bulk handle*/
    hg_bulk_t server_bulk_handle;
    assert( (input.offset+input.size) <= server_buffer_length );
    void* server_buffer_ptr = (char*)server_buffer + input.offset;
    void **buf_ptrs = (void **) &(server_buffer_ptr);
    ret = margo_bulk_create(mid,
			 1, buf_ptrs,&(input.size),
			 HG_BULK_READ_ONLY,
			 &server_bulk_handle);
    if(ret != HG_SUCCESS){
      UMAP_ERROR("Failed to create bulk handle on server");
    }

    UMAP_LOG(Debug,"start bulk transfer");
    /* initiate bulk transfer from server to client */
    /* margo_bulk_transfer is a blocking version of */
    /* that only returns when HG_Bulk_transfer complete */
    ret = margo_bulk_transfer(mid, HG_BULK_PUSH,
			      info->addr, input.bulk_handle, 0,  //client address, bulk handle, offset
			      server_bulk_handle, 0, input.size);//server bulk handle, offset, size of transfer
    if(ret != HG_SUCCESS){
      UMAP_ERROR("Failed to bulk transfer from server to client");
    }
    UMAP_LOG(Debug,"end bulk transfer");


    /* Inform the client side */
    umap_read_rpc_out_t output;
    output.ret  = 1234;
    ret = margo_respond(handle, &output);
    assert(ret == HG_SUCCESS);
    margo_bulk_free(server_bulk_handle);
  }

  
 fini:
    /* free margo resources */
    ret = margo_free_input(handle, &input);
    assert(ret == HG_SUCCESS);
    ret = margo_destroy(handle);
    assert(ret == HG_SUCCESS);
    UMAP_LOG(Debug, "Exiting");

    /* stop the server only if the clients specify the total number of clients */
    if(  num_clients>0 && num_completed_clients == num_clients)
      fini_servers();

    return 0;
}
DEFINE_MARGO_RPC_HANDLER(umap_server_read_rpc)



static void setup_margo_server(){

  /* Init Margo using different transport protocols */
  int use_progress_thread = 1;//flag to use a dedicated thread for running Mercury's progress loop. 
  int rpc_thread_count = -1; //number of threads for running rpc calls
  mid = margo_init(PROTOCOL_MARGO_VERBS,
		   MARGO_SERVER_MODE,
		   use_progress_thread,
		   rpc_thread_count);
  if (mid == MARGO_INSTANCE_NULL) {
    UMAP_ERROR("margo_init protocol "<<PROTOCOL_MARGO_VERBS<<" failed");
  }
  UMAP_LOG(Info, "margo_init done");

  
  /* Find the address of this server */
  hg_addr_t addr;
  hg_return_t ret = margo_addr_self(mid, &addr);
  if (ret != HG_SUCCESS) {
    UMAP_ERROR("margo_addr_self failed");
    margo_finalize(mid);
  }

  /* Convert the server address to string*/
  char addr_string[128];
  hg_size_t addr_string_len = sizeof(addr_string);
  ret = margo_addr_to_string(mid, addr_string, &addr_string_len, addr);
  if (ret != HG_SUCCESS) {
    UMAP_ERROR("margo_addr_to_string failed");
    margo_addr_free(mid, addr);
    margo_finalize(mid);
  }
  UMAP_LOG(Info, "Margo RPC server: "<<addr_string);

  publish_server_addr(addr_string);
  
  margo_addr_free(mid, addr);
  
}


/*
 * Set the connection between servers
 * each peer server's margo address.
 */
void connect_margo_servers(void)
{
}

/* Setup the memory resource on the server*/
/* the resource should be prepared and init by the user*/
void setup_server_buffer( void* _ptr , size_t rsize){

  server_buffer = _ptr;
  server_buffer_length = rsize;
  
}

void start_server(size_t _num_clients)
{
  /* init counters*/
  num_clients = _num_clients;
  num_completed_clients = 0;

  /* Two Options: (1) keep server active */
  /*              (2) shutdown when all clients complete*/
  while (1) {
    sleep(1);
  }
}

/*
 * Initialize a margo sever on the calling process
 */
void init_servers()
{

  /* setup Margo RPC only if not done */
  assert( mid == MARGO_INSTANCE_NULL );
  setup_margo_server();
  if (mid == MARGO_INSTANCE_NULL) {
    UMAP_ERROR("cannot initialize Margo server");
  }
  
  /* register a remote read RPC */
  /* umap_rpc_in_t, umap_rpc_out_t are only significant on clients */
  /* uhg_umap_cb is only significant on the server */
  hg_id_t rpc_read_id = MARGO_REGISTER(mid, "umap_read_rpc",
				       umap_read_rpc_in_t,
				       umap_read_rpc_out_t,
				       umap_server_read_rpc);
  
  //connect_margo_servers();

}


void fini_servers(void)
{
  
  UMAP_LOG(Info, "Server shutting down ...");

  /* shut down margo */
  if(mid!=MARGO_INSTANCE_NULL)
    margo_finalize(mid);

}
