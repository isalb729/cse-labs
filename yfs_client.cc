// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  lc = new lock_client_cache(lock_dst);
  if (ec->put(1, "") != extent_protocol::OK)
      printf("error init root dir\n"); // XYB: init root dir
}


yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    extent_protocol::attr a;
    lc->acquire(inum);
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        lc->release(inum);
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        lc->release(inum);
        return true;
    }

    printf("isfile: %lld is not a file\n", inum);
    lc->release(inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool
yfs_client::isdir(inum inum)
{
    extent_protocol::attr a;
    lc->acquire(inum);
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        lc->release(inum);
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a directory\n", inum);
        lc->release(inum);
        return true;
    }

    printf("isdir: %lld is not a directory\n", inum);
    lc->release(inum);
    return false;
}

bool
yfs_client::issymlink(inum inum)
{
    extent_protocol::attr a;
    lc->acquire(inum);
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        lc->release(inum);
        return false;
    }

    if (a.type == extent_protocol::T_SYM) {
        printf("issymlink: %lld is a symbol link\n", inum);
        lc->release(inum);
        return true;
    }

    printf("issymlink: %lld is not a symbol link\n", inum);
    lc->release(inum);
    return false;
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;
    lc->acquire(inum);
    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    lc->release(inum);
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;
    lc->acquire(inum);
    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    lc->release(inum);
    return r;
}
int
yfs_client::getsym(inum inum, syminfo &sin)
{
    int r = OK;
    lc->acquire(inum);
    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    sin.atime = a.atime;
    sin.mtime = a.mtime;
    sin.ctime = a.ctime;

release:
    lc->release(inum);
    return r;
}

#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
    int r = OK;
    lc->acquire(ino);
    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    std::string buf;
    if (ec->get(ino, buf) != extent_protocol::OK) {
        printf("Fail to get inode.\n");
        lc->release(ino);
        return r;
    }
    buf.resize(size);
    if ((r = ec->put(ino, buf)) != extent_protocol::OK) {
        printf("Fail to setattr.\n");
        lc->release(ino);
        return r;
    }
    lc->release(ino);
    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;
    lc->acquire(parent);
    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    if ((r = lookup(parent, name, found, ino_out)) != extent_protocol::OK) {
        printf("Lookup fail.\n");
        lc->release(parent);
        return r;
    }
    if (found) {
        r = EXIST;
        printf("File %s already exists.\n", name);
        lc->release(parent);
        return r;
    }
    ec->create(extent_protocol::T_FILE, ino_out);
    extent_protocol::attr a;
    if ((r = ec->getattr(parent, a)) != extent_protocol::OK) {
        printf("Get attr failed.\n");
        lc->release(parent);
        return r;
    }
    a.atime = std::time(0);
    a.ctime = std::time(0);
    a.mtime = std::time(0);
    std::string buf;
    ec->get(parent, buf);
    std::string de_str;
    struct dirent de;
    de.inum = ino_out;
    de.name = name;
    // de_str.assign((char *)&de, sizeof(dirent));
    // buf.append(de_str);
    // if (ec->put(parent, buf) != extent_protocol::OK) {
    //     exit(0);
    // }
    struct dirent_t de_t;
    memcpy(de_t.filename, name, strlen(name));//
    de_t.inum = de.inum;
    de_t.len = (unsigned short)strlen(name);
    de_str.assign((char *)&de_t, sizeof(dirent_t));
    buf += de_str;
    if (ec->put(parent, buf) != extent_protocol::OK) {
        exit(0);
    }
    lc->release(parent);
    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;
    lc->acquire(parent);
    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    if ((r = lookup(parent, name, found, ino_out)) != extent_protocol::OK) {
        printf("Lookup fail.\n");
        lc->release(parent);
        return r;
    }
    if (found) {
        r = EXIST;
        printf("File %s already exists.\n", name);
        lc->release(parent);
        return r;
    }
    ec->create(extent_protocol::T_DIR, ino_out);
    extent_protocol::attr a;
    if ((r = ec->getattr(parent, a)) != extent_protocol::OK) {
 
        printf("Get attr failed.\n");
        lc->release(parent);
        return r;
    }
    a.atime = std::time(0);
    a.ctime = std::time(0);
    a.mtime = std::time(0);
    std::string buf;
    ec->get(parent, buf);
    std::string de_str;
    struct dirent de;
    de.inum = ino_out;
    de.name = name;
    // de_str.assign((char *)&de, sizeof(dirent));
    // buf.append(de_str);
    // if (ec->put(parent, buf) != extent_protocol::OK) {
    //     exit(0);
    // }
    struct dirent_t de_t;
    memcpy(de_t.filename, name, strlen(name));//
    de_t.inum = de.inum;
    de_t.len = (unsigned short)strlen(name);
    de_str.assign((char *)&de_t, sizeof(dirent_t));
    buf += de_str;
    if (ec->put(parent, buf) != extent_protocol::OK) {
        exit(0);
    }
    lc->release(parent);
    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;
    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    std::list<dirent> list(MAXFILE);
    readdir(parent, list);
    for (std::list<struct dirent>::iterator it = list.begin(); 
    it != list.end(); it++ ) {
        if (it->name == name) {
            found = true;
            ino_out = it->inum;
            return r;
        }   
    }
    found = false;
    return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;
    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    std::string buf;
    
    extent_protocol::attr a;
    if (ec->get(dir, buf) != extent_protocol::OK) {
        printf("Fail to read dir %lld.\n", dir);
        return IOERR;
    }
    const char *cbuf = buf.c_str();
    ec->getattr(dir, a);
    a.atime = std::time(0);
    uint32_t ne = buf.size()/ sizeof(struct dirent_t);
    for (uint32_t i = 0; i < ne; i++) {
        // dirent de;
        // memcpy(&de, cbuf + i * sizeof(struct dirent), sizeof(dirent));
        // ("cbuf: %s, name: %s, inum: %llu\n", cbuf, de.name.c_str(), de.inum);
        dirent_t de_t;
        memcpy(&de_t, cbuf + i * sizeof(struct dirent_t), sizeof(dirent_t));
        dirent de;
        de.name.assign(de_t.filename, de_t.len);
        de.inum = de_t.inum;
        list.push_back(de);
    }
    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;

    /*
     * your code goes here.
     * note: read using ec->get().
     */
    std::string buf;
    
    if ( (r = ec->get(ino, buf)) != extent_protocol::OK) {
        return r;
    }
    if (!buf.size() || (size_t)off > buf.size() - 1) {
        r = extent_protocol::IOERR;
        printf("Invalid offset.\n");
        return r;
    }
    if (off + size > buf.size()) {
        data = buf.substr(off);
    } else {
        data = buf.substr(off, size);
    }
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;
    lc->acquire(ino);
    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    std::string buf;
    if ( (r = ec->get(ino, buf)) != extent_protocol::OK) {
        lc->release(ino);
        return r;
    }
    if ((size_t)off + size < buf.size()) {
        // buf = buf.replace(off, size, data);
        std::string app = buf.substr(off + size);
        buf = buf.substr(0, off).append(data, size);
        buf.append(app);
        bytes_written = size;
    } else if ((size_t)off <= buf.size()) {
        buf = buf.substr(0, off);
        buf.append(data, size);
        bytes_written = size;
    } else {
        bytes_written = off - buf.size() + size;
        std::string zstr(off - buf.size(), '\0');
        buf.append(zstr);
        buf.append(data, size);
    }
    if ((r = ec->put(ino, buf)) != extent_protocol::OK) {
        printf("Write failed.\n");
        lc->release(ino);
        return r;
    }
    lc->release(ino);
    return r;
}

