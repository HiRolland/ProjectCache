#ifndef _SYSTEM_EXTRAS_EXT4_UTILS_XATTR_H
#define _SYSTEM_EXTRAS_EXT4_UTILS_XATTR_H
#include "mogu.h"

#define EXT4_XATTR_MAGIC 0xEA020000
#define EXT4_XATTR_INDEX_SECURITY 6

//目前用不到此结构体
//inode中的空间足以存放
//
struct ext4_xattr_header {
    __le32  h_magic;
    __le32  h_refcount;
    __le32  h_blocks;
    __le32  h_hash;
    __le32  h_checksum;
    __u32   h_reserved[3];
};

struct ext4_xattr_ibody_header {
    __le32  h_magic;
};

struct ext4_xattr_entry {
    __u8 e_name_len;
    __u8 e_name_index;
    __le16 e_value_offs;
    __le32 e_value_block;
    __le32 e_value_size;
    __le32 e_hash;
    char e_name[0];
};

#define EXT4_XATTR_PAD_BITS 2
#define EXT4_XATTR_PAD (1<<EXT4_XATTR_PAD_BITS)
#define EXT4_XATTR_ROUND (EXT4_XATTR_PAD-1)
#define EXT4_XATTR_LEN(name_len) \
    (((name_len) + EXT4_XATTR_ROUND + \
    sizeof(struct ext4_xattr_entry)) & ~EXT4_XATTR_ROUND)
#define EXT4_XATTR_NEXT(entry) \
    ((struct ext4_xattr_entry *)( \
     (char *)(entry) + EXT4_XATTR_LEN((entry)->e_name_len)))
#define EXT4_XATTR_SIZE(size) \
    (((size) + EXT4_XATTR_ROUND) & ~EXT4_XATTR_ROUND)
#define IS_LAST_ENTRY(entry) (*(unsigned int *)(entry) == 0)

#define XATTR_SELINUX_SUFFIX "selinux"
#define XATTR_CAPS_SUFFIX "capability"

int xattr_addto_inode(struct ext4_inode *inode, int name_index,
        const char *name, const void *value, size_t value_len, u32 inode_size);
void erase_inode_xattr(struct ext4_inode *inode, u32 inode_size);
#endif
