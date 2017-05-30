#include "xattr.h"
#include <stdio.h>
#include <QDebug>

static size_t xattr_free_space(struct ext4_xattr_entry *entry, char *end)
{
    while(!IS_LAST_ENTRY(entry) && (((char *) entry) < end)) {
        end   -= EXT4_XATTR_SIZE(le32_to_cpu(entry->e_value_size));
        entry  = EXT4_XATTR_NEXT(entry);
    }

    if (((char *) entry) > end) {
        //error("unexpected read beyond end of xattr space");
        return 0;
    }

    return end - ((char *) entry);
}

/*
 * Returns a pointer to the free space immediately after the
 * last xattr element
 */
static struct ext4_xattr_entry* xattr_get_last(struct ext4_xattr_entry *entry)
{
    for (; !IS_LAST_ENTRY(entry); entry = EXT4_XATTR_NEXT(entry)) {
        // skip entry
    }
    return entry;
}

#define NAME_HASH_SHIFT 5
#define VALUE_HASH_SHIFT 16

static void ext4_xattr_hash_entry(struct ext4_xattr_header *header,
        struct ext4_xattr_entry *entry)
{
    unsigned int hash = 0;
    char *name = entry->e_name;
    int n;
    u32 *value;

    for (n = 0; n < entry->e_name_len; n++) {
        hash = (hash << NAME_HASH_SHIFT) ^
            (hash >> (8*sizeof(hash) - NAME_HASH_SHIFT)) ^
            *name++;
    }

    if (entry->e_value_block == 0 && entry->e_value_size != 0) {
           value = (u32 *)((char *)header +
            le16_to_cpu(entry->e_value_offs));
        for (n = (le32_to_cpu(entry->e_value_size) +
            EXT4_XATTR_ROUND) >> EXT4_XATTR_PAD_BITS; n; n--) {
            hash = (hash << VALUE_HASH_SHIFT) ^
                (hash >> (8*sizeof(hash) - VALUE_HASH_SHIFT)) ^
                le32_to_cpu(*value++);
        }
    }
    entry->e_hash = cpu_to_le32(hash);
}

static struct ext4_xattr_entry* xattr_addto_range(
        void *block_start,
        void *block_end,
        struct ext4_xattr_entry *first,
        int name_index,
        const char *name,
        const void *value,
        size_t value_len)
{
    size_t name_len, available_size, needed_size, e_value_offs;
    char *val;
    struct ext4_xattr_entry *new_entry;
     name_len = strlen(name);
    if (name_len > 255)
        return NULL;

     available_size = xattr_free_space(first, (char*)block_end);
     needed_size = EXT4_XATTR_LEN(name_len) + EXT4_XATTR_SIZE(value_len);

    if (needed_size > available_size)
        return NULL;

    new_entry = xattr_get_last(first);
    memset(new_entry, 0, EXT4_XATTR_LEN(name_len));

    new_entry->e_name_len = name_len;
    new_entry->e_name_index = name_index;
    memcpy(new_entry->e_name, name, name_len);
    new_entry->e_value_block = 0;
    new_entry->e_value_size = cpu_to_le32(value_len);

    val = (char *) new_entry + available_size - EXT4_XATTR_SIZE(value_len);
    e_value_offs = val - (char *) block_start;

    new_entry->e_value_offs = cpu_to_le16(e_value_offs);
    memset(val, 0, EXT4_XATTR_SIZE(value_len));
    memcpy(val, value, value_len);

    //xattr_assert_sane(first);
    return new_entry;
}


int xattr_addto_inode(struct ext4_inode *inode, int name_index,
        const char *name, const void *value, size_t value_len,u32 inode_size)
{
    struct ext4_xattr_entry *result;
    struct ext4_xattr_ibody_header *hdr;
    struct ext4_xattr_entry *first;
    char *block_end;
    hdr = (struct ext4_xattr_ibody_header *) (inode + 1);
    first = (struct ext4_xattr_entry *) (hdr + 1);
    block_end = ((char *) inode) + inode_size;

    result =
        xattr_addto_range(first, block_end, first, name_index, name, value, value_len);

    if (result == NULL)
        return -1;

    hdr->h_magic = cpu_to_le32(EXT4_XATTR_MAGIC);
    inode->i_extra_isize = cpu_to_le16(sizeof(struct ext4_inode) - EXT4_GOOD_OLD_INODE_SIZE);

    return 0;
}


void erase_inode_xattr(ext4_inode *inode, u32 inode_size)
{
    struct ext4_xattr_ibody_header *hdr;
    struct ext4_xattr_entry *first;
    char *block_start, *block_end;
    int len = 0;
    hdr = (struct ext4_xattr_ibody_header *) (inode + 1);
    first = (struct ext4_xattr_entry *) (hdr + 1);
    block_start = (char*) first;
    block_end = ((char *) inode) + inode_size;

    if(block_start > block_end)
        return;
    len = block_end - block_start;

    memset(block_start, 0, len);
}
