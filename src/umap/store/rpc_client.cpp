//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2020 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////
#include "umap/util/Macros.hpp"
#include "rpc_util.hpp"
#include "rpc_client.hpp"

static char* get_server_address(){

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

  if (NULL != addr) {
    UMAP_LOG(Info, "found local server rpc address "<< addr);
  }
  else
    UMAP_ERROR("Unable to find local server rpc address ");
    
  return addr;
}

static margo_instance_id setup_margo_client(){

  char* svr_addr_string = get_server_address();

  //get the protocol used by the server
  char* protocol = strdup(svr_addr_string);
  char* del = strchr(protocol, ';');
  if (del) *del = '\0';
  UMAP_LOG(Info, "svr_addr:"<<svr_addr_string<<" proto: "<<protocol);
  
  /* Init Margo using server's protocol */
  margo_instance_id mid;
  int use_progress_thread = 1;//flag to use a dedicated thread for running Mercury's progress loop. 
  int rpc_thread_count = 1; //number of threads for running rpc calls
  mid = margo_init(protocol, MARGO_SERVER_MODE, use_progress_thread, rpc_thread_count);
  if (mid == MARGO_INSTANCE_NULL) {
    UMAP_ERROR("margo_init protocol "<<protocol<<" failed");
    return mid;
  }
  UMAP_LOG(Info, "margo_init done");

  /* TODO: want to keep this enabled all the time */
  /* what's this do? */
  margo_diag_start(mid);
  
  /* Find the address of this client */
  hg_addr_t addr;
  hg_return_t ret = margo_addr_self(mid, &addr);
  if (ret != HG_SUCCESS) {
    UMAP_ERROR("margo_addr_self failed");
    margo_finalize(mid);
    return MARGO_INSTANCE_NULL;
  }

  /* Convert the server address to string*/
  char addr_string[128];
  hg_size_t addr_string_len = sizeof(addr_string);
  ret = margo_addr_to_string(mid, addr_string, &addr_string_len, addr);
  if (ret != HG_SUCCESS) {
    UMAP_ERROR("margo_addr_to_string failed");
    margo_addr_free(mid, addr);
    margo_finalize(mid);
    return MARGO_INSTANCE_NULL;
  }
  UMAP_LOG(Info, "Margo client: " << addr_string);
  margo_addr_free(mid, addr);

  free(svr_addr_string);
  free(protocol);
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
