#ifndef _RPC_SERVER_H
#define _RPC_SERVER_H

#include <margo.h>

void init_servers(size_t rsize);
void fini_servers(void);

static int num_completed_clients=0;
static void   *server_buffer=NULL;
static size_t server_buffer_length=0;
static bool has_margo_setup=false;

#endif
