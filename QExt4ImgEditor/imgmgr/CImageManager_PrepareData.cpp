#include "CImageManager.h"
//#include <QFileInfo>
//#include <QFile>
//#include <QDir>

// from android source
static struct ext4_dir_entry_2 *add_dentry(u8 *data,
                                           u32 *offset,
                                           struct ext4_dir_entry_2 *prev,
                                           u32 inode,
                                           const char *name,
                                           u8 file_type,
                                           u32 block_size)
{
    u8 name_len = strlen(name);
    u16 rec_len = 8 + ALIGN(name_len, 4);
    struct ext4_dir_entry_2 *dentry;

    u32 start_block = *offset / block_size;
    u32 end_block = (*offset + rec_len - 1) / block_size;
    if (start_block != end_block) {
        /* Adding this dentry will cross a block boundary, so pad the previous
           dentry to the block boundary */
        prev->rec_len += end_block * block_size - *offset;
        *offset = end_block * block_size;
    }

    dentry = (struct ext4_dir_entry_2 *)(data + *offset);
    dentry->inode = inode;
    dentry->rec_len = rec_len;
    dentry->name_len = name_len;
    dentry->file_type = file_type;
    memcpy(dentry->name, name, name_len);

    *offset += rec_len;
    return dentry;
}

//分配inode 和block

tBlockNum CImageManager::do_allocate_inode_extent_contigous_block(ext4_inode *inode, u64 size)
{

    u32 block_len = DIV_ROUND_UP(size, android_img.get_block_size());

    tBlockNum start_block_num = allocate_continus_blocks(block_len);
    if(start_block_num > 0)
    {
        struct ext4_extent_header *header;
        struct ext4_extent *extent;
#if 0
        header = get_ext4_header(inode);
        header->eh_depth = 0;
        header->eh_entries = 1;

        extent = EXT_FIRST_EXTENT(header);
        extent->ee_block = 0;
        extent->ee_len = blocks;
        extent->ee_start_hi = 0;
        extent->ee_start_lo = start_block_num;
#else
        header =
                (struct ext4_extent_header *)&inode->i_block[0];
        header->eh_magic = EXT4_EXT_MAGIC;
        //
        //只有连续的一块
        //
        header->eh_entries = 1;
        header->eh_max = 3;
        header->eh_generation = 0;
        header->eh_depth = 0;

        extent = EXT_FIRST_EXTENT(header);
        extent->ee_block = 0;
        extent->ee_len = block_len;
        extent->ee_start_hi = 0;
        extent->ee_start_lo = start_block_num;

#endif
        u64 blocks = (u64)block_len * android_img.get_block_size() / 512;

        inode->i_size_lo = size;
        inode->i_size_high = size >> 32;
        inode->i_blocks_lo = blocks & 0xFFFFFFFF;
        inode->osd2.linux2.l_i_blocks_high = (u16)((blocks >> 32) & 0xFFFF);

    }

    return start_block_num;
}

