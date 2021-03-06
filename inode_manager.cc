#include "inode_manager.h"
#include <ctime>

#define prt(s, ...) do { \
  FILE *f = fopen("./try", "a+"); \
  fprintf(f, s, ##__VA_ARGS__); \
  fclose(f); \
  } while (0);

// disk layer -----------------------------------------

disk::disk() {
    bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf) {
    if (id < 0 || id >= BLOCK_NUM || buf == NULL)
        return;
    memcpy(buf, blocks[id], BLOCK_SIZE);
}

void
disk::write_block(blockid_t id, const char *buf) {
    memcpy(blocks[id], buf, BLOCK_SIZE);
}

// block layer -----------------------------------------


// Allocate a free disk block.
blockid_t
block_manager::alloc_block() {
    /*
     * your code goes here.
     * note: you should mark the corresponding bit in block bitmap when alloc.
     * you need to think about which block you can start to be allocated.
     */
    int i = IBLOCK(INODE_NUM, sb.nblocks) + 1;
    for (; i < BLOCK_NUM; i++)
        if (!using_blocks[i]) {
            using_blocks[i] = 1;

            blockid_t bitblock = BBLOCK(i);
            char *buf = new char[BLOCK_SIZE];
            read_block(bitblock, buf);
            int n = (i - 1) % BPB;
            char bt = *(buf + n / 8);
            bt |= 1 < (n % 8);
            *(buf + n / 8) = bt;
            write_block(bitblock, buf);

            return i;
        }
    printf("No free blocks.\n");
    return 0;
}

