// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst,
                                     class lock_release_user *_lu)
        : lock_client(xdst), lu(_lu) {
    srand(time(NULL) ^ last_port);
    rlock_port = ((rand() % 32000) | (0x1 << 10));
    const char *hname;
    // VERIFY(gethostname(hname, 100) == 0);
    hname = "127.0.0.1";
    std::ostringstream host;
    host << hname << ":" << rlock_port;
    id = host.str();

    last_port = rlock_port;
    rpcs *rlsrpc = new rpcs(rlock_port);
    rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
    rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);
    mux = PTHREAD_MUTEX_INITIALIZER;
}

lock_protocol::status
lock_client_cache::acquireFromServer(lock_protocol::lockid_t lid) {
    int r, ret;
    locks[lid]->state = ACQUIRING;
    while (locks[lid]->state == ACQUIRING) {
        if (locks[lid]->retry) {
            locks[lid]->retry = false;
        }
        pthread_mutex_unlock(&mux);
        ret = cl->call(lock_protocol::acquire, lid, id, r);
        pthread_mutex_lock(&mux);
        if (ret == lock_protocol::OK) {
            locks[lid]->state = LOCKED;
            locks[lid]->qthreads.pop();
            pthread_mutex_unlock(&mux);
            return ret;
        } else if (ret == lock_protocol::RETRY) {
            if (locks[lid]->retry) {
                continue;
            }
            if (thread_cond.find(pthread_self()) == thread_cond.end()) {
                thread_cond[pthread_self()] = new pthread_cond_t();
                *thread_cond[pthread_self()] = PTHREAD_COND_INITIALIZER;
            }
            pthread_cond_wait(thread_cond[pthread_self()], &mux);
            if (locks[lid]->retry) {
                continue;
            }
            if (locks[lid]->state == FREE) {
                locks[lid]->state = LOCKED;
                locks[lid]->qthreads.pop();
            } else if (locks[lid]->state == NONE) {
                locks[lid]->state = ACQUIRING;
            } else if (locks[lid]->state != ACQUIRING) {
                pthread_cond_wait(thread_cond[pthread_self()], &mux);
                while (locks[lid]->revoke) {
                    pthread_cond_wait(thread_cond[pthread_self()], &mux);
                }
                if (locks[lid]->retry) {
                    locks[lid]->retry = false;
                    locks[lid]->state = ACQUIRING;
                    continue;
                }
                if (locks[lid]->state == FREE) {
                    locks[lid]->state = LOCKED;
                    locks[lid]->qthreads.pop();
                } else if (locks[lid]->state == NONE) {
                    locks[lid]->state = ACQUIRING;
                    continue;
                }
                tprintf("NOT COOL!\n");
            }
        }
    }
    pthread_mutex_unlock(&mux);
    return lock_protocol::OK;
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid) {
    pthread_mutex_lock(&mux);
//    tprintf("acquire: %s, %lu, %llu\n", id.c_str(), pthread_self(), lid);
    int ret = lock_protocol::OK, r;
    if (locks.find(lid) == locks.end()) {
        locks[lid] = new lock_client_entry();
    }
    if (locks[lid]->qthreads.empty() && locks[lid]->state == FREE) {
        locks[lid]->state = LOCKED;
    } else {
        if (thread_cond.find(pthread_self()) == thread_cond.end()) {
            thread_cond[pthread_self()] = new pthread_cond_t();
            *thread_cond[pthread_self()] = PTHREAD_COND_INITIALIZER;
        }
        if (locks[lid]->qthreads.empty() && locks[lid]->state == NONE) {
            locks[lid]->qthreads.push(pthread_self());
            return acquireFromServer(lid);
        } else {
            locks[lid]->qthreads.push(pthread_self());
            pthread_cond_wait(thread_cond[pthread_self()], &mux);
            while (locks[lid]->state != FREE && locks[lid]->state != NONE) {
                pthread_cond_wait(thread_cond[pthread_self()], &mux);
            }
            if (locks[lid]->state == FREE) {
                locks[lid]->qthreads.pop();
                locks[lid]->state = LOCKED;
            } else if (locks[lid]->state == NONE) {
                return acquireFromServer(lid);
            } else {
                tprintf("NOT COOL !!!\n");
            }
        }
    }

//    switch (locks[lid]->state) {
//        case FREE:
//            if (locks[lid]->qthreads.empty()) {
//                locks[lid]->state = LOCKED;
//            } else {
//                locks[lid]->qthreads.push(pthread_self());
//                if (thread_cond.find(pthread_self()) == thread_cond.end()) {
//                    thread_cond[pthread_self()] = new pthread_cond_t();
//                    *thread_cond[pthread_self()] = PTHREAD_COND_INITIALIZER;
//                }
//                pthread_cond_wait(thread_cond[pthread_self()], &mux);
//
//            }
//            pthread_mutex_unlock(&mux);
//            return lock_protocol::OK;
//        case NONE:
//            locks[lid]->qthreads.push(pthread_self());
////            pthread_mutex_unlock(&mux);
//            ret = acquireFromServer(lid);
//            return ret;
//        case LOCKED:
//        case ACQUIRING:
//        case RELEASING:
//            locks[lid]->qthreads.push(pthread_self());
//            if (thread_cond.find(pthread_self()) == thread_cond.end()) {
//                thread_cond[pthread_self()] = new pthread_cond_t();
//                *thread_cond[pthread_self()] = PTHREAD_COND_INITIALIZER;
//            }
//            while (locks[lid]->state != FREE && locks[lid]->state != NONE) {
//                pthread_cond_wait(thread_cond[pthread_self()], &mux);
//                if (locks[lid]->retry) {
//                    locks[lid]->retry = false;
////                    pthread_mutex_unlock(&mux);
//                    ret = acquireFromServer(lid);
//                    return ret;
//                }
//                if (locks[lid]->state == FREE) {
//                    locks[lid]->state = LOCKED;
//                    locks[lid]->qthreads.pop();
//                    pthread_mutex_unlock(&mux);
//                    return lock_protocol::OK;
//                } else if (locks[lid]->state == NONE) {
////                    pthread_mutex_unlock(&mux);
//                    ret = acquireFromServer(lid);
//                    return ret;
//                }
//            }
//            tprintf("NOT COOL1!\n");
//    }
    pthread_mutex_unlock(&mux);
    return ret;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid) {
    tprintf("release: %s, %lu, %llu\n", id.c_str(), pthread_self(), lid);
    int r;
    pthread_mutex_lock(&mux);
    locks[lid]->state = RELEASING;
    if (locks[lid]->revoke) {
        pthread_mutex_unlock(&mux);
        cl->call(lock_protocol::release, lid, id, r);
        pthread_mutex_lock(&mux);
        locks[lid]->state = NONE;
        locks[lid]->revoke = false;
        if (!locks[lid]->qthreads.empty()) {
            pthread_t pid = locks[lid]->qthreads.front();
            pthread_cond_signal(thread_cond[pid]);
        }
    } else if (!locks[lid]->qthreads.empty()) {
        pthread_t pid = locks[lid]->qthreads.front();
        locks[lid]->state = FREE;
        pthread_cond_signal(thread_cond[pid]);
    } else {
        locks[lid]->state = FREE;
    }
    pthread_mutex_unlock(&mux);
    return lock_protocol::OK;
}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid,
                                  int &) {
    int ret = rlock_protocol::OK, r;
    tprintf("revoke: %s, %lu, %llu\n", id.c_str(), pthread_self(), lid);
    pthread_mutex_lock(&mux);
    locks[lid]->revoke = true;
    if (locks[lid]->state == FREE) {
        locks[lid]->state = RELEASING;
        pthread_mutex_unlock(&mux);
        cl->call(lock_protocol::release, lid, id, r);
        pthread_mutex_lock(&mux);
        locks[lid]->state = NONE;
        locks[lid]->revoke = false;
        if (!locks[lid]->qthreads.empty()) {
            pthread_cond_signal(thread_cond[locks[lid]->qthreads.front()]);
        }
    }
    pthread_mutex_unlock(&mux);
    return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid,
                                 int &) {
    int ret = rlock_protocol::OK;
//    tprintf("retry: %s, %lu, %llu\n", id.c_str(), pthread_self(), lid);
    pthread_mutex_lock(&mux);
    locks[lid]->retry = true;
//    if (locks[lid]->qthreads.empty()) {
//        tprintf("empty!\n");
//    }
    if (thread_cond.find(locks[lid]->qthreads.front()) != thread_cond.end()) {
        pthread_cond_signal(thread_cond[locks[lid]->qthreads.front()]);
    }
    pthread_mutex_unlock(&mux);
    return ret;
}