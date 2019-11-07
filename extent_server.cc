// the extent server implementation

#include "extent_server.h"
#include "handle.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define prt(s, ...) do { \
  FILE *f = fopen("./try", "a+"); \
  fprintf(f, s, ##__VA_ARGS__); \
  fclose(f); \
  } while (0);

extent_server::extent_server() {
    im = new inode_manager();
    cset[1] = std::set<std::string>{};
}

int extent_server::create(uint32_t type, std::string cid, extent_protocol::extentid_t &id) {
    // alloc a new inode and return inum
    id = im->alloc_inode(type);
    cset[id] = std::set<std::string>{};
    cset[id].insert(cid);
    return extent_protocol::OK;
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, std::string cid, int &) {
    id &= 0x7fffffff;

    const char *cbuf = buf.c_str();
    int size = buf.size();
    im->write_file(id, cbuf, size);
    int r;
    extent_protocol::attr attr;
    im->getattr(id, attr);
    if (cset.find(id) != cset.end()) {
        cset[id].insert(cid);
        for (std::set<std::string>::iterator it = cset[id].begin(); it != cset[id].end(); it++) {
            handle(*it).safebind()->call(rextent_protocol::reset_attr, id, attr, r);
            handle(*it).safebind()->call(rextent_protocol::reset_content, id, buf, r);
        }
    } else {
        assert(0);
    }
    return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string cid, std::string &buf) {

    id &= 0x7fffffff;

    int size = 0;
    char *cbuf = NULL;

    im->read_file(id, &cbuf, &size);
    if (size == 0)
        buf = "";
    else {
        buf.assign(cbuf, size);
        free(cbuf);
    }
    extent_protocol::attr attr;
    im->getattr(id, attr);
    if (cset.find(id) != cset.end()) {
        cset[id].insert(cid);
        for (std::set<std::string>::iterator it = cset[id].begin(); it != cset[id].end(); it++) {
            int r;
            handle(*it).safebind()->call(rextent_protocol::reset_attr, id, attr, r);
        }
    } else {
        assert(0);
    }
    return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, std::string cid, extent_protocol::attr &a) {

    id &= 0x7fffffff;

    extent_protocol::attr attr;
    memset(&attr, 0, sizeof(attr));
    im->getattr(id, attr);
    a = attr;
    if (cset.find(id) != cset.end()) {
        cset[id].insert(cid);
    } else {
        assert(0);
    }
    return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, std::string cid, int &) {

    id &= 0x7fffffff;
    im->remove_file(id);
    int r;
    for (std::set<std::string>::iterator it = cset[id].begin(); it != cset[id].end(); it++) {
        handle(*it).safebind()->call(rextent_protocol::revoke, id, r);
    }
    cset[id].clear();
    cset.erase(id);
    return extent_protocol::OK;
}

