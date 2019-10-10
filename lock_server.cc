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
    locks.insert(std::pair<lock_protocol::lockid_t, pthread_mutex_t>(lid, PTHREAD_MUTEX_INITIALIZER));
  pthread_mutex_unlock(&mux);
    pthread_mutex_lock(&locks.at(lid));
  } else {
  pthread_mutex_unlock(&mux);
    pthread_mutex_lock(&locks.at(lid));
  }
  nacquire++;
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  pthread_mutex_lock(&mux);
  if (locks.find(lid) == locks.end()) {
    ret = lock_protocol::IOERR;
  } else {
    pthread_mutex_unlock(&locks.at(lid));
  }
  nacquire--;
  r = nacquire;
  pthread_mutex_unlock(&mux);
  return ret;
}
