#include "CImageManager.h"
#include <QFile>
#include "CChunk.h"
//#include "CImageHandlerExt.h"

void CImageManager::complement_group_new_data_block()
{
    foreach(CBlockGroup * group, block_group_vec)
    {
#if 1
        if(group->has_super_block() /*&& !group->get_group_index()*/)
        {
            //
            //TODO:当组描述块大于1时,判断具体哪一个块变了
            //
            // 和superblock同时申请缓冲区,+1
            int blk_cnt = android_img.get_bg_desc_blocks() + 1;
            BlockMemoryBuffer *buf = allocate_block_buffer(blk_cnt, block_size());
            CBlockData *bd = new CBlockData(buf->data,
                                            group->get_group_index() * android_img.get_blocks_per_group(),
                                            blk_cnt);
            register_blockdata(bd);

            //first, init the super block block
            u8* sb_buf = bd->data_buffer();
            memcpy(sb_buf, group->super_block_data(), block_size());

            //then init group desc block
            u8* group_des_buf = bd->data_buffer() + block_size();
            memcpy(group_des_buf,
                   android_img.get_group_desc_data(),
                   android_img.get_total_groups() * sizeof(ext2_group_desc));

        }

        //
        if(group->block_changed())
        {
            BlockMemoryBuffer *buf = allocate_block_buffer(1, block_size());
            CBlockData *bd = new CBlockData(buf->data,
                                            group->block_bitmap_block_num() );
            register_blockdata(bd);

            memcpy(bd->data_buffer(), group->block_bitmap(), block_size());

        }

        //更新inode bitmap
        //
        if(group->inode_changed())
        {
            BlockMemoryBuffer *buf = allocate_block_buffer(1, block_size());
            CBlockData *bd = new CBlockData(buf->data,
                                            group->inode_bitmap_block_num() );
            register_blockdata(bd);

            memcpy(bd->data_buffer(), group->inode_bitmap(), block_size());
        }
#endif
        const tInodeIndexInGroupArray& inode_indices = group->get_inode_changed_index();
        if(!inode_indices.isEmpty() && group->inode_table())
        {
            tBlockNumArray cached;//缓存已经处理了的inodetable的块号

            u32 inode_size= android_img.get_inode_size();
            foreach(tInodeIndexInGroup idx, inode_indices)
            {
                u64 len = (u64)idx * (u64)inode_size;
                int blk_off = len / block_size();//块数偏移
                tBlockNum ibnum = blk_off + group->inode_table_block_num();

                // 按块处理
                // 如果包含了这个块，则在上一次循环处理中已经处理了该inode
                //
                if(!cached.contains(ibnum))
                {
                    cached.append(ibnum);

                    BlockMemoryBuffer *buf = allocate_block_buffer(1, block_size());
                    CBlockData *bd = new CBlockData(buf->data, ibnum );
                    register_blockdata(bd);

                    memcpy(bd->data_buffer(),
                           group->inode_table() + blk_off * block_size(),
                           block_size());

                }
            }
        }
    }
}

/**
 * @brief CImageManager::write_to_image 将变更写回img
 * 要变更的内容:
 *
 * 1, 处理删除操作,在block中清空的要删除的inode和bloc对应的bitmap,但不做实际操作
 * 2, 处理添加新文件(文件夹)操作
 * 3, 根据2,3操作,整合获取最终要更新的数据:对最终要清空的inode和block进行写 0清空
 * 4, 准备写回superblock 和 bg_desc,其中每个bg_desc要重新计算checksum
 *
 * 注意:2,3操作顺序不能颠倒,因为,2中标记删除的inode或block可以在3中重新分配
 *
 */
bool CImageManager::prepare_update_data()
{
    if(origin_changed_dir_lst.isEmpty()
            && new_added_dir_list.isEmpty()
            && new_added_file_list.isEmpty()
            && origin_changed_file_lst.isEmpty()
            )
        return false;
#if 1
    //        qDebug() << "update img : " << origin_changed_dir_lst.size()
    //                 << new_added_dir_list.size()
    //                 << new_added_file_list.size()
    //                 << origin_changed_file_lst.size();
    //step1  porcess delete operation
    //
    //////qDebug() << "process delete operation";
    foreach (CFileInfo *cur, origin_changed_dir_lst) {
        process_origin_dir_removed_entries(cur);
    }

    //step2: update origin modified dir    //
    foreach (CFileInfo *cur, origin_changed_dir_lst) {
        update_origin_dir_into_img(cur);
    }

    foreach (CFileInfo *cur, origin_changed_file_lst) {
        update_origin_file_into_img(cur);
    }

    //step3 do add new dir operation
    foreach(CFileInfo *cur, new_added_dir_list)
    {
        add_new_dir_into_img(cur);
    }

    //step4 add new file
    //
    foreach(CFileInfo *cur, new_added_file_list)
    {
        add_new_file_into_img(cur);
    }

    // prepare remove block on sparse img
    prepare_removed_block_data();

    foreach(CBlockGroup * cur, block_group_vec)
    {
        if(cur->has_changed())
        {
            cur->recal_checksum();
        }
    }
#endif

    complement_group_new_data_block();

    return true;
}

