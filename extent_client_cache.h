#ifndef extent_client_cache_h
#define extent_client_cache_h

#include "extent_client.h"
#include "tprintf.h"
#include <map>
#include <ctime>

class extent_release_user {
public:
    virtual void dorelease(extent_protocol::extentid_t) = 0;

    virtual ~extent_release_user() {};
};

class extent_client_cache : public extent_client {
private:
    class extent_release_user *lu;

    bool firstReq;
    int rextent_port;
    std::string hostname;
    std::string cid;
    pthread_mutex_t mux;

    std::map<extent_protocol::extentid_t, extent_protocol::attr> attr_cache{};
    std::map<extent_protocol::extentid_t, std::string> content_cache{};
public:
    extent_client_cache(std::string dst, class extent_release_user *l = 0);

    extent_protocol::status create(uint32_t type, extent_protocol::extentid_t &eid);

    extent_protocol::status get(extent_protocol::extentid_t eid,
                                std::string &buf);

    extent_protocol::status getattr(extent_protocol::extentid_t eid,
                                    extent_protocol::attr &a);

    extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);

    extent_protocol::status remove(extent_protocol::extentid_t eid);

    virtual ~extent_client_cache() {}

    static int last_port;

    rextent_protocol::status revoke_handler(extent_protocol::extentid_t eid, int &);

    rextent_protocol::status
    reset_content_handler(extent_protocol::extentid_t eid, std::string buf, int &);

    rextent_protocol::status reset_attr_handler(extent_protocol::extentid_t eid, extent_protocol::attr attr, int &);
};


#endif