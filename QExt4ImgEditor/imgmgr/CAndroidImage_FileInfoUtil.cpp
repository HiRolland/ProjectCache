#include "CAndroidImage.h"
#include "CFileInfo.h"
#include "Logger.h"
bool CAndroidImage::detect_layout_ext()
{
    CBlockGroup *group = NULL;

    s_real_free_blocks = 0;
    tBlockNum jnl_first_blk_num = 0;
    tBlockNum jnl_last_blk_num = 0;
    if(m_b_has_jnl)
    {
        jnl_first_blk_num = m_jnl_start_block;
        jnl_last_blk_num = m_jnl_start_block + m_jnl_block_cnt - 1;
    }

    for(u32 i = 0; i < s_total_groups; ++i)
    {
        group = new CBlockGroup(this, i);
        block_group_vec.append(group);
        struct ext2_group_desc*    bgd = &block_group_desc_data[i];
        group->set_group_desc(bgd);

        if(i == (s_total_groups -1))//the last block group
            group->set_block_count( s_total_blocks - i * s_blocks_per_group );
        else
            group->set_block_count( s_blocks_per_group );

        if(m_b_has_jnl)
        {
            if(jnl_first_blk_num >= i * s_blocks_per_group
                    && jnl_first_blk_num <((i + 1) * s_blocks_per_group))
            {
                group->set_has_jnl_block(true);
            }

            if(jnl_last_blk_num >= i * s_blocks_per_group
                    && jnl_last_blk_num <((i + 1) * s_blocks_per_group))

            {
                group->set_has_jnl_block(true);
            }
        }

        s_real_free_blocks += group->free_blocks();
    }

    s_real_free_blocks -= 500;

    if(s_real_free_blocks < 10)
    {
        s_real_free_blocks = 10;
    }

    INFO("free blocks %d", s_real_free_blocks);
    return true;
}

CFileInfo *CAndroidImage::create_fileinfo(u8 file_type,
                                          tInodeNum inode_num,
                                          const QString &filename,
                                          CFileInfo *parent)
{
    ext4_inode *inode = read_inode_from_group(inode_num);
    if(!inode)
    {
        return NULL;
    }
    CFileInfo * info = new CFileInfo(this,
                                     inode_num,
                                     file_type,
                                     inode,
                                     filename,
                                     parent);
    if(!info)
    {
        return NULL;
    }
    file_info_list.append(info);

    return info;
}

void CAndroidImage::build_dir_entry_fileinfo(char *block_data,
                                             CFileInfo *dir)
{
    u32 pos  = 0;
    struct ext4_dir_entry_2 *sub_dir_entry  = NULL;
    QString child_name;

    char *data = block_data;
    sub_dir_entry = (struct ext4_dir_entry_2 *)(data + pos);
    while(pos < s_block_size && sub_dir_entry->inode!=0)
    {
        if(sub_dir_entry->rec_len == 0)
        {
            return;
        }

        child_name = QString::fromUtf8(sub_dir_entry->name, sub_dir_entry->name_len);

        if( child_name == QString(".") || child_name == QString(".."))
        {
            //do noting
            //            if(dir->is_root())
            //                ////qDebug() << sub_dir_entry->inode;
        }
        else{

            CFileInfo *child = create_fileinfo(sub_dir_entry->file_type,
                                               sub_dir_entry->inode,
                                               child_name,
                                               dir);
            if(child)
            {
                if(sub_dir_entry->file_type == EXT4_FT_SYMLINK)
                {
                    ext4_inode* child_inode = read_inode_from_group(sub_dir_entry->inode);
                    QString link_name = QString::fromUtf8((char *)child_inode->i_block,
                                                          child_inode->i_size_lo );
                    child->set_link_to(link_name);
                }
            }
        }

        pos += sub_dir_entry->rec_len;
        sub_dir_entry = (struct ext4_dir_entry_2 *)(data + pos);
    }//while
}

void CAndroidImage::build_dir_entry_fileinfo(CFileInfo *dir)
{
    if(dir->is_new())
        return;

    if(!dir->is_dir())
        return;

    if(dir->has_built_dir_entries())
        return ;

    dir->mark_built_dir_entries();

    if(dir->filename() == "customized")
    {
        int idex = 0;
        idex += 1;
    }

    //dir->set_build_dir_entries_tag();

    ext4_inode * inode = dir->inode();
    char data[4096];
    bool inited_ = false;

    //先判断是否为exent,且若是叶子节点,则直接读取,否则用查找方式
    //
    if(INODE_HAS_EXTENT(inode))
    {
        struct ext4_extent_header *header;
        struct ext4_extent *extent;
        header = get_ext4_header(inode);

        if(header->eh_magic == EXT4_EXT_MAGIC && header->eh_depth == 0)
        {
            inited_ = true;
            extent = EXT_FIRST_EXTENT(header);

            //逐个extent读取
            //
            for(int extent_index = 0; extent_index < header->eh_entries; ++extent_index)
            {
                extent += extent_index;
                tBlockNum block_base = ext_to_block(extent);
                for(int index = 0; index < extent->ee_len; ++index)
                {
                    memset(data, 0, s_block_size);
                    read_disk(data, block_base + index, 1, s_block_size);

                    build_dir_entry_fileinfo(data, dir);

                }//for-inner
            }//for-outer
        }
    }

    if(!inited_){
        int file_size = ((u64)inode->i_size_lo + (((u64)inode->i_size_high)<<32));
        int block_count = DIV_ROUND_UP(file_size, s_block_size);
        tBlockNum block;

        ////获取文件尺寸和所占block数目
        for(int blkindex = 0; blkindex < block_count; blkindex++)//可能有一个块未满
        {
            block = get_physic_block_num(inode, blkindex);
            if(block == -1)
            {
                break;
            }
            else{
                memset(data, 0, s_block_size);
                read_disk(data, block, 1, s_block_size);

                build_dir_entry_fileinfo(data, dir);
            }
        }
    }
}

void CAndroidImage::get_root_file_list()
{
    root_fileinfo = create_fileinfo(EXT4_FT_DIR, EXT4_ROOT_INO, "/", NULL);

    build_dir_entry_fileinfo(root_fileinfo);


}

CFileInfo *CAndroidImage::get_root_dir_file(const QString &filename)
{
    const QList<CFileInfo* >& lst = root_fileinfo->get_directly_child_list();
    foreach(CFileInfo* cur, lst)
    {
        if(cur->filename() == filename)
        {
            return cur;
        }
    }

    return NULL;
}



