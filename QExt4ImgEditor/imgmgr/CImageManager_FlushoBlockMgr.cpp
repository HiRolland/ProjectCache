#include "CImageManager.h"
#include <QFileInfo>

void CImageManager::process_origin_dir_removed_entries(CFileInfo *dir)
{
    if(!dir->if_del())
    {
        return;
    }
    /**
     * @brief lst,获取删除的项,注意,这里获取的删除列表均是原img内的file,不包含新添加到本dir的file
     */
    const tFileInfoList &lst = dir->get_delete_list();
    tFileInfoList child_lst;
    foreach(CFileInfo * cur, lst)
    {
        //删除的是文件夹
        //
        if(cur->is_dir())
        {
            child_lst.clear();
            //获取该文件夹的所有的文件,包含各级子目录的内容
            //
            cur->get_all_childs(child_lst);

            foreach(CFileInfo * info, child_lst)
            {
                //child lst可能是new
                //
                if(info->is_new())
                {
                    delete_new_added_file(info);
                }
                else
                {
                    delete_origin_file(info);
                }
            }
        }

        delete_origin_file(cur);
    }
}

void CImageManager::update_origin_dir_into_img(CFileInfo *dir)
{
    if(!dir->is_dir() || dir->is_new())
        return;

    if(!dir->if_add() && !dir->if_del())
    {
        return;
    }

    //更新原始文件夹的数据块内容
    //更新dirlist
    //
    u32 block_size = android_img.get_block_size();

    u64 size = 0, len =0;
    u32 dirs = 0;
    get_dir_entry_size(len, dirs, dir);

    int old_blocks = dir->block_count();
    int new_blocks = DIV_ROUND_UP(len, block_size);

    size = new_blocks * block_size;


    ext4_inode *inode = dir->inode();
    inode->i_links_count = dirs + 2;
    add_changed_inode(dir->inode_num());

    if(new_blocks <= old_blocks)
    {
        // well use old blocks directly
        BlockMemoryBuffer * buf = allocate_block_buffer(new_blocks, block_size);
        if(!buf)
            return;
#if 1
        const tBlockNumArray &bna = dir->get_data_block_nums();
        int blk_cnt = 0;
        for(blk_cnt; blk_cnt < bna.size(); ++blk_cnt)
        {
            tBlockNum blknum = bna.at(blk_cnt);

            mark_block_used(blknum);

            CBlockData *bd = new CBlockData((u8*)(buf->data + blk_cnt * block_size),
                                            blknum);
            register_blockdata(bd);
        }

        if(new_blocks < old_blocks)
        {
            for(blk_cnt; blk_cnt < bna.size(); ++blk_cnt)
            {
                tBlockNum blknum = bna.at(blk_cnt);

                delete_block(blknum);
            }
        }
#else
        int blk_cnt = 0;
        foreach(tBlockNum blknum, dir->get_data_block_nums())
        {
            CBlockData *bd = new CBlockData((u8*)(buf->data + blk_cnt * block_size),
                                            blknum);
            register_blockdata(bd);

            ++blk_cnt;
        }
#endif
        INFO("use origin block");
        //写入dir的数据块内容,即 目录列表
        //
        fill_dir_data_block(buf->data, block_size, old_blocks, dir);
    }
    else{

        foreach(tBlockNum blknum, dir->get_data_block_nums())
        {
            delete_block(blknum);
        }

        allocate_dir_blocks_and_fill_data(inode, dir, size, new_blocks, block_size);
    }
}

void CImageManager::update_origin_file_into_img(CFileInfo *file)
{
    if(!file->is_file() || file->is_new())
        return;

    if(file->get_file_src_path().isEmpty())
    {
        return;
    }
    QString loc_fn = file->get_file_src_path();
    QFileInfo info(loc_fn);

    u64 size = info.size();
    u32 block_size= android_img.get_block_size();

    int old_blocks = file->block_count();
    int new_blocks = DIV_ROUND_UP(size, block_size);

    ext4_inode *inode = file->inode();
    add_changed_inode(file->inode_num());

    if(new_blocks <= old_blocks)
    {
        QFile inFile(loc_fn);
        if(inFile.open(QIODevice::ReadOnly))
        {
            // well use old blocks directly
            CBlockData * bd = 0;
            char *data = 0;
            // create a CBlockData with block num then fill data.
            BlockMemoryBuffer *buf = allocate_block_buffer(new_blocks, block_size);
            if(!buf)
                return ;

            const tBlockNumArray &bna = file->get_data_block_nums();
            int blk_cnt = 0;

            for(blk_cnt; blk_cnt < bna.size(); ++blk_cnt)
            {
                tBlockNum blknum = bna.at(blk_cnt);

                mark_block_used(blknum);

                bd = new CBlockData((u8*)(buf->data + blk_cnt * block_size), blknum);
                register_blockdata(bd);

                data = (char*)bd->data_buffer();
                if(data)
                {
                    if(inFile.read(data, block_size) < 0)
                        break;
                }

            }

            inFile.close();

            inode->i_size_lo = size;
            inode->i_size_high = size >> 32;

            if(new_blocks < old_blocks)
            {
                for(blk_cnt; blk_cnt < bna.size(); ++blk_cnt)
                {
                    tBlockNum blknum = bna.at(blk_cnt);

                    delete_block(blknum);
                }
            }
        }
    }
    else
    {
        foreach(tBlockNum blknum, file->get_data_block_nums())
        {
            delete_block(blknum);
        }

        //call add new file directly
        allocate_file_blocks_and_fill_data(inode, loc_fn, size, new_blocks, block_size);
    }
}


