#ifndef _RPC_CLIENT_H
#define _RPC_CLIENT_H

#include <map>
#include <margo.h>


void init_client(void);
void fini_client(void);
int  read_from_server(int server_id, void *buf_ptr, size_t nbytes, off_t offset);

static margo_instance_id mid;
static std::map<int, hg_addr_t> server_map;
static hg_id_t rpc_read_id ;

#endif
