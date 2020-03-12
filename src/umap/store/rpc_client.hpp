#ifndef _RPC_CLIENT_H
#define _RPC_CLIENT_H


void client_init(void);
void client_fini(void);

int  client_read_from_server(int server_id, void *buf_ptr, size_t nbytes, off_t offset);
int  client_write_to_server(int server_id, void *buf_ptr, size_t nbytes, off_t offset);

bool client_check_resource(const char*id);
int client_request_resource(const char* id, size_t rsize);
void client_add_resource(const char*id, void* ptr, size_t rsize);
int client_delete_resource(const char* id);

#endif