void CImageManager::prepare_removed_block_data()
{
    u32 blocks;
    foreach(CBlockGroup *group, block_group_vec)
    {
        tBlockNumList num_lst;
        group->get_released_blocks(num_lst);

        blocks = num_lst.size();

        for(int index = 0; index < blocks; ++index)
        {
            tBlockNum block_num = num_lst.at(index);

            CBlockData *bd = new CBlockData(NULL,//use null to mark as to be delete
                                            block_num);
            register_blockdata(bd);
        }
    }
}

///////////////////////////////////////////////////
/////////////////////////////////////////////////

bool CImageManager::add_new_dir_into_img(CFileInfo *dir)
{
    u64 len = 0;
    u32 dirs = 0;

    get_dir_entry_size(len, dirs, dir);

    if(len == 0)
    {
        return false;
    }

    u32 block_size = android_img.get_block_size();
    int blocks = DIV_ROUND_UP(len, block_size);
    u64 size = blocks * block_size;

    ext4_inode *inode = dir->inode();
    inode->i_links_count = dirs + 2;

    if(allocate_dir_blocks_and_fill_data(inode, dir, size, blocks, block_size))
        return true;
    else
    {
        delete_inode(dir->inode_num(), true);
        return false;
    }
}

bool CImageManager::add_new_file_into_img(CFileInfo *new_file)
{
    QString loc_fn = new_file->get_file_src_path();

    QFileInfo info(loc_fn);

    u64 size = info.size();
    u32 block_size= android_img.get_block_size();
    int blocks = DIV_ROUND_UP(size, block_size);

    ext4_inode *inode = new_file->inode();


    if(allocate_file_blocks_and_fill_data(inode,
                                       loc_fn,
                                       size,
                                       blocks,
                                       block_size))
        return true;
    else
    {
        delete_inode(new_file->inode_num(), false);
    }

    return false;
}

void CImageManager::delete_origin_file(CFileInfo *fi)
{
    if(fi->is_new())
        return;
    //回收inode
    //
    delete_inode(fi->inode_num(), fi->is_dir());
    //回收块
    //
    foreach(tBlockNum blknum, fi->get_data_block_nums())
    {
        delete_block(blknum);
    }
}

void CImageManager::delete_new_added_file(CFileInfo *fi)
{
    if(fi->is_new())
    {
        if(fi->is_dir())
        {
            new_added_dir_list.removeOne(fi);

            tFileInfoList alls;
            fi->get_all_childs(alls);

            foreach(CFileInfo * cur, alls)
            {
                if(cur->is_dir())
                    new_added_dir_list.removeOne(cur);
                else if(cur->is_file())
                    new_added_file_list.removeOne(cur);
                else if(cur->is_link())
                    new_added_link_list.removeOne(cur);

                delete cur;
            }
        }
        else{
            if(fi->is_file())
                new_added_file_list.removeOne(fi);

            if(fi->is_link())
            {
                new_added_link_list.removeOne(fi);
            }
        }

        delete fi;
    }
}

void CImageManager::get_dir_entry_size(u64& len, u32& dirs, CFileInfo *dir)
{
    len = dirs = 0;
    if(!dir->is_dir())
        return;

    u32 block_size = android_img.get_block_size();

#if 1
    int dentry_len= 0;
    len = 24;
    foreach(CFileInfo* cur, dir->get_directly_child_list())
    {
        dentry_len = 8 + ALIGN(cur->filename().size(), 4);
        if ((len % block_size + dentry_len) > block_size)
            len += block_size - (len % block_size);
        len += dentry_len;

        if(cur->is_dir())
            ++dirs;
    }

#else
    const tDirEntryInfoList& sub_dir_lst = dir->get_dir_entry_list();

    int dentry_len;
    len = 24;
    foreach(CDirEntryInfo *cur, sub_dir_lst)
    {
        dentry_len = 8 + ALIGN(cur->name_len(), 4);
        if ((len % block_size + dentry_len) > block_size)
            len += block_size - (len % block_size);
        len += dentry_len;

        if(cur->is_dir())
            ++dirs;
    }
#endif
}

