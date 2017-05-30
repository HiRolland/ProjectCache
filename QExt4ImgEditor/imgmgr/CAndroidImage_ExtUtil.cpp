#include "CAndroidImage.h"
#include "xattr.h"

static  u64 idx_to_block(struct ext4_extent_idx  *idx)
{
    u64 block;

    block = (u64)idx->ei_leaf_lo;
    block |= ((u64) idx->ei_leaf_hi << 31) << 1;

    return block;
}

ext4_inode *CAndroidImage::read_inode(tInodeNum inode_num)
{
    struct ext4_inode *inode_buffer;
    //申请一个struct ext4_inode，最大256字节
    inode_buffer = (struct ext4_inode *)calloc(1,s_inode_size);
    if( NULL == inode_buffer)
    {
        return NULL;
    }

    memset(inode_buffer, 0, s_inode_size);
    read_inode(inode_num, inode_buffer);

    return inode_buffer;
}

bool CAndroidImage::read_inode(tInodeNum inode_num, ext4_inode *inode_buffer)
{
    u32 group,index,blknum;
    char *inode_block_buffer = NULL;

    u32 blocksize = s_block_size;
    u32 inodes_per_group = s_inodes_per_group;
    ext2_group_desc *desc = block_group_desc_data;
    u32 inodes_count = s_total_inodes;
    u32 inode_size = s_inode_size;


    inode_block_buffer = (char *)calloc(1,blocksize * sizeof(char));
    if( NULL == inode_block_buffer)
    {
        return false;
    }

    if(inode_num == 0 || inode_num > inodes_count )
        return false;

    group = (inode_num -1)/inodes_per_group;//第几个group中
    index = ((inode_num -1) % inodes_per_group)*inode_size;//在inode table中的偏移
    blknum = desc[group].bg_inode_table + (index / blocksize);//该inode在该镜像中的具体块

    if(read_disk(inode_block_buffer, blknum, 1, blocksize) < blocksize)
    {
        free(inode_block_buffer);
        return false;
    }

    //申请一个struct ext4_inode，最大256字节
    memcpy(inode_buffer, inode_block_buffer + (index % blocksize), inode_size);
    free(inode_block_buffer);

    return true;
}

qint64 CAndroidImage::extent_binarysearch(ext4_extent_header *header, u64 lbn, int isallocated)
{
    struct ext4_extent *extent;
    struct ext4_extent_idx *index;
    struct ext4_extent_header *child;
    tBlockNum block, physical_block = 0;

    int blocksize = s_block_size;

    if(header->eh_magic != EXT4_EXT_MAGIC)
    {
        return -1;
    }

    extent = EXT_FIRST_EXTENT(header);//头（12个字节）后面是第一个entent
    if(header->eh_depth == 0)//没有树形结构的关系
    {
        for(int i = 0; i < header->eh_entries; i++)
        {
            if((lbn >= extent->ee_block) && (lbn < (extent->ee_block + extent->ee_len)))
            {
                physical_block = ext_to_block(extent) + lbn;
                physical_block = physical_block - (u64)extent->ee_block;

                if(isallocated)
                    free(header);
                return physical_block;
            }

            extent++; // Pointer increment by size of Extent.
        }

        return -1;
    }

    index = EXT_FIRST_INDEX(header);
    for(int i = 0; i < header->eh_entries; i++)
    {
        if(lbn >= index->ei_block)//ei_block:This index node covers file blocks from 'block' onward
        {
            child = (struct ext4_extent_header *) calloc(1,blocksize);
            block = idx_to_block(index);

            read_disk((char*)child, block, 1, blocksize);

            return extent_binarysearch(child, lbn, 1);
        }
    }
    // We reach here if we do not find the key
    if(isallocated)
        free(header);
    return physical_block;
}

tBlockNum CAndroidImage::extent_to_logical(ext4_inode *ino, u64 lbn)
{
    tBlockNum block;
    struct ext4_extent_header *header;

    header = get_ext4_header(ino);//ext4_header在这个inode的i_block数组中
    block = extent_binarysearch(header, lbn, 0);

    return block;
}

