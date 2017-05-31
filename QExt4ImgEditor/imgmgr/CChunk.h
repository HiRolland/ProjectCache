#ifndef CCHUNK_H
#define CCHUNK_H
#include <QtGlobal>
#include <QList>
#include "CBlockData.h"
#include "sparse_format.h"

class CChunk
{
public:

    /**
     * @brief CChunk
     * @param f ：关联的sparse文件的,可以为空
     * @param hdr
     * @param first_mapped_block 本chunk对应的第一个ext中的数据块号
     * @param sparse_data_offset 本chunk对应的sparse文件中的起始数据偏移
     * @param blk_size  block的数据块大小
     */
    explicit CChunk(
                    normal_chunk_header_t *hdr,
                    tBlockNum first_mapped_block,
                    qint64 sparse_data_offset,
                    int blk_size = 4096);
    explicit CChunk(samsung_chunk_header_t *hdr,
                    tBlockNum first_mapped_block,
                    qint64 sparse_data_offset,
                    int blk_size = 4096);
    explicit CChunk(marvell_chunk_header_t *hdr,
                    tBlockNum first_mapped_block,
                    qint64 sparse_data_offset,
                    int blk_size = 4096);


    /**
     * @brief is_normal_type 判断sparse类型
     * @return
     */
    inline bool is_normal() const{return m_sparse_format_type == tSparseFormat_Normal;}
    inline bool is_samsung() const{return m_sparse_format_type == tSparseFormat_Samsung;}
    inline bool is_marvell() const{return m_sparse_format_type == tSparseFormat_Marvell;}

    const normal_chunk_header_t   *normal_chunk() { return is_normal() ? &m_chunk.normal : NULL;}
    const samsung_chunk_header_t  *samsung_chunk() { return is_samsung() ? &m_chunk.samsung : NULL;}
    const marvell_chunk_header_t  *marvell_chunk() { return is_marvell() ? &m_chunk.marvell : NULL;}

    /*
     *本chunk的类别,
     */
    inline bool is_raw()  const{return m_chk_type == CHUNK_TYPE_RAW;}
    inline bool is_fill()  const{return m_chk_type == CHUNK_TYPE_FILL;}
    inline bool is_skip()  const{return m_chk_type == CHUNK_TYPE_DONT_CARE;}
    inline bool is_crc32() const{return m_chk_type == CHUNK_TYPE_CRC32;}

    inline u32 block_size() const{return m_blk_size;}
    /**
     * @brief start_block 本chunk对应的ext4的起始数据块的块号
     * @return
     */
    inline int start_block() const{return m_start_block;}
    inline int last_block()const {return m_start_block + block_count() - 1;}
    /**
     * @brief block_count 本chunk对应的数据块的数目
     * @return
     */
    inline int block_count() const{return m_block_cnt_in_ext;}

    /**
     * @brief ext_data_size 本chunk对应的数据长度,不包含header
     * @return
     */
    inline int ext_data_size() const{return m_chunk_data_size;}

    /**
     * @brief sparse_data_offset 本chunk在sparse文件中的数据块起始偏移(不包含header)
     * @return
     */
    inline qint64 sparse_data_offset() const{return m_sdata_offset;}

    /**
     * @brief extfs_data_offset 本chunk对应的ext4文件系统的起始数据偏移
     * @return
     */
    inline qint64 extfs_data_offset() const{return m_ndata_offset;}

    bool contains_block(int blk_num) const{return blk_num >= m_start_block && blk_num < (m_start_block + m_block_cnt_in_ext);}

    /**
     * @brief set_fill_val 若本chunk是fill类型的chunk,则填充fill,否则不用关心
     * @param val
     */
    inline void set_fill_val(u32 val){m_fill_val = val;}
    inline const u32& fill_val() const{return m_fill_val;}
private:
    void init(int chk_type, int chk_sz,
              qint64 first_mapped_block,
              qint64 sparse_data_offset,
              int blk_size);

    enum tSparseFormat
    {
        tSparseFormat_Normal,
        tSparseFormat_Samsung,
        tSparseFormat_Marvell,

    };
    union thechunk
    {
        normal_chunk_header_t normal;
        samsung_chunk_header_t samsung;
        marvell_chunk_header_t marvell;
    };

    thechunk m_chunk;

    //seems too verbose
    u32 m_blk_size;

    /**
     * @brief m_type normal,samsung or mavell
     */
    tSparseFormat m_sparse_format_type;

    /**
     * @brief m_fill_val
     * 若是填充类型的chunk则记录填充值，否则忽略
     *
     */
    u32 m_fill_val;

    /**
     * @brief m_chk_type
     * chunk类别
     */
    int m_chk_type;

    /**
     * @brief m_chunk_data_size
     */
    int m_chunk_data_size;

    /**
     * @brief m_sdata_offset
     * sparse文件中本chunk的数据起始偏移
     */
    qint64 m_sdata_offset;

    /**
     * @brief m_start_block
     * 对应的在ext4中的起始block块号
     */
    tBlockNum m_start_block;
    /**
     * @brief m_ndata_offset
     * 本chunk对应的在ext4中的数据块起始偏移
     */
    qint64 m_ndata_offset;

    /**
     * @brief m_block_cnt_in_ext 覆盖块的数量
     */
    int m_block_cnt_in_ext;

};

#endif // CCHUNK_H
