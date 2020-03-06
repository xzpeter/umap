#ifndef _RPC_UTIL_H
#define _RPC_UTIL_H


#define LOCAL_RPC_ADDR_FILE "serverfile"

/* UMap RPC input structure */
MERCURY_GEN_PROC(umap_read_rpc_in_t,
                 ((hg_size_t)(size))\
                 ((hg_size_t)(offset))\
                 ((hg_bulk_t)(bulk_handle)))

/* UMap RPC output structure */
MERCURY_GEN_PROC(umap_read_rpc_out_t,
		 ((hg_size_t)(size))\
		 ((int32_t)(ret)))


#endif