tBlockNum CAndroidImage::fileblock_to_logical(ext4_inode *ino, u32 lbn)
{
    tBlockNum block, indlast, dindlast, tindlast;
    u32 tmpblk, sz;
    u32 *indbuffer, *dindbuffer, *tindbuffer;

    int blocksize = s_block_size;

    //inode的数据具体的块号存放在i_blocks里 一共15个
    // 0 ~11是一级索引
    //12,13是二级索引
    //14是三级索引
    if(lbn < EXT4_NDIR_BLOCKS) //12以内是一级索引,直接返回块号即可
    {
        block = ino->i_block[lbn];
        if (block <= 0)
            return -1;
        else
            return ino->i_block[lbn];
    }

    sz = blocksize / sizeof(u32);//这个是计算i_block[12]能够存放的block数是否够文件存放
    indlast = sz + EXT4_NDIR_BLOCKS;
    indbuffer = (u32*) calloc(1,sz * sizeof(u32));
    if(NULL == indbuffer)
        return -1;

    if((lbn >= EXT4_NDIR_BLOCKS) && (lbn < indlast))//小于一级索引所能表示的范围
    {
        block = ino->i_block[EXT4_IND_BLOCK];
        if (block <= 0) {
            free(indbuffer);
            return -1;
        }

        read_disk((char*)indbuffer, block, 1, blocksize);

        lbn -= EXT4_NDIR_BLOCKS;
        block = indbuffer[lbn];
        free(indbuffer);

        return block;
    }

    //否则就是在二级索引块对应的数据中
    dindlast = (sz * sz) + indlast;
    dindbuffer =(u32*) calloc(1,sz * sizeof(u32));
    if(NULL == dindbuffer)
        return -1;

    //大于一级索引能够表示的范围，小于二级索引能表示的范围
    if((lbn >= indlast) && (lbn < dindlast))
    {
        block = ino->i_block[EXT4_DIND_BLOCK];
        if (block <= 0) {
            free(dindbuffer);
            free(indbuffer);
            return -1;
        }
        read_disk((char*)dindbuffer, block, 1, blocksize);

        tmpblk = lbn - indlast;//减去一级索引能够表示的范围
        block = dindbuffer[tmpblk/sz];//得到二级索引对应的条目
        read_disk((char*)indbuffer, block, 1, blocksize);//取得这个块

        lbn = tmpblk % sz;
        block = indbuffer[lbn];

        free(dindbuffer);
        free(indbuffer);
        return block;
    }

    //否则就是三级索引所能表示的范围了
    tindlast = (sz * sz * sz) + dindlast;
    tindbuffer = (u32*)calloc(1,sz * sizeof(u32));
    if(NULL == tindbuffer)
        return -1;
    if((lbn >= dindlast) && (lbn < tindlast))
    {
        if (lbn >= 0x2010c)
            lbn = lbn;
        block = ino->i_block[EXT4_TIND_BLOCK];
        if (block <= 0) {

            free(tindbuffer);
            free(dindbuffer);
            free(indbuffer);

            return -1;
        }
        read_disk((char*)tindbuffer, block, 1, blocksize);

        tmpblk = lbn - dindlast;
        block = tindbuffer[tmpblk/(sz * sz)];
        read_disk((char*)dindbuffer,block, 1, blocksize);

        tmpblk = tmpblk % (sz * sz);
        block = tmpblk / sz;
        lbn = tmpblk % sz;
        block = dindbuffer[block];
        read_disk((char*)indbuffer, block, 1, blocksize);
        block = indbuffer[lbn];

        free(tindbuffer);
        free(dindbuffer);
        free(indbuffer);

        return block;
    }
    //// We should not reach here,file could not larger than 4GB
    return -1;
}

int CAndroidImage::read_data_block(ext4_inode *inode, tBlockNum block_num, char *buf)
{
    tBlockNum block;
    int blocksize = s_block_size;

    if(INODE_HAS_EXTENT(inode))
    {
        block = extent_to_logical(inode, block_num);
    }
    else
    {
        block = fileblock_to_logical(inode, block_num);
    }
    if(block == -1)
        return -1;
    return read_disk(buf, block, 1, blocksize);
}

void CAndroidImage::read_selinux_from_inode_directly(ext4_inode *inode, char *buffer)
{
    struct ext4_xattr_ibody_header *hdr;
    struct ext4_xattr_entry *entry;

    char *val;
    //    struct vfs_cap_data cap_data;
    //    memset(&cap_data, 0, sizeof(cap_data));

    //first type: the extended attributes is located between two inodes
    hdr = (struct ext4_xattr_ibody_header *) (inode + 1);
    entry = (struct ext4_xattr_entry *) (hdr + 1);

    val = (char *) entry + entry->e_value_offs;

    if(!strcmp(entry->e_name, XATTR_SELINUX_SUFFIX)){
        memcpy(buffer, val, entry->e_value_size);
    }
    //    else if(!strcmp(entry->e_name, XATTR_CAPS_SUFFIX)){
    //        memcpy(&cap_data, val, entry->e_value_size);
    //        xattr_capabilities_value = ((u64)cap_data.data[1].permitted << 32) + cap_data.data[0].permitted;
    //    }
}

void CAndroidImage::read_selinux_from_inode_block(ext4_inode *inode, char *buffer)
{
    struct ext4_xattr_header *header;
    struct ext4_xattr_entry *entry;

    char *val;
    //struct vfs_cap_data cap_data;
    int blocksize = s_block_size;

    header = (struct ext4_xattr_header *)calloc(1,blocksize);
    if( NULL == header)
    {
        return ;
    }

    read_disk((char*)header, inode->i_file_acl_lo, 1, blocksize);

    entry = (struct ext4_xattr_entry *) (header + 1);
    val = (char *) entry + entry->e_value_offs;

    if(!strcmp(entry->e_name, XATTR_SELINUX_SUFFIX)){
        memcpy(buffer, val, entry->e_value_size);
    }
}

void CAndroidImage::read_selinux(ext4_inode *inode, char* buf)
{

    if(inode->i_extra_isize){
        read_selinux_from_inode_directly(inode, buf);

        if(inode->i_file_acl_lo){
            read_selinux_from_inode_block(inode, buf);
        }
    }
}

ext4_inode *CAndroidImage::read_inode_from_group(tInodeNum num)
{
    foreach(CBlockGroup * cur, block_group_vec)
    {
        if(cur->contains_inode(num))
        {
            return cur->get_inode(num);
        }
    }

    return NULL;
}

tBlockNum CAndroidImage::get_physic_block_num(ext4_inode *inode, tBlockNum block_num)
{
    tBlockNum block;
    if(INODE_HAS_EXTENT(inode))
    {
        block = extent_to_logical(inode, block_num);
    }
    else
    {
        block = fileblock_to_logical(inode, block_num);
    }
    return block;
}

