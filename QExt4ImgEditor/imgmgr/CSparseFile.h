#ifndef CSPARSEFILE_H
#define CSPARSEFILE_H

#include <QList>
#include <QString>
#include <QFile>
class CChunk;
class QFile;
class CSparseFile
{
public:
    CSparseFile(const QString& fn);
    ~CSparseFile();
    void clear();
    bool empty(){return m_chk_lst.isEmpty();}
    void set_filename(const QString& fn){m_img_fn = fn;}
    bool load_sparse();

    /**
     * @brief convert_to_ext4
     * @param out_ext_fn
     */
    void convert_to_ext4(const QString& out_ext_fn);

    /**
     * @brief create_new_sparse
     * @param ext4_fn
     * @param new_sparse_fn
     */
    void create_sparse_on_ext4(const QString& in_ext4_fn, const QString& out_sparse_fn);

    void create_sparse(const QString& ext_fn, const QString& sparse_fn);

    /**
     * @brief read_chunk_data 从offset_block块号开始读取 n_blocks个块到ptr中
     * @param ptr
     * @param offset_blocks
     * @param n_blocks
     */
    void read_chunk_data(char *ptr, int offset_block, int n_blocks);

    /**
     * @brief update_block 更新blocknum上的数据
     * @param data
     * @param block_num
     */
    void update_block(char *data, int block_num);

    /**
     * @brief erase_block_data 清除block上的数据
     * @param block_num
     */
    void erase_block_data(int block_num);

    /**
     * @brief fill_block 申请blocknum,并填充上面的数据
     * @param data
     * @param block_num
     */
    void fill_block(char *data, int block_num);
private:
    bool load_normal(QFile *f);
    bool load_samsung(QFile *f);
    bool load_marvell(QFile *f);

    void dump_raw_chunk (CChunk* chk, QFile *in_sparse, QFile *out_ext4);
    void dump_fill_chunk(CChunk* chk, QFile *in_sparse, QFile *out_ext4);
    void dump_skip_chunk(CChunk* chk, QFile *in_sparse, QFile *out_ext4);

    void create_raw_chunk (CChunk *chk, QFile *in_ext4, QFile *out_sparse);
    void create_fill_chunk(CChunk *chk, QFile *in_ext4, QFile *out_sparse);
    void create_skip_chunk(CChunk *chk, QFile *in_ext4, QFile *out_sparse);

    char *block_buffer(unsigned int  init = 0);
    inline int block_size(){return m_block_sz;}

    void clear_chunk_list();

    CChunk *find_chunk_on_blocknum(int blk_num);
private:
    QString m_img_fn;
    QFile   m_fd;
    QList<CChunk*> m_chk_lst;

    int m_block_sz;
    enum tSparseType
    {
        tSparseType_Normal,
        tSparseType_Samsung,
        tSparseType_Marvell,
    };

    tSparseType m_sparse_type;

    char *m_blk_buf;

    CChunk *m_search_cache;
    bool m_b_crc32;
};

#endif // CSPARSEFILE_H
