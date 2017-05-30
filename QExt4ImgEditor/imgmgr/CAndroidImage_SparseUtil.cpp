#include "CAndroidImage.h"
#include "CChunk.h"
#include <QDebug>
void CAndroidImage::init_sparse_info()
{
    m_search_cache = NULL;
}

void CAndroidImage::clear_sparse_info()
{
    m_search_cache = NULL;
    foreach(CChunk *ck, m_chk_lst)
    {
        delete ck;
    }
    m_chk_lst.clear();
}

CChunk *CAndroidImage::find_chunk_on_blocknum(int blk_num)
{
    if(m_chk_lst.isEmpty()) return NULL;

    if(m_search_cache && m_search_cache->contains_block(blk_num))
        return m_search_cache;
    else
    {
        int low = 0, high = m_chk_lst.size() - 1;
        int mid;

        //use binary search to speed up
        while(low <= high)
        {
            mid = (low + high) / 2;
            m_search_cache = m_chk_lst.at(mid);
            if(m_search_cache->contains_block(blk_num))
            {
                return m_search_cache;
            }

            if(blk_num < m_search_cache->start_block())
            {
                high = mid;
            }
            else
            {
                low = mid;
            }
        }
    }

    m_search_cache = NULL;
}

qint64 CAndroidImage::read_chunk_data(char *ptr,
                                      tBlockNum offset_block,
                                      int n_blocks,
                                      int block_size)
{
#if 1

    qint64 low, mid, high;
    qint64 offset;

    qint64 len;
    qint64 read_len;

    offset = offset_block * block_size;//chunk 位置
    len = n_blocks * block_size;

    low = 0;
    high = m_chk_lst.size() - 1;
    qint64 chk_idx = 0;
    while (low <= high) {
        mid = (low + high) / 2;
        CChunk *chk = m_chk_lst.at(mid);
        if(offset >= chk->extfs_data_offset() &&
           offset < (chk->extfs_data_offset() + chk->ext_data_size()))
        {
            // offset in current chunk
            // in fact, only the first chunk need this value.
            // cz, if we read more than one chunk ,the subsequent
            // must start with offset at 0.
            qint64 offset_in_chunk = offset - chk->extfs_data_offset();
            u32 read_len_in_chunk = 0;

            u32 chk_remain_len = 0;
            while(len > 0)
            {
                 CChunk *chk = m_chk_lst.at(mid + chk_idx);
                 chk_remain_len = chk->ext_data_size() - offset_in_chunk;
                 read_len_in_chunk = len < chk_remain_len ? len : chk_remain_len;

                 qint64 pos = chk->sparse_data_offset() + offset_in_chunk;
                 if(! img_file_handler.seek(pos))
                 {
                     return -1;
                 }
                 if(chk->is_raw())
                 {
                    read_len = img_file_handler.read(ptr, read_len_in_chunk);
                    if(read_len != read_len_in_chunk)
                        return -1;
                 }
                 else if(chk->is_fill())
                 {
                     int fill_count = read_len_in_chunk / sizeof(chk->fill_val());

                     for (int i = 0; i < fill_count; i++)
                         ((u32 *)ptr)[i] = chk->fill_val();
                 }
                 else if(chk->is_skip())
                 {
                     memset(ptr, 0, read_len_in_chunk);
                     break;
                 }

                 ptr += read_len_in_chunk;
                 len -= read_len_in_chunk;
                 chk_idx++;
                 offset_in_chunk = 0;
            }

            return n_blocks * block_size;
        }
        else if (chk->extfs_data_offset() > offset)
            high = mid - 1;
        else
            low = mid + 1;
    }

    //qDebug() << "find faild" << offset << (m_chk_lst.last()->extfs_data_offset() + m_chk_lst.last()->ext_data_size()) << m_chk_lst.last()->start_block() * 4096;
    return -1;
#else

    int blk_num;

    int rd_blks = 0;
    int to_rd_blks, can_rd_blks;

    int chk_blk_offset, chk_blk_remain;
    while(rd_blks < n_blocks)
    {
        to_rd_blks = n_blocks - rd_blks;

        blk_num = offset_block + rd_blks;
        CChunk *chk = find_chunk_on_blocknum(blk_num);

        if(chk)
        {
            //
            //获取本chunk覆盖的目标块数 以便一并读取
            //
            chk_blk_offset = blk_num - chk->start_block();
            chk_blk_remain = chk->block_count() - chk_blk_offset;
            can_rd_blks = chk_blk_remain > to_rd_blks ? to_rd_blks : chk_blk_remain;


            if(chk->is_raw())
            {
                qint64 pos = chk->sparse_data_offset() + chk_blk_offset * block_sz;
                img_file_handler.seek(pos);
                img_file_handler.read(ptr, block_sz * can_rd_blks);
            }
            else if(chk->is_fill())
            {
                memset(ptr, chk->fill_val(), block_sz);
            }
            else if(chk->is_skip())
            {
                memset(ptr, 0, block_sz);
            }

            ptr += can_rd_blks * block_sz;
            rd_blks += can_rd_blks;
        }
        else
        {
            return rd_blks * block_sz;
        }
    }

    return rd_blks * block_sz;
#endif
}
