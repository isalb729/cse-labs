// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
#include "map"
#include "queue"

class lock_release_user {
public:
    virtual void dorelease(lock_protocol::lockid_t) = 0;

    virtual ~lock_release_user() {};
};


class lock_client_cache : public lock_client {
private:
    class lock_release_user *lu;

    int rlock_port;
    std::string hostname;
    std::string id;
    pthread_mutex_t mux;
    lock_protocol::status acquireFromServer(lock_protocol::lockid_t);
public:
    enum lock_client_state {
        NONE, LOCKED, FREE, ACQUIRING, RELEASING
    };
    std::map<pthread_t, pthread_cond_t *> thread_cond;
    struct lock_client_entry {
        lock_client_state state = NONE;
        std::queue<pthread_t> qthreads;
        bool retry = false, revoke = false;
    };

    std::map<lock_protocol::lockid_t, lock_client_entry *> locks;


    static int last_port;

    lock_client_cache(std::string xdst, class lock_release_user *l = 0);

    virtual ~lock_client_cache() {};

    lock_protocol::status acquire(lock_protocol::lockid_t);

    lock_protocol::status release(lock_protocol::lockid_t);

    rlock_protocol::status revoke_handler(lock_protocol::lockid_t,
                                          int &);

    rlock_protocol::status retry_handler(lock_protocol::lockid_t,
                                         int &);
};


#endif