void
block_manager::free_block(uint32_t id) {
    /*
     * your code goes here.
     * note: you should unmark the corresponding bit in the block bitmap when free.
     */
    using_blocks[id] = 0;

    blockid_t bitblock = BBLOCK(id);
    char *buf = new char[BLOCK_SIZE];
    read_block(bitblock, buf);
    int n = (id - 1) % BPB;
    char bt = *(buf + n / 8);
    bt &= ~(1 < (n % 8));
    *(buf + n / 8) = bt;
    write_block(bitblock, buf);

    return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager() {
    d = new disk();

    // format the disk
    sb.size = BLOCK_SIZE * BLOCK_NUM;
    sb.nblocks = BLOCK_NUM;
    sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf) {
    if (buf) d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf) {
    if (buf) d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager() {

    bm = new block_manager();
    uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
    if (root_dir != 1) {
        printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
        exit(0);
    }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type) {
    /*
     * your code goes here.
     * note: the normal inode block should begin from the 2nd inode block.
     * the 1st is used for root_dir, see inode_manager::inode_manager().
  '   */
    //
    int i = 1;
    for (; i < INODE_NUM + 1; i++)
        if (get_inode(i) == NULL || get_inode(i)->type == 0) break;
    inode_t *inode = new inode_t();
    inode->atime = std::time(0);
    inode->ctime = std::time(0);
    inode->mtime = std::time(0);
    inode->type = type;
    inode->size = 0;
//   bm->write_block(IBLOCK(i, bm->sb.nblocks), (char *)inode);
    put_inode(i, inode);
    free(inode);
    return i;
}

void
inode_manager::free_inode(uint32_t inum) {
    /*
     * your code goes here.
     * note: you need to check if the inode is already a freed one;
     * if not, clear it, and remember to write back to disk.
     */
    inode_t *ino = get_inode(inum);
    if (!ino || ino->type == 0) return;
    ino->type = 0;
    ino->atime = 0;
    ino->ctime = 0;
    ino->size = 0;
    ino->mtime = 0;
    memset(ino->blocks, 0, sizeof(blockid_t) * (NDIRECT + 1));
    put_inode(inum, ino);
    free(ino);
    return;
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode *
inode_manager::get_inode(uint32_t inum) {
    struct inode *ino, *ino_disk;
    char buf[BLOCK_SIZE];

    if (inum < 0 || inum >= INODE_NUM) {
        printf("\tim: inum out of range\n");
        return NULL;
    }

    bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
    // printf("%s:%d\n", __FILE__, __LINE__);

    ino_disk = (struct inode *) buf + inum % IPB;
    if (ino_disk->type == 0) {
        return NULL;
    }

    ino = (struct inode *) malloc(sizeof(struct inode));
    *ino = *ino_disk;

    return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino) {
    char buf[BLOCK_SIZE];
    struct inode *ino_disk;

    if (ino == NULL)
        return;

    bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
    ino_disk = (struct inode *) buf + inum % IPB;
    *ino_disk = *ino;
    bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a, b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return allocated data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size) {
    /*
     * your code goes here.
     * note: read blocks related to inode number inum,
     * and copy them to buf_Out
     */
    inode_t *ino = get_inode(inum); //
    ino->atime = std::time(0); //
    *size = ino->size;
    int nb = *size ? (*size - 1) / BLOCK_SIZE + 1 : 0;

    char *buf = new char[BLOCK_SIZE * nb];
    char *block_data = new char[BLOCK_SIZE], *ind_block = new char[BLOCK_SIZE];
    std::string content;
    for (int i = 0; i < nb; i++) {

        if (i < NDIRECT)
            bm->read_block(ino->blocks[i], block_data);
        else {
            bm->read_block(ino->blocks[NDIRECT], ind_block);
            bm->read_block(((blockid_t *) ind_block)[i - NDIRECT], block_data);
        }
        memcpy(buf + i * BLOCK_SIZE, block_data, BLOCK_SIZE);
    }
    *buf_out = buf;
    put_inode(inum, ino); //
    return;
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size) {
    /*
     * your code goes here.
     * note: write buf to blocks of inode inum.
     * you need to consider the situation when the size of buf
     * is larger or smaller than the size of original inode
     */
    inode_t *ino = get_inode(inum);
    int o_blocks = ino->size ? (ino->size - 1) / BLOCK_SIZE + 1 : 0;
    int w_blocks = size ? (size - 1) / BLOCK_SIZE + 1 : 0;
    blockid_t id;
    if (o_blocks > w_blocks) {
        for (int i = w_blocks; i < o_blocks; i++)
            bm->free_block(ino->blocks[i]);
    } else if (o_blocks < w_blocks) {
        for (int i = 0; i < w_blocks - o_blocks; i++) {
            id = bm->alloc_block();
            if (o_blocks + i >= NDIRECT) {
                blockid_t inblock_id = ino->blocks[NDIRECT];
                char inblock[BLOCK_SIZE];
                bm->read_block(inblock_id, inblock);
                (((blockid_t *) inblock)[o_blocks + i - NDIRECT]) = id;
                bm->write_block(inblock_id, inblock);
            } else
                ino->blocks[o_blocks + i] = id;
        }
    }

    ino->size = size;

    char *buf_b = new char[BLOCK_SIZE];
    for (int i = 0; i < w_blocks; i++) {
        memset(buf_b, 0, BLOCK_SIZE);
        memcpy(buf_b, buf + i * BLOCK_SIZE, size < BLOCK_SIZE * (i + 1) ? (size - BLOCK_SIZE * i) : BLOCK_SIZE);
        if (i < NDIRECT)
            bm->write_block(ino->blocks[i], buf_b);
        else {
            char *buf = new char[BLOCK_SIZE];
            bm->read_block(ino->blocks[NDIRECT], buf);
            bm->write_block(((blockid_t *) buf)[i - NDIRECT], buf_b);
        }
    }

    ino->atime = std::time(0);
    ino->ctime = std::time(0);
    ino->mtime = std::time(0);
    put_inode(inum, ino);
    return;
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a) {
    /*
     * your code goes here.
     * note: get the attributes of inode inum.
     * you can refer to "struct attr" in extent_protocol.h
     */

    inode_t *ino = get_inode(inum);
    if (!ino) return;
    a.atime = ino->atime;
    a.ctime = ino->ctime;
    a.mtime = ino->mtime;
    a.type = ino->type;
    a.size = ino->size;
    return;
}

void
inode_manager::remove_file(uint32_t inum) {
    /*
     * your code goes here
     * note: you need to consider about both the data block and inode of the file
     */
    inode_t *ino = get_inode(inum);
    int nb = ino->size ? (ino->size - 1) / BLOCK_SIZE : 0;
    for (int i = 0; i < nb; i++) {
        bm->free_block(ino->blocks[i]);
    }
    free_inode(inum);
    return;
}
