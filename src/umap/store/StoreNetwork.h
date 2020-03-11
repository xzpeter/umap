//////////////////////////////////////////////////////////////////////////////
// Copyright 2017-2020 Lawrence Livermore National Security, LLC and other
// UMAP Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: LGPL-2.1-only
//////////////////////////////////////////////////////////////////////////////

#ifndef _UMAP_NETWORK_STORE_H_
#define _UMAP_NETWORK_STORE_H_
#include <cstdint>
#include <map>
#include "umap/store/Store.hpp"
#include "umap/umap.h"


namespace Umap {
  typedef struct remote_memory_object
  {
    void *ptr;
    size_t rsize;
    remote_memory_object(void* p, size_t s)
    :ptr(p), rsize(s) {}
  } RemoteMemoryObject;
  
  class StoreNetwork : public Store {
  public:
    StoreNetwork(std::size_t _rsize_, bool _is_server=false);
    ~StoreNetwork();
    
    ssize_t read_from_store(char* buf, size_t nb, off_t off);
    ssize_t write_to_store(char* buf, size_t nb, off_t off);

  private:
    size_t rsize;
    bool is_server;
    size_t num_clients;
    const char* ds_id;
  protected:    
    static std::map<const char*, RemoteMemoryObject> remote_memory_pool;
    static int server_id;
    static int client_id;
  };

  class StoreNetworkServer : public StoreNetwork {
  public:
    StoreNetworkServer(const char* id, void* _ptr, std::size_t _rsize_, std::size_t _num_clients=0);
    ~StoreNetworkServer();
  };
  
  class StoreNetworkClient : public StoreNetwork {
  public:
    StoreNetworkClient(const char* id, std::size_t _rsize_);
    ~StoreNetworkClient();
  };
}
#endif