void CImageManager::build_modified_block_list(tModBlkLst &lst)
{
    tBlockNum last_num = 0;
    foreach(CBlockData* bd, m_mod_blockdata_ascend_lst)
    {
        u8 *d = bd->data_buffer();
        tBlockNum bnum = bd->start_block_num();


        last_num = bnum;
        for(int i = 0; i < bd->block_count(); ++i)
        {
            tModifiedBlock *b = new tModifiedBlock(bnum + i,
                                                   bd->is_erase_block() ? NULL : (d + i * block_size()));

            lst.append(b);
        }
    }
}

bool CImageManager::flush_data_into_ext4_img(const QString &new_ext)
{
#if 0
    foreach(CBlockData *cur, m_mod_blockdata_ascend_lst)
    {
        android_img.write_disk((char*)cur->data_buffer(),
                               cur->block_count() * block_size(),
                               cur->start_block_num(),
                               block_size());
    }
#else

    bool bret = false;
    int written_blks = 0;
    do
    {
        QFile *org_f = android_img.file_handler();
        if(!org_f)
            break;

        if(m_mod_blockdata_ascend_lst.isEmpty())
        {
            bret = org_f->copy(new_ext);
        }
        else
        {
            if(!org_f->seek(0))
                break;

            QFile new_f(new_ext);
            if(!new_f.open(QIODevice::WriteOnly))
                break;

            int blk_cnt = org_f->size() / block_size();

            //qint64 fz = android_img.
            char * blk_buf = (char*)calloc(1, block_size());


            tModBlkLst mod_lst;
            build_modified_block_list(mod_lst);

            int cur_mod_idx = 0;
            tModifiedBlock * cur_mod = mod_lst.at(cur_mod_idx);

            for(int blk = 0; blk < blk_cnt; ++blk)
            {
                if(cur_mod && blk == cur_mod->blk_num)
                {
                    //qDebug() << cur_mod->blk_num;
                    if(cur_mod->data)
                    {
                        new_f.write((char*)cur_mod->data, block_size());
                    }
                    else
                    {
                        //erase this block
                        memset(blk_buf, 0, block_size());

                        new_f.write(blk_buf, block_size());
                    }

                    ++cur_mod_idx;
                    if(cur_mod_idx < mod_lst.size())
                    {
                        cur_mod = mod_lst.at(cur_mod_idx);
                    }
                    else
                    {
                        cur_mod = NULL;
                    }
                }
                else
                {
                    if(org_f->seek(blk * block_size()) && org_f->read(blk_buf, block_size()))
                    {
                        new_f.write(blk_buf, block_size());
                    }
                    else
                    {
                        bret = (cur_mod_idx >= mod_lst.size());
                        break;
                    }
                }
                ++written_blks;


                if(m_pack_hdl_ext)
                {
                    int prog = 80 + blk * 100 / (blk_cnt * 5) ;

                    //m_pack_hdl_ext->create_progress(prog, prog, QObject::tr("last stepping . . ."));
                }
            }
        }
    }while(0);

    return bret;
#endif
}


bool CImageManager::flush_data_into_sparse_img(const QString &sparse_fn)
{
    QFile new_s(sparse_fn);
    bool bret = false;
    do
    {
        if(!new_s.open(QIODevice::WriteOnly))
            break;

        QFile *org_s = android_img.file_handler();
        if(!org_s->isOpen())
            break;

        if(!org_s->seek(0))
            break;
#if 1
        u32 old_chunks = 0;
        sparse_hdr* sh = android_img.get_sparse_hdr();
        if(sh->type == tImgType_sparse_normal)
        {
            old_chunks = sh->n_hdr.total_chunks;
            new_s.write((char*)&sh->n_hdr, sizeof(normal_sparse_header_t));
        }
        else if(sh->type == tImgType_sparse_samsung)
        {
            old_chunks = sh->s_hdr.total_chunks;
            new_s.write((char*)&sh->s_hdr, sizeof(samsung_sparse_header_t));
        }
        else if(sh->type == tImgType_sparse_marvell)
        {
            old_chunks = sh->m_hdr.total_chunks;
            new_s.write((char*)&sh->m_hdr, sizeof(marvell_sparse_header_t));
        }
#endif
        //todo:check sparse hdr padding

        const QList<CChunk* > & chunk_lst = android_img.get_chunk_list();

        tModBlkLst mod_lst;

        build_modified_block_list(mod_lst);

        u32 chunk_count = write_multi_chunks(&new_s,
                                             org_s,
                                             chunk_lst,
                                             mod_lst);

        if(old_chunks != chunk_count)
        {
            //update sparse header
            if(!new_s.seek(0))
            {
                bret = false;
                new_s.close();
                break;
            }

            if(sh->type == tImgType_sparse_normal)
            {
                sh->n_hdr.total_chunks = chunk_count;
                new_s.write((char*)&sh->n_hdr, sizeof(normal_sparse_header_t));
            }
            else if(sh->type == tImgType_sparse_samsung)
            {
                sh->s_hdr.total_chunks = chunk_count;
                new_s.write((char*)&sh->s_hdr, sizeof(samsung_sparse_header_t));
            }
            else if(sh->type == tImgType_sparse_marvell)
            {
                sh->m_hdr.total_chunks = chunk_count;
                new_s.write((char*)&sh->m_hdr, sizeof(marvell_sparse_header_t));
            }
        }

        new_s.close();

        bret = true;

        foreach(tModifiedBlock* b, mod_lst)
        {
            delete b;
        }

    }while(0);

    return bret;
}

