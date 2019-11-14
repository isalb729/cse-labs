#include "extent_client_cache.h"
#include "fstream"

#define prt(s, ...) do { \
  FILE *f = fopen("./try", "a+"); \
  fprintf(f, s, ##__VA_ARGS__); \
  fclose(f); \
  } while (0);

int extent_client_cache::last_port = 1;

extent_client_cache::extent_client_cache(std::string dst,
                                         class extent_release_user *_lu) : extent_client(dst), lu(_lu) {
    srand(time(NULL) ^ last_port);
    rextent_port = ((rand() % 32000) | (0x1 << 10));
    const char *hname;
    // VERIFY(gethostname(hname, 100) == 0);
    hname = "127.0.0.1";
    std::ostringstream host;
    host << hname << ":" << rextent_port;
    cid = host.str();
    rpcs *rlsrpc = new rpcs(rextent_port);
    rlsrpc->reg(rextent_protocol::revoke, this, &extent_client_cache::revoke_handler);
    rlsrpc->reg(rextent_protocol::reset_attr, this, &extent_client_cache::reset_attr_handler);
    rlsrpc->reg(rextent_protocol::reset_content, this, &extent_client_cache::reset_content_handler);
    mux = PTHREAD_MUTEX_INITIALIZER;
    firstReq = false;
};

extent_protocol::status
extent_client_cache::create(uint32_t type, extent_protocol::extentid_t &id) {
    extent_protocol::status ret = extent_protocol::OK;
    ret = cl->call(extent_protocol::create, type, cid, id);
    if (ret == extent_protocol::OK) {
        extent_protocol::attr attr;
        attr.size = 0;
        attr.type = type;
        attr.atime = std::time(0);
        attr.ctime = std::time(0);
        attr.mtime = std::time(0);
        attr_cache[id] = attr;
        content_cache[id] = "";
    }
    firstReq = true;
    return ret;
}

extent_protocol::status
extent_client_cache::get(extent_protocol::extentid_t eid, std::string &buf) {
    extent_protocol::status ret = extent_protocol::OK;
    firstReq = true;
    if (content_cache.find(eid) != content_cache.end()) {
        buf = content_cache[eid];
    } else {
        ret = cl->call(extent_protocol::get, eid, cid, buf);
        if (ret == extent_protocol::OK) {
            content_cache[eid] = buf;
        }
    }
    return ret;
}

extent_protocol::status
extent_client_cache::getattr(extent_protocol::extentid_t eid,
                             extent_protocol::attr &attr) {
    extent_protocol::status ret = extent_protocol::OK;
    firstReq = true;
    if (attr_cache.find(eid) != attr_cache.end()) {
        attr = attr_cache[eid];
        return ret;
    } else {
        ret = cl->call(extent_protocol::getattr, eid, cid, attr);
        if (ret == extent_protocol::OK) {
            attr_cache[eid] = attr;
        }
    }
    return ret;
}

extent_protocol::status
extent_client_cache::put(extent_protocol::extentid_t eid, std::string buf) {
    int r;
    extent_protocol::status ret = extent_protocol::OK;
    firstReq = true;
    if (content_cache.find(eid) != content_cache.end() && content_cache[eid] == buf) {
        return ret;
    }
    ret = cl->call(extent_protocol::put, eid, buf, cid, r);
    if (ret == extent_protocol::OK) {
        content_cache[eid] = buf;
    }
    return ret;
}

extent_protocol::status
extent_client_cache::remove(extent_protocol::extentid_t eid) {
    int r;
    extent_protocol::status ret = extent_protocol::OK;
    if (content_cache.find(eid) != content_cache.end() || !firstReq)
        ret = cl->call(extent_protocol::remove, eid, cid, r);
    return ret;
}

rextent_protocol::status
extent_client_cache::revoke_handler(extent_protocol::extentid_t eid, int &) {
    rextent_protocol::status ret = rextent_protocol::OK;
    content_cache.erase(eid);
    attr_cache.erase(eid);
    return ret;
}

rextent_protocol::status
extent_client_cache::reset_attr_handler(extent_protocol::extentid_t eid, extent_protocol::attr attr, int &) {
    rextent_protocol::status ret = rextent_protocol::OK;
    attr_cache[eid] = attr;
    return ret;
}

rextent_protocol::status
extent_client_cache::reset_content_handler(extent_protocol::extentid_t eid, std::string buf, int &) {
    rextent_protocol::status ret = rextent_protocol::OK;
    content_cache[eid] = buf;
    return ret;
}