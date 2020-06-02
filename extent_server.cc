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
    allc = std::set<std::string>{};
}

int extent_server::create(uint32_t type, std::string cid, extent_protocol::extentid_t &id) {
    id = im->alloc_inode(type);
    allc.insert(cid);
    return extent_protocol::OK;
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, std::string cid, int &) {
    id &= 0x7fffffff;

    const char *cbuf = buf.c_str();
    int size = buf.size();
    im->write_file(id, cbuf, size);
    int r;
    extent_protocol::attr attr;
    allc.insert(cid);
    im->getattr(id, attr);
    for (std::set<std::string>::iterator it = allc.begin(); it != allc.end(); it++) {
        handle(*it).safebind()->call(rextent_protocol::reset_attr, id, attr, r);
        handle(*it).safebind()->call(rextent_protocol::reset_content, id, buf, r);
    }
    return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string cid, std::string &buf) {

    id &= 0x7fffffff;

    int size = 0;
    char *cbuf = NULL;

    im->read_file(id, &cbuf, &size);
    allc.insert(cid);
    if (size == 0)
        buf = "";
    else {
        buf.assign(cbuf, size);
        free(cbuf);
    }
    extent_protocol::attr attr;
    im->getattr(id, attr);
    for (std::set<std::string>::iterator it = allc.begin(); it != allc.end(); it++) {
        int r;
        handle(*it).safebind()->call(rextent_protocol::reset_attr, id, attr, r);
    }
    return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, std::string cid, extent_protocol::attr &a) {

    id &= 0x7fffffff;

    extent_protocol::attr attr;
    allc.insert(cid);
    memset(&attr, 0, sizeof(attr));
    im->getattr(id, attr);
    a = attr;
    return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, std::string cid, int &) {

    id &= 0x7fffffff;
    im->remove_file(id);
    int r;
    for (std::set<std::string>::iterator it = allc.begin(); it != allc.end(); it++) {
        handle(*it).safebind()->call(rextent_protocol::revoke, id, r);
    }
    return extent_protocol::OK;
}