bool CImageManager::do_allocate_inode_extent_partial_block(ext4_inode *inode, u64 size, tBlockRegionList &ret_lst)
{
    ret_lst.clear();

    u32 block_len = DIV_ROUND_UP(size, android_img.get_block_size());


    //多申请一个，用于存放extendindex.如果不需要则释放掉多申请的这个块
    //
    bool state = allocate_partial_blocks(block_len + 1, ret_lst);

    if(!state || ret_lst.isEmpty()) return false;

    tBlockNum extent_blk = 0;
    if(ret_lst.size() <= 3)
    {
        // there are 3 extents at most, this means we can stored those in the inode.
        // release the last block, as mention, we have calloc one more.
        BlockRegion *reg = ret_lst.takeLast();

        delete_block(reg->start_block + reg->len - 1);
        reg->len  -= 1;
        if(reg->len == 0){
            delete reg;
        }else{
            ret_lst.append(reg);
        }
    }
    else
    {
        // take the first block used as extent node
        BlockRegion *reg = ret_lst.takeFirst();

        extent_blk = reg->start_block;
        reg->len -= 1;
        if(reg->len == 0){
            delete reg;
        }else{
            reg->start_block += 1;
            ret_lst.prepend(reg);
        }
    }

    // construct extent tree now.
    struct ext4_extent_header *hdr;
    struct ext4_extent_idx *idx;
    struct ext4_extent *extent = NULL;
    if (!extent_blk) {
        struct ext4_extent_header *hdr = (struct ext4_extent_header *)&inode->i_block[0];
        hdr->eh_magic = EXT4_EXT_MAGIC;
        hdr->eh_entries = ret_lst.size();
        hdr->eh_max = 3;
        hdr->eh_generation = 0;
        hdr->eh_depth = 0;

        extent = (struct ext4_extent *)&inode->i_block[3];
    }else{

        hdr = (struct ext4_extent_header *)&inode->i_block[0];
        hdr->eh_magic = EXT4_EXT_MAGIC;
        hdr->eh_entries = 1;
        hdr->eh_max = 3;
        hdr->eh_generation = 0;
        hdr->eh_depth = 1;

        idx = (struct ext4_extent_idx *)&inode->i_block[3];
        idx->ei_block = 0;
        idx->ei_leaf_lo = extent_blk;
        idx->ei_leaf_hi = 0;
        idx->ei_unused = 0;

        //intit extent node block
        u32 block_size = android_img.get_block_size();
        BlockMemoryBuffer *buf = allocate_block_buffer(1, block_size);
        if(!buf)
            return false;
        CBlockData * bd = new CBlockData((u8*)buf->data, extent_blk);
        register_blockdata(bd);

        u8* data = (u8*)bd->data_buffer();

        if(data)
        {
            hdr = (struct ext4_extent_header *)data;
            hdr->eh_magic = EXT4_EXT_MAGIC;
            hdr->eh_entries = ret_lst.size();
            hdr->eh_max = (android_img.get_block_size() - sizeof(struct ext4_extent_header)) /
                    sizeof(struct ext4_extent);
            hdr->eh_generation = 0;
            hdr->eh_depth = 0;

            extent = (struct ext4_extent *)(data + sizeof(struct ext4_extent_header));
        }


    }

    //update extent content
    u32 file_block = 0;
    for(int index = 0; index < ret_lst.size(); ++index)
    {
        extent += index;

        BlockRegion *reg = ret_lst.at(index);

        extent->ee_block    = file_block;
        extent->ee_len      = reg->len;
        extent->ee_start_hi = 0;
        extent->ee_start_lo = reg->start_block;
        file_block          += reg->len;
    }

    if (extent_blk)
        block_len += 1;

    //
    u64 blocks = (u64)block_len * android_img.get_block_size() / 512;
    /*
     * i_blocks_lo EXT4_HUGE_FILE_FL is NOT set in inode.i_flags,
     * then the file consumes i_blocks_lo + (i_blocks_hi
     *  << 32) 512-byte blocks on disk. If huge_file
     * is set and EXT4_HUGE_FILE_FL IS set in
     * inode.i_flags, then this file consumes
     * (i_blocks_lo + i_blocks_hi << 32) filesystem
     * blocks on disk.
     *
     * currently, i_blocks_lo is 512 bytes  one block
     */
    //update inode
    //inode->i_flags |= EXT4_EXTENTS_FL;
    inode->i_size_lo = size;
    inode->i_size_high = size >> 32;
    inode->i_blocks_lo = blocks;
    inode->osd2.linux2.l_i_blocks_high = blocks >> 32;//(u16)((blocks >> 32) & 0xFFFF);

    return true;
}

void CImageManager::fill_dir_data_block(u8 *data, u32 block_size, u32 blocks, CFileInfo *dir)
{
    struct ext4_dir_entry_2 *dentry = NULL;

    u32 offset = 0;
    dentry = add_dentry(data,
                        &offset,
                        NULL,
                        dir->inode_num(),
                        ".",
                        EXT4_FT_DIR,
                        block_size);
    dentry = add_dentry(data,
                        &offset,
                        dentry,
                        dir->is_root() ? dir->inode_num(): dir->parent()->inode_num(),
                        "..",
                        EXT4_FT_DIR,
                        block_size);

    //const tFileInfoList& lst = dir->get_directly_child_list();
    foreach(CFileInfo *cur, dir->get_directly_child_list())
    {
        std::string name = cur->filename().toStdString();
        dentry = add_dentry(data,
                            &offset,
                            dentry,
                            cur->inode_num(),
                            name.c_str(),
                            cur->file_type(),
                            block_size);
    }

    /* pad the last dentry out to the end of the block */
    dentry->rec_len += blocks * block_size - offset;
}

