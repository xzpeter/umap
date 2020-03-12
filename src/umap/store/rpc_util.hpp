#ifndef _RPC_UTIL_H
#define _RPC_UTIL_H

#include <map>
#include <margo.h>
#include "mercury_macros.h"
#include "mercury_proc_string.h"

#define LOCAL_RPC_ADDR_FILE "serverfile"
#define RPC_RESPONSE_READ_DONE 1234
#define	RPC_RESPONSE_WRITE_DONE 4321
#define	RPC_RESPONSE_REQ_UNAVAIL 7777
#define RPC_RESPONSE_REQ_AVAIL 8888
#define RPC_RESPONSE_REQ_WRONG_SIZE 9999

typedef struct remote_memory_object
{
  void *ptr;
  size_t rsize;
  remote_memory_object(){}
  remote_memory_object(void* p, size_t s)
    :ptr(p), rsize(s) {}
} RemoteMemoryObject;


/* global for each server */
static  bool has_server_setup;
static bool has_client_setup ; 
static margo_instance_id mid;
static hg_id_t umap_request_rpc_id;
static hg_id_t umap_read_rpc_id;
static hg_id_t umap_write_rpc_id;
//static std::map<const char*, RemoteMemoryObject> remote_memory_pool;
//static std::map<int, hg_addr_t> server_map;

void print_memory_pool();


#ifdef __cplusplus
extern "C" {
#endif

/* UMap RPC input structure */
MERCURY_GEN_PROC(umap_read_rpc_in_t,
                 ((hg_string_t)(id))\
                 ((hg_size_t)(size))\
                 ((hg_size_t)(offset))\
                 ((hg_bulk_t)(bulk_handle)))

/* UMap RPC output structure */
MERCURY_GEN_PROC(umap_read_rpc_out_t,
		 ((int32_t)(ret)))

/* UMap RPC input structure */
MERCURY_GEN_PROC(umap_write_rpc_in_t,
                 ((hg_string_t)(id))\
                 ((hg_size_t)(size))\
                 ((hg_size_t)(offset))\
                 ((hg_bulk_t)(bulk_handle)))

/* UMap RPC output structure */
MERCURY_GEN_PROC(umap_write_rpc_out_t,
		 ((int32_t)(ret)))

/* UMap RPC input structure */
MERCURY_GEN_PROC(umap_request_rpc_in_t,
		 ((hg_string_t)(id))\
                 ((hg_size_t)(size)))

/* UMap RPC output structure */
MERCURY_GEN_PROC(umap_request_rpc_out_t,
		 ((int32_t)(ret)))

#ifdef __cplusplus
} // extern "C"
#endif
  
#endif
