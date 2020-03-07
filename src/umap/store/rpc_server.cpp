//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2020 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////
#include <cassert>
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
  UMAP_LOG(Info, "Entering");

  assert(has_margo_setup);
  
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

  UMAP_LOG(Info, "request "<<input.size<<" bytes at offset "<< input.offset);
  
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
    assert(input.size <= server_buffer_length );
    void* server_buffer_ptr = (char*)server_buffer;// + input.offset;
    void **buf_ptrs = (void **) &(server_buffer_ptr);
    ret = margo_bulk_create(mid,
			 1, buf_ptrs,&(input.size),
			 HG_BULK_READ_ONLY,
			 &server_bulk_handle);
    if(ret != HG_SUCCESS){
      UMAP_ERROR("Failed to create bulk handle on server");
    }

    UMAP_LOG(Info,"start bulk transfer");

    /* initiate bulk transfer from server to client */
    /* margo_bulk_transfer is a blocking version of */
    /* that only returns when HG_Bulk_transfer complete */
    ret = margo_bulk_transfer(mid, HG_BULK_PUSH,
			      info->addr, input.bulk_handle, 0,  //client address, bulk handle, offset
			      server_bulk_handle, 0, input.size);//server bulk handle, offset, size of transfer
    if(ret != HG_SUCCESS){
      UMAP_ERROR("Failed to bulk transfer from server to client");
    }
    UMAP_LOG(Info,"end bulk transfer");


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
    UMAP_LOG(Info, "Exiting");
    return 0;
}
DEFINE_MARGO_RPC_HANDLER(umap_server_read_rpc)



static margo_instance_id setup_margo_server(){

  /* Init Margo using different transport protocols */
  margo_instance_id mid;
  int use_progress_thread = 1;//flag to use a dedicated thread for running Mercury's progress loop. 
  int rpc_thread_count = -1; //number of threads for running rpc calls
  mid = margo_init(PROTOCOL_MARGO_VERBS,
		   MARGO_SERVER_MODE,
		   use_progress_thread,
		   rpc_thread_count);
  if (mid == MARGO_INSTANCE_NULL) {
    UMAP_ERROR("margo_init protocol "<<PROTOCOL_MARGO_VERBS<<" failed");
    return mid;
  }
  UMAP_LOG(Info, "margo_init done");

  
  /* Find the address of this server */
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
  UMAP_LOG(Info, "Margo RPC server: "<<addr_string);

  publish_server_addr(addr_string);
  
  margo_addr_free(mid, addr);

  
  return mid;
}


/*
 * Set the connection between servers
 * each peer server's margo address.
 */
void connect_margo_servers(void)
{
  /*
    int rc;
    int ret = (int)UNIFYFS_SUCCESS;
    size_t i;
    hg_return_t hret;

    // block until a margo_svr key pair published by all servers
    rc = unifyfs_keyval_fence_remote();
    if ((int)UNIFYFS_SUCCESS != rc) {
        LOGERR("keyval fence on margo_svr key failed");
        ret = (int)UNIFYFS_FAILURE;
        return ret;
    }

    for (i = 0; i < glb_num_servers; i++) {
        int remote_pmi_rank = -1;
        char* pmi_rank_str = NULL;
        char* margo_addr_str = NULL;

        rc = unifyfs_keyval_lookup_remote(i, key_unifyfsd_pmi_rank,
                                          &pmi_rank_str);
        if ((int)UNIFYFS_SUCCESS != rc) {
            LOGERR("server index=%zu - pmi rank lookup failed", i);
            ret = (int)UNIFYFS_FAILURE;
            return ret;
        }
        if (NULL != pmi_rank_str) {
            remote_pmi_rank = atoi(pmi_rank_str);
            free(pmi_rank_str);
        }
        glb_servers[i].pmi_rank = remote_pmi_rank;

        margo_addr_str = rpc_lookup_remote_server_addr(i);
        if (NULL == margo_addr_str) {
            LOGERR("server index=%zu - margo server lookup failed", i);
            ret = (int)UNIFYFS_FAILURE;
            return ret;
        }
        glb_servers[i].margo_svr_addr = HG_ADDR_NULL;
        glb_servers[i].margo_svr_addr_str = margo_addr_str;
        LOGDBG("server index=%zu, pmi_rank=%d, margo_addr=%s",
               i, remote_pmi_rank, margo_addr_str);
        if (!margo_lazy_connect) {
            hret = margo_addr_lookup(unifyfsd_rpc_context->svr_mid,
                                     glb_servers[i].margo_svr_addr_str,
                                     &(glb_servers[i].margo_svr_addr));
            if (hret != HG_SUCCESS) {
                LOGERR("server index=%zu - margo_addr_lookup(%s) failed",
                       i, margo_addr_str);
                ret = (int)UNIFYFS_FAILURE;
            }
        }
    }

    return ret;*/
}


/*
 * Initialize a margo sever on the calling process
 */
void init_servers(size_t rsize)
{

  if( !has_margo_setup){

    margo_instance_id mid = setup_margo_server();
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

    has_margo_setup = true;
  }
  
  server_buffer_length = rsize;
  server_buffer = malloc(rsize);
  memset(server_buffer,1,rsize);
  if(!server_buffer){
    UMAP_ERROR(" Unable to allocate "<<rsize<<" bytes on the server");
  }
}


void fini_servers(void)
{
  //rpc_clean_local_server_addr();

  /* shut down margo */
  //margo_finalize(mid);
  
  /* free memory allocated for context structure */
  //free(ctx);
}