bool CImageManager::allocate_dir_blocks_and_fill_data(ext4_inode *inode, CFileInfo *dir, u64 size, u32 blocks, u32 block_size)
{
    tBlockNum start_block_num = do_allocate_inode_extent_contigous_block(inode,
                                                                         size
                                                                         );
    if(start_block_num > 0)
    {
        BlockMemoryBuffer *buf = allocate_block_buffer(blocks, block_size);
        if(!buf)
            return false;
        CBlockData* bd = new CBlockData((u8*)buf->data,
                                        start_block_num,
                                        blocks);
        register_blockdata(bd);
        fill_dir_data_block((u8*)buf->data, block_size, blocks, dir);

        return true;
    }
    else
    {
        tBlockRegionList lst;

        if(do_allocate_inode_extent_partial_block(inode, size, lst))
        {
            // prepare data block bufferS
            BlockMemoryBuffer *buf = allocate_block_buffer(blocks, block_size);

            if(!buf)
                return false;

            // construct CBlockData with a specified block num
            u32 offset = 0;
            for(int index = 0; index < lst.size(); ++index)
            {
                BlockRegion *reg = lst.at(index);

                for(int i = 0; i < reg->len; ++i)
                {
                    CBlockData *bd = new CBlockData((u8*)(buf->data + offset * block_size),
                                                    reg->start_block + i);

                    register_blockdata(bd);
                    offset += 1;
                }
            }

            INFO("file uncontinues blocks");
            fill_dir_data_block((u8*)buf->data, block_size, blocks, dir);

            return true;
        }
    }

    return false;
}

bool CImageManager::allocate_file_blocks_and_fill_data(ext4_inode *inode, const QString &file_path, u64 file_size, u32 blocks, u32 block_size)
{
    tBlockNum start_block_num = do_allocate_inode_extent_contigous_block(inode,
                                                                         file_size);

    if(start_block_num > 0)
    {
        QFile inFile(file_path);
        if(inFile.open(QIODevice::ReadOnly))
        {
            CBlockData * bd = 0;
            char *data = 0;
            // create a CBlockData with block num then fill data.
            BlockMemoryBuffer *buf = allocate_block_buffer(blocks, block_size);
            if(!buf)
                return false;
            for(int i = 0; i < blocks; ++i)
            {
                bd = new CBlockData((u8*)(buf->data + i * block_size), start_block_num + i);
                register_blockdata(bd);

                data = (char*)bd->data_buffer();
                if(data)
                {
                    inFile.read(data, block_size);
                }
            }

            inFile.close();

            return true;
        }
    }
    else
    {
        tBlockRegionList lst;

        if(do_allocate_inode_extent_partial_block(inode, file_size, lst))
        {
            QFile inFile(file_path);
            if(inFile.open(QIODevice::ReadOnly))
            {
                CBlockData * bd = 0;
                char *data = 0;
                // create CBlockData base on each BlockRegion with block num in a BlockRegion
                // after crt, fill data directlyS
                for(int index = 0; index < lst.size(); ++index)
                {
                    BlockRegion *reg = lst.at(index);
                    BlockMemoryBuffer *buf = allocate_block_buffer(reg->len, block_size);
                    if(!buf)
                        return false;
                    int offset = 0;
                    for(int i = 0; i < reg->len; ++i)
                    {

                        bd = new CBlockData((u8*)buf->data + offset * android_img.get_block_size(), reg->start_block + i);
                        register_blockdata(bd);

                        data = (char*)bd->data_buffer();
                        if(data)
                            inFile.read(data, block_size);

                        ++offset;
                    }

                    delete reg;
                }

                inFile.close();

                return true;
            }
        }
    }

    return false;
}