int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;
    lc->acquire(parent);
    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    bool found;
    inum ino;
    lookup(parent, name, found, ino);
    if (!found) {
        printf("File not found.\n");
        r = extent_protocol::NOENT;
        lc->release(parent);
        return r;
    }
    
    std::string buf;
    std::list<dirent> list;
    readdir(parent, list);
    for (std::list<dirent>::iterator it = list.begin(); it != list.end(); it++) { 
        if (it->inum != ino) {
        std::string de_str;
        struct dirent de;
        de.inum = it->inum;
        de.name = it->name;
        struct dirent_t de_t;
        memcpy(de_t.filename, de.name.c_str(), strlen(de.name.c_str()));
        de_t.inum = de.inum;
        de_t.len = (unsigned short)strlen(de.name.c_str());
        de_str.assign((char *)&de_t, sizeof(dirent_t));
        buf += de_str;
        }
    }
    ec->put(parent, buf);

    extent_protocol::attr a;
    ec->getattr(ino, a);
    if (a.type == extent_protocol::T_FILE) {
        if ((r = ec->remove(ino)) != extent_protocol::OK) {
            printf("Unlink failed.\n");
            lc->release(parent);
            return r;
        }
    } else if (a.type == extent_protocol::T_DIR) {
        if ((r = rmdir(ino)) != extent_protocol::OK) {
            printf("Rmdir failed.\n");
            lc->release(parent);
            return r;
        }
    } else {
        r = extent_protocol::NOENT;
        lc->release(parent);
        return r;
    }
    lc->release(parent);
    return r;
}

int yfs_client::rmdir(inum dir) {
    lc->acquire(dir);
    int r = OK;
    std::list<dirent> list;
    readdir(dir, list);
    for (std::list<dirent>::iterator it = list.begin(); it != list.end(); it++) {
        if (isdir(it->inum)) rmdir(it->inum);
        if (isfile(it->inum)) ec->remove(it->inum);
        if (issymlink(it->inum)) ec->remove(it->inum);
    }
    lc->release(dir);
    return r;
}

int yfs_client::readlink(inum ino, std::string &buf) {
    return ec->get(ino, buf);
}

int yfs_client::symlink(inum parent, const char *name, const char *link, inum &ino) {
    int r = OK;
    bool found;
    inum id;
    lc->acquire(parent);
    lookup(parent, name, found, id);
    if (found) {
        lc->release(parent);
        return EXIST;
    }
    if ((r = ec->create(extent_protocol::T_SYM, ino)) != extent_protocol::OK){
        lc->release(parent); 
        return r;
    }
    
    if ((r = ec->put(ino, link)) != extent_protocol::OK){
        lc->release(parent);
        return r;
    }
    std::string buf;
    r = ec->get(parent, buf);
    struct dirent_t de_t;
    de_t.inum = ino;
    de_t.len = strlen(name);
    memcpy(de_t.filename, name, de_t.len);
    std::string app; 
    app.assign((char *)&de_t, sizeof(struct dirent_t));
    r = ec->put(parent, buf + app);
    lc->release(parent);
    return r;
}
