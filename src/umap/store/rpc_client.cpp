//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2020 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include "umap/util/Macros.hpp"
#include "rpc_util.hpp"
#include "rpc_client.hpp"

/* Read the server address published in the file */
static char* get_server_address_string(){

  char* addr = NULL;

  /* read server address from local file */
  FILE* fp = fopen(LOCAL_RPC_ADDR_FILE, "r");
  if (fp != NULL) {
    char addr_string[256];
    memset(addr_string, 0, sizeof(addr_string));
    if (1 == fscanf(fp, "%255s", addr_string)) {
      addr = strdup(addr_string);
    }
    fclose(fp);
  }

  if ( !addr)
    UMAP_ERROR("Unable to find local server rpc address ");
    
  return addr;
}

static margo_instance_id setup_margo_client(){

  /* get the protocol used by the server */
  char* server_address_string = get_server_address_string();
  char* protocol = strdup(server_address_string);
  char* del = strchr(protocol, ';');
  if (del) *del = '\0';
  UMAP_LOG(Debug, "server :"<<server_address_string<<" protocol: "<<protocol);
  
  /* Init Margo using server's protocol */
  int use_progress_thread = 1;//flag to use a dedicated thread for running Mercury's progress loop. 
  int rpc_thread_count = 1; //number of threads for running rpc calls
  //mid = margo_init(protocol, MARGO_SERVER_MODE, use_progress_thread, rpc_thread_count);
  mid = margo_init(protocol, MARGO_CLIENT_MODE, use_progress_thread, rpc_thread_count);
  free(protocol);
  if (mid == MARGO_INSTANCE_NULL) {
    free(server_address_string);
    UMAP_ERROR("margo_init protocol failed");
    return mid;
  }

  
  /* TODO: want to keep this enabled all the time */
  margo_diag_start(mid);

  
  /* lookup server address from string */
  hg_addr_t server_address = HG_ADDR_NULL;
  margo_addr_lookup(mid, server_address_string, &server_address);
  //free(server_address_string);
  if (server_address == HG_ADDR_NULL) {
    margo_finalize(mid);
    UMAP_ERROR("Failed to lookup margo server address from string: "<<server_address_string);
  }
  server_map[0]=server_address;
  UMAP_LOG(Info, "margo_init done");
  
  /* Find the address of this client process */
  hg_addr_t client_address;
  hg_return_t ret = margo_addr_self(mid, &client_address);
  if (ret != HG_SUCCESS) {
    margo_addr_free(mid, server_address);
    margo_finalize(mid);
    UMAP_ERROR("failed to lookup margo_addr_self on client");
  }

  /* Convert the address to string*/
  char client_address_string[128];
  hg_size_t len = sizeof(client_address_string);
  ret = margo_addr_to_string(mid, client_address_string, &len, client_address);
  if (ret != HG_SUCCESS) {
    margo_addr_free(mid, server_address);
    margo_addr_free(mid, client_address);
    margo_finalize(mid);
    UMAP_ERROR("failed to convert client address to string");
  }
  UMAP_LOG(Info, "Margo client adress: " << client_address_string);


  /* register a remote read RPC */
  /* umap_rpc_in_t, umap_rpc_out_t are only significant on clients */
  /* uhg_umap_cb is only significant on the server */
  rpc_read_id = MARGO_REGISTER(mid, "umap_read_rpc",
				       umap_read_rpc_in_t,
				       umap_read_rpc_out_t,
				       NULL);

  return mid;
}


/*
 * Initialize a margo client on the calling process
 */
void init_client(void)
{

  margo_instance_id mid = setup_margo_client();
  if (mid == MARGO_INSTANCE_NULL) {
    UMAP_ERROR("cannot initialize Margo client");
  }
  
  /* initialize margo */
  //register_server_rpcs(mid);
  
  //connect_margo_servers();
}


void fini_client(void)
{
  //rpc_clean_local_server_addr();

  /* shut down margo */
  //margo_finalize(mid);
  
  /* free memory allocated for context structure */
  //free(ctx);
}


int read_from_server(int server_id, void *buf_ptr, size_t nbytes, off_t offset){

  auto it = server_map.find(server_id);
  assert( it!=server_map.end());  
  hg_addr_t server_address = it->second;
  hg_return_t ret;
  
  /* Forward the RPC. umap_client_fwdcompleted_cb will be called
   * when receiving the response from the server
   * After completion, user callback is placed into a
   * completion queue and can be triggered using HG_Trigger().
   */
  /* Create a RPC handle */
  hg_handle_t handle;
  ret = margo_create(mid, server_address, rpc_read_id, &handle);
  assert(ret == HG_SUCCESS);
    
  
  /* Create input structure
   * empty string attribute causes segfault 
   * because Mercury doesn't check string length before copy
   */
  umap_read_rpc_in_t in;
  in.size   = nbytes;
  in.offset = offset;
  in.bulk_handle = HG_BULK_NULL;
  void **buf_ptrs    = (void **) &(buf_ptr);
  size_t *buf_sizes  = &(in.size);

  UMAP_LOG(Debug, "create bulk "<< in.size << " bytes at 0x" <<buf_ptr);
  /* Create a bulk transfer handle in args */
  ret = margo_bulk_create(mid,
			  1, buf_ptrs, buf_sizes,
			  HG_BULK_READWRITE,
			  &(in.bulk_handle));
  assert(ret == HG_SUCCESS);
    
  
  /* Forward RPC requst to the server */
  ret = margo_forward(handle, &in);
  assert(ret == HG_SUCCESS);

    
  /* verify the response */
  umap_read_rpc_out_t out;
  ret = margo_get_output(handle, &out);
  assert(ret == HG_SUCCESS);
  assert( out.ret=1234);
  margo_free_output(handle, &out);
 
  /* Free handle and bulk handles*/
  ret = margo_bulk_free(in.bulk_handle);
  assert(ret == HG_SUCCESS);
  ret = margo_destroy(handle);
  assert(ret == HG_SUCCESS);

  uint64_t *arr = (uint64_t*) buf_ptr;
  UMAP_LOG(Debug, "after getting response "<< arr[0]);
  
  return ret;

}
