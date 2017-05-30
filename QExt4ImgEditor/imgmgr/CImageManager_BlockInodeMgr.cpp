#include "CImageManager.h"

//implements of block and  inode allocate

tBlockNum CImageManager::allocate_continus_blocks(int nblocks)
{
    tBlockNum first = 0;
    foreach(CBlockGroup* group, block_group_vec)
    {
        if(group->get_group_index() == 0)
            continue;

        first = group->allocate_contiguous_blocks(nblocks);

        if(first > 0)
        {
            return first;
        }
    }

    return 0;
}

bool CImageManager::allocate_partial_blocks(int nblocks, tBlockRegionList &ret)
{
    ret.clear();

    int free_blk_cnt = 0;
    //
    // 根据剩余块数降序排列
    //
    tBlockGroupVec descend_vec;

    foreach(CBlockGroup * cur, block_group_vec)
    {
        free_blk_cnt += cur->free_blocks_count();

        int idx = 0;
        for(idx = 0; idx <descend_vec.size(); ++idx)
        {
            if(cur->free_blocks_count() > descend_vec.at(idx)->free_blocks_count())
            {
                break;
            }
        }

        descend_vec.insert(idx, cur);
    }

    if(free_blk_cnt < nblocks)
    {
        return false;
    }
    else
    {
        int need_blocks = nblocks;
        foreach(CBlockGroup * cur, descend_vec)
        {
            need_blocks -= cur->allocate_fitted_contiguous_blocks(ret,  need_blocks);

            if(need_blocks <= 0)
            {
                break;
            }
        }
    }

    int alloc_blks = 0;
    foreach(BlockRegion *reg, ret)
    {
        alloc_blks += reg->len;
    }

    if(alloc_blks == nblocks)
    {
        INFO("alloc ok!");
        return true;
    }
    else
    {
        INFO("should not run here!!");
        return false;
    }

}


void CImageManager::delete_block(tBlockNum num)
{
    foreach(CBlockGroup * group, block_group_vec)
    {
        if(group->contains_block(num))
        {
            group->clear_block(num);
            break;
        }
    }
}

void CImageManager::delete_inode(tInodeNum num, bool isdir)
{
    foreach(CBlockGroup * group, block_group_vec)
    {
        if(group->contains_inode(num))
        {
            group->clear_inode(num, isdir);
            break;
        }
    }
}

bool CImageManager::is_block_used(tBlockNum num)
{
    foreach(CBlockGroup * group, block_group_vec)
    {
        if(group->contains_block(num))
        {
            return group->is_this_block_used(num);
        }
    }

    return false;
}

void CImageManager::mark_block_used(tBlockNum num)
{
    foreach(CBlockGroup * group, block_group_vec)
    {
        if(group->contains_block(num))
        {
             group->mark_block_used(num);
             return;
        }
    }

}


void CImageManager::add_changed_inode(tInodeNum num)
{
    foreach(CBlockGroup *cur, block_group_vec)
    {
        if(cur->add_changed_inode(num))
            return;
    }
}

