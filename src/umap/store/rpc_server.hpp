#ifndef _RPC_SERVER_H
#define _RPC_SERVER_H

#include <margo.h>

void init_servers(size_t _rsize, size_t _num_clients);
void fini_servers(void);
void setup_server_buffer( void* _ptr );

static int num_completed_clients=0;
static int num_clients=0;
static void  *server_buffer=NULL;
static size_t server_buffer_length=0;

#endif
