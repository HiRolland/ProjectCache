/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _EXT4_EXTENTS
#define _EXT4_EXTENTS

#include "ext4.h"

#define AGGRESSIVE_TEST_

#define EXTENTS_STATS__

#define CHECK_BINSEARCH__

#define EXT_DEBUG__
#ifdef EXT_DEBUG
#define ext_debug(a...) printk(a)
#else
#define ext_debug
#endif

#define EXT_STATS_

struct ext4_extent {
 __le32 ee_block; //first block num
 __le16 ee_len; // count of blokcs
 __le16 ee_start_hi;
 __le32 ee_start_lo;
};

//ei_block: 索引包含的逻辑数据块 index covers logical blocks from 'block',即，这个
//          索引项指向的最小的块号是 ei_block 块（B+树中的非子节点中的关键字）
//
// ei_leaf_lo: //下一级物理数据块
//
struct ext4_extent_idx {
 __le32 ei_block;
 __le32 ei_leaf_lo;
 __le16 ei_leaf_hi;
 __u16 ei_unused;
};

struct ext4_extent_header {
 __le16 eh_magic;
 __le16 eh_entries;//valild entries num
 __le16 eh_max;
 __le16 eh_depth;
 __le32 eh_generation;
};

#define EXT4_EXT_MAGIC 0xf30a

struct ext4_ext_path {
 ext4_fsblk_t p_block;
 __u16 p_depth;
 struct ext4_extent *p_ext;
 struct ext4_extent_idx *p_idx;
 struct ext4_extent_header *p_hdr;
 struct buffer_head *p_bh;
};

#define EXT4_EXT_CACHE_NO 0
#define EXT4_EXT_CACHE_GAP 1
#define EXT4_EXT_CACHE_EXTENT 2

#define EXT_CONTINUE 0
#define EXT_BREAK 1
#define EXT_REPEAT 2

#define EXT_MAX_BLOCK 0xffffffff

#define EXT_INIT_MAX_LEN (1UL << 15)
#define EXT_UNINIT_MAX_LEN (EXT_INIT_MAX_LEN - 1)

#define EXT_FIRST_EXTENT(__hdr__)   ((struct ext4_extent *) (((char *) (__hdr__)) +   sizeof(struct ext4_extent_header)))
#define EXT_FIRST_INDEX(__hdr__)   ((struct ext4_extent_idx *) (((char *) (__hdr__)) +   sizeof(struct ext4_extent_header)))
#define EXT_HAS_FREE_INDEX(__path__)   (le16_to_cpu((__path__)->p_hdr->eh_entries)   < le16_to_cpu((__path__)->p_hdr->eh_max))
#define EXT_LAST_EXTENT(__hdr__)   (EXT_FIRST_EXTENT((__hdr__)) + le16_to_cpu((__hdr__)->eh_entries) - 1)
#define EXT_LAST_INDEX(__hdr__)   (EXT_FIRST_INDEX((__hdr__)) + le16_to_cpu((__hdr__)->eh_entries) - 1)
#define EXT_MAX_EXTENT(__hdr__)   (EXT_FIRST_EXTENT((__hdr__)) + le16_to_cpu((__hdr__)->eh_max) - 1)
#define EXT_MAX_INDEX(__hdr__)   (EXT_FIRST_INDEX((__hdr__)) + le16_to_cpu((__hdr__)->eh_max) - 1)


#define INODE_HAS_EXTENT(i) ((i)->i_flags & EXT4_EXTENTS_FL)
#define get_ext4_header(i)      ((struct ext4_extent_header *) (i)->i_block)
#endif


