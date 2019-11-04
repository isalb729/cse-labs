// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"


lock_server_cache::lock_server_cache() {
    mux = PTHREAD_MUTEX_INITIALIZER;
}


int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id,
                               int &r) {
    lock_protocol::status ret = lock_protocol::OK;
    pthread_mutex_lock(&mux);
    if (locks.find(lid) == locks.end()) {
        locks[lid] = new lock_server_entry();
    }
    switch (locks[lid]->state) {
        case NONE:
            locks[lid]->ownerId = id;
            locks[lid]->state = LOCKED;
//            tprintf("%s got lock %llu\n", id.c_str(), lid);
            pthread_mutex_unlock(&mux);
            return lock_protocol::OK;
        case LOCKED:
            locks[lid]->wclients.push(id);
            locks[lid]->state = REVOKING;
            pthread_mutex_unlock(&mux);
            handle(locks[lid]->ownerId).safebind()->call(rlock_protocol::revoke, lid, r);
            return lock_protocol::RETRY;
        case REVOKING:
            locks[lid]->wclients.push(id);
            pthread_mutex_unlock(&mux);
            return lock_protocol::RETRY;
        case RETRYING:
            if (locks[lid]->wclients.front() == id) {
                locks[lid]->ownerId = id;
                locks[lid]->state = LOCKED;
                locks[lid]->wclients.pop();
                pthread_mutex_unlock(&mux);
//                tprintf("%s got lock %llu\n", id.c_str(), lid);
                if (!locks[lid]->wclients.empty()) {
                    handle(locks[lid]->ownerId).safebind()->call(rlock_protocol::revoke, lid, r);
                }
                return lock_protocol::OK;
            } else {
                locks[lid]->wclients.push(id);
                pthread_mutex_unlock(&mux);
                return lock_protocol::RETRY;
            }
    }
    return ret;
}

int
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id,
                           int &r) {
    lock_protocol::status ret = lock_protocol::OK;
    pthread_mutex_lock(&mux);
    if (locks.find(lid) == locks.end()) {
        locks[lid] = new lock_server_entry();
        pthread_mutex_unlock(&mux);
        return ret;
    }
    locks[lid]->ownerId = "";
    if (!locks[lid]->wclients.empty()) {
        locks[lid]->state = RETRYING;
        pthread_mutex_unlock(&mux);
        handle(locks[lid]->wclients.front()).safebind()->call(rlock_protocol::retry, lid, r);
        pthread_mutex_lock(&mux);
    } else {
        locks[lid]->state = NONE;
    }
//    tprintf("%s release lock %llu\n", id.c_str(), lid);
    pthread_mutex_unlock(&mux);
    return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r) {
    tprintf("stat request\n");
    r = nacquire;
    return lock_protocol::OK;
}
