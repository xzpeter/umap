#ifndef _RPC_UTIL_H
#define _RPC_UTIL_H

#include <map>
#include <string>
#include <margo.h>
#include "mercury_macros.h"
#include "mercury_proc_string.h"


#define MAX_ADDR_LENGTH 64
#define LOCAL_RPC_ADDR_FILE "serverfile"
#define RPC_RESPONSE_READ_DONE 1234
#define	RPC_RESPONSE_WRITE_DONE 4321
#define	RPC_RESPONSE_REQ_UNAVAIL 7777
#define RPC_RESPONSE_REQ_AVAIL 8888
#define RPC_RESPONSE_REQ_WRONG_SIZE 9999
#define RPC_RESPONSE_RELEASE 6666
#define	RPC_RESPONSE_GENERAL_ERROR 1111

typedef struct remote_memory_object
{
  void *ptr;
  size_t rsize;
  size_t num_clients;
  remote_memory_object(){}
  remote_memory_object(void* p, size_t s, size_t n=0)
    :ptr(p), rsize(s), num_clients(n) {}
} RemoteMemoryObject;


/* global for each server */
//static  bool has_server_setup;
//static bool has_client_setup ; 

typedef std::map<std::string, RemoteMemoryObject> ResourcePool;


#ifdef __cplusplus
extern "C" {
#endif

/* UMap RPC input structure */
MERCURY_GEN_PROC(umap_read_rpc_in_t,
                 ((hg_const_string_t)(id))\
                 ((hg_size_t)(size))\
                 ((hg_size_t)(offset))\
                 ((hg_bulk_t)(bulk_handle)))

/* UMap RPC output structure */
MERCURY_GEN_PROC(umap_read_rpc_out_t,
		 ((int32_t)(ret)))

/* UMap RPC input structure */
MERCURY_GEN_PROC(umap_write_rpc_in_t,
                 ((hg_const_string_t)(id))\
                 ((hg_size_t)(size))\
                 ((hg_size_t)(offset))\
                 ((hg_bulk_t)(bulk_handle)))

/* UMap RPC output structure */
MERCURY_GEN_PROC(umap_write_rpc_out_t,
		 ((int32_t)(ret)))

/* UMap RPC input structure */
MERCURY_GEN_PROC(umap_request_rpc_in_t,
		 ((hg_const_string_t)(id))\
                 ((hg_size_t)(size)))

/* UMap RPC output structure */
MERCURY_GEN_PROC(umap_request_rpc_out_t,
		 ((int32_t)(ret)))

/* UMap RPC input structure */
MERCURY_GEN_PROC(umap_release_rpc_in_t,
		 ((hg_const_string_t)(id)))

/* UMap RPC output structure */
MERCURY_GEN_PROC(umap_release_rpc_out_t,
		 ((int32_t)(ret)))

#ifdef __cplusplus
} // extern "C"
#endif
  
#endif
