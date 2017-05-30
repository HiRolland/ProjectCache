
#include "CSparseFile.h"
#include "CChunk.h"

CChunk *CSparseFile::find_chunk_on_blocknum(int blk_num)
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


void CSparseFile::read_chunk_data(char *ptr, int offset_block, int n_blocks)
{
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
                qint64 pos = chk->sparse_data_offset() + chk_blk_offset * block_size();
                m_fd.seek(pos);
                m_fd.read(ptr, block_size() * can_rd_blks);
            }
            else if(chk->is_fill())
            {
                memset(ptr, chk->fill_val(), block_size());
            }
            else if(chk->is_skip())
            {
                memset(ptr, 0, block_size());
            }

            ptr += can_rd_blks * block_size();
            rd_blks += can_rd_blks;
        }
        else
        {
            return;
        }
    }
}

void CSparseFile::update_block(char *data, int block_num)
{

}

void CSparseFile::erase_block_data(int block_num)
{

}

void CSparseFile::fill_block(char *data, int block_num)
{

}
