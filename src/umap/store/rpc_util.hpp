#ifndef _RPC_UTIL_H
#define _RPC_UTIL_H

#include "mercury_macros.h"

#define LOCAL_RPC_ADDR_FILE "serverfile"

static margo_instance_id mid;
static hg_id_t umap_read_rpc_id;
static hg_id_t umap_write_rpc_id;

#ifdef __cplusplus
extern "C" {
#endif

/* UMap RPC input structure */
MERCURY_GEN_PROC(umap_read_rpc_in_t,
                 ((hg_size_t)(size))\
                 ((hg_size_t)(offset))\
                 ((hg_bulk_t)(bulk_handle)))

/* UMap RPC output structure */
MERCURY_GEN_PROC(umap_read_rpc_out_t,
		 ((int32_t)(ret)))

/* UMap RPC input structure */
MERCURY_GEN_PROC(umap_write_rpc_in_t,
                 ((hg_size_t)(size))\
                 ((hg_size_t)(offset))\
                 ((hg_bulk_t)(bulk_handle)))

/* UMap RPC output structure */
MERCURY_GEN_PROC(umap_write_rpc_out_t,
		 ((int32_t)(ret)))

#ifdef __cplusplus
} // extern "C"
#endif
  
#endif