u32 CImageManager::write_multi_chunks(QFile *news,
                                      QFile *orgs,
                                      const QList<CChunk *> &chk_lst,
                                      const tModBlkLst &all_mod_lst)
{
    u32 chunk_count = 0;

    int mdb_next_pos = 0;//当前第一个未处理的mdblk位置
    tModifiedBlock *mfirst_blk  = NULL;
    int idx = 0;
    int idx_size = chk_lst.size();
    foreach(CChunk *chk, chk_lst)
    {
        if(mdb_next_pos < all_mod_lst.size())
        {
            mfirst_blk = all_mod_lst.at(mdb_next_pos);
            if(chk->last_block() < mfirst_blk->blk_num)
            {
                chunk_count++;
                write_origin_chunk(news, orgs, chk);
            }
            else
            {
                // 搜寻本chunk对应的修改的块
                //
                tModBlkLst cov_lst;
                int pos = 0;
                for(int i = 0;;++i)
                {
                    pos = mdb_next_pos + i;

                    if(pos >= all_mod_lst.size())
                        break;

                    mfirst_blk = all_mod_lst.at(pos);

                    if(chk->contains_block(mfirst_blk->blk_num))
                    {
                        cov_lst.append(mfirst_blk);
                    }
                    else
                    {
                        break;
                    }
                }

                mdb_next_pos += cov_lst.size();

                if(!cov_lst.isEmpty())
                {
                    chunk_count += write_modified_chunk(news, orgs, chk, cov_lst);
                }
                else
                {
                    chunk_count++;
                    write_origin_chunk(news, orgs, chk);
                }
            }
        }
        else
        {
            chunk_count++;
            write_origin_chunk(news, orgs, chk);
        }


        if(m_pack_hdl_ext)
        {
            ++idx;
            int prog = 80 + idx * 100 / (idx_size * 5) ;

            //m_pack_hdl_ext->create_progress(prog, prog, QObject::tr("last stepping . . ."));
        }
    }

    return chunk_count;
}

void CImageManager::write_origin_chunk(QFile *news,
                                       QFile *orgs,
                                       CChunk *chk)
{
    //write chunk header
    if(android_img.is_normal_sparse())
    {
        news->write((char*)chk->normal_chunk(), sizeof(normal_chunk_header_t));
    }
    else if(android_img.is_marvell_sparse())
    {
        news->write((char*)chk->marvell_chunk(), sizeof(marvell_chunk_header_t));
    }
    else if(android_img.is_samsung_sparse())
    {
        news->write((char*)chk->samsung_chunk(), sizeof(samsung_chunk_header_t));
    }
    //write data
    if(chk->is_raw())
    {
        orgs->seek(chk->sparse_data_offset());
        char *buf = (char*) calloc(1, block_size());

        for(int i = 0; i < chk->block_count(); ++i)
        {
            orgs->read(buf, block_size());
            news->write(buf, block_size());
        }

        free(buf);
    }
    else if(chk->is_fill())
    {
        u32 fv = chk->fill_val();
        news->write((char*)&fv, sizeof(fv));
    }
}

u32 CImageManager::write_modified_chunk(QFile *news,
                                        QFile *orgs,
                                        CChunk *chk,
                                        const tModBlkLst &mod_lst)
{

    if(android_img.is_normal_sparse())
    {
        return write_modified_chunk_normal(news, orgs, chk, mod_lst);
    }
    else if(android_img.is_marvell_sparse())
    {
        return write_modified_chunk_marvell(news, orgs, chk, mod_lst);
    }
    else if(android_img.is_samsung_sparse())
    {
        return write_modified_chunk_samsung(news, orgs, chk, mod_lst);
    }

    return 0;
}

