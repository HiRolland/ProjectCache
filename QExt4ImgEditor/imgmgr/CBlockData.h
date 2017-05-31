#ifndef CBLOCKDATA_H
#define CBLOCKDATA_H

#include "mogu.h"
#include <QList>

/**
 * @brief The CModifiedBlock class 表示将要被删除的一个块
 * 注意：
 */
struct tModifiedBlock
{
    tModifiedBlock(int num, u8*d)
        : blk_num(num)
        , data(d)
    {}
    int blk_num;
    u8  *data;
};

typedef QList<tModifiedBlock*> tModBlkLst;

/**
 * @brief The CBlockData class
 * 数据改动的连续的块的数据
 * 只记录缓冲区域对应的数据块号不实际保存数据
 * 覆盖1个或连续的多个块
 */
class CBlockData
{
public:
    explicit CBlockData(u8* buf, tBlockNum startnum, int blocks = 1);
    ~CBlockData();

    tBlockNum start_block_num(){return m_start_block_num;}
    inline u8* data_buffer(){return data;}
    int block_count(){return m_block_count;}

    inline bool is_erase_block(){return ((data == NULL) || (data == 0));}
private:
    tBlockNum m_start_block_num;
    int m_block_count;
    enum
    {
        BlockSize = 4096,
    };
    u8 *data;
};

typedef QList<CBlockData*> tBlockDataList;


#endif // CBLOCKDATA_H
