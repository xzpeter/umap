//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2020 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////
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

  if (NULL != addr) {
    UMAP_LOG(Info, "found local server rpc address "<< addr);
  }
  else
    UMAP_ERROR("Unable to find local server rpc address ");
    
  return addr;
}

static margo_instance_id setup_margo_client(){

  /* get the protocol used by the server */
  char* server_address_string = get_server_address_string();
  char* protocol = strdup(server_address_string);
  char* del = strchr(protocol, ';');
  if (del) *del = '\0';
  UMAP_LOG(Info, "server :"<<server_address_string<<" protocol: "<<protocol);
  
  /* Init Margo using server's protocol */
  margo_instance_id mid;
  int use_progress_thread = 1;//flag to use a dedicated thread for running Mercury's progress loop. 
  int rpc_thread_count = 1; //number of threads for running rpc calls
  mid = margo_init(protocol, MARGO_SERVER_MODE, use_progress_thread, rpc_thread_count);
  free(protocol);
  if (mid == MARGO_INSTANCE_NULL) {
    free(server_address_string);
    UMAP_ERROR("margo_init protocol failed");
    return mid;
  }
  UMAP_LOG(Info, "margo_init done");

  
  /* TODO: want to keep this enabled all the time */
  margo_diag_start(mid);

  
  /* lookup server address from string */
  hg_addr_t server_address = HG_ADDR_NULL;
  margo_addr_lookup(mid, server_address_string, &server_address);
  free(server_address_string);
  if (server_address == HG_ADDR_NULL) {
    margo_finalize(mid);
    UMAP_ERROR("Failed to lookup margo server address from string: "<<server_address_string);
  }
  
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
  hg_id_t rpc_read_id = MARGO_REGISTER(mid, "umap_read_rpc",
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
