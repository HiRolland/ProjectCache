#include "CImageManager.h"

bool CImageManager::register_blockdata(CBlockData *newd)
{
    int ipos = 0;
    foreach(CBlockData *cur, m_mod_blockdata_ascend_lst)
    {
        if(newd->start_block_num() > cur->start_block_num())
            ++ipos;
        else
            break;
    }
    if(ipos < m_mod_blockdata_ascend_lst.size())
        m_mod_blockdata_ascend_lst.insert(ipos, newd);
    else
        m_mod_blockdata_ascend_lst.append(newd);

    return true;
}

void CImageManager::clear_new_block_data_list()
{
    foreach(CBlockData *cur, m_mod_blockdata_ascend_lst)
    {
        delete cur;
    }

    m_mod_blockdata_ascend_lst.clear();
}

BlockMemoryBuffer *CImageManager::allocate_block_buffer(u32 blocks, u32 block_size)
{
    if(blocks <= 0)
        return NULL;
    int len = blocks * block_size * sizeof(u8);
    BlockMemoryBuffer * buf = (BlockMemoryBuffer *)calloc(1,sizeof(BlockMemoryBuffer) + len);
    if(buf)
    {
        buf->blocks = blocks;
        block_buffer_lst.append(buf);
        return buf;
    }
    return NULL;

}

void CImageManager::clear_block_buffer_list()
{
    foreach(BlockMemoryBuffer * buf, block_buffer_lst)
    {
        free(buf);
    }

    block_buffer_lst.clear();
}

