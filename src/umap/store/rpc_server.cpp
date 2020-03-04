//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2020 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////
#include "umap/util/Macros.hpp"
#include "rpc_server.hpp"

static const char* PROTOCOL_MARGO_SHM   = "na+sm://";
static const char* PROTOCOL_MARGO_VERBS = "ofi+verbs://";
static const char* PROTOCOL_MARGO_TCP   = "bmi+tcp://";
static const char* PROTOCOL_MARGO_MPI   = "mpi+static";


static margo_instance_id setup_margo_server(){

  /* Init Margo using different transport protocols */
  margo_instance_id mid;
  int use_progress_thread = 1;//flag to use a dedicated thread for running Mercury's progress loop. 
  int rpc_thread_count = -1; //number of threads for running rpc calls
  mid = margo_init(PROTOCOL_MARGO_VERBS, MARGO_SERVER_MODE, use_progress_thread, rpc_thread_count);
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
  margo_addr_free(mid, addr);

  return mid;
}


/*
 * Set the connection between servers
 * each peer server's margo address.
 */
void connect_margo_servers(void)
{/*
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
void init_servers(void)
{

  margo_instance_id mid = setup_margo_server();
  if (mid == MARGO_INSTANCE_NULL) {
    UMAP_ERROR("cannot initialize Margo server");
  }
  
  /* initialize margo */
  //register_server_rpcs(mid);
  
  //connect_margo_servers();
}


void fini_servers(void)
{
  //rpc_clean_local_server_addr();

  /* shut down margo */
  //margo_finalize(mid);
  
  /* free memory allocated for context structure */
  //free(ctx);
}
