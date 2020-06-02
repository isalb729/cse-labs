// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#define prt(s, ...) do { \
  FILE *f = fopen("./try", "a+"); \
  fprintf(f, s, ##__VA_ARGS__); \
  fclose(f); \
  } while (0);


lock_server::lock_server():
  nacquire (0)
{
  mux = PTHREAD_MUTEX_INITIALIZER;
}

lock_server::~lock_server()
{
  pthread_mutex_destroy(&mux);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  pthread_mutex_lock(&mux);
  if (locks.find(lid) == locks.end()) {
    locks.insert(std::pair<lock_protocol::lockid_t, int>(lid, 1));
    conds.insert(std::pair<lock_protocol::lockid_t, pthread_cond_t>(lid, PTHREAD_COND_INITIALIZER));
  } else if (!locks[lid]) {
    locks[lid] = 1;
  } else {
    while (locks[lid]) {
      pthread_cond_wait(&conds[lid], &mux);
    }
    locks[lid] = 1;
  }  
  pthread_mutex_unlock(&mux);
  return r = ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  pthread_mutex_lock(&mux);
  locks[lid] = 0;
  pthread_cond_signal(&conds[lid]);
  pthread_mutex_unlock(&mux);
  return r = ret;
}
