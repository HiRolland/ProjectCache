#include "CAndroidImage.h"
#include <QFile>
#include "CChunk.h"
#include <QDebug>
#include "Logger.h"

#if 1
bool CAndroidImage::load_normal_chunk(QFile *f)
{
    //    if(!empty())
    //        return true;

    INFO("is normal ");
    normal_sparse_header_t sparse_hdr;
    normal_chunk_header_t chunk_hdr;

    int spr_hdr_len = NORMAL_SPARSE_HEADER_LEN;
    int chk_hdr_len = NORMAL_CHUNK_HEADER_LEN;
    //f->seek(0);
    int read_len = f->read((char*)&sparse_hdr, spr_hdr_len);
    if(read_len != spr_hdr_len)
    {

        return false;
    }

    if(sparse_hdr.magic != SPARSE_HEADER_MAGIC)
    {
        return true;
    }

    m_sparse_hdr.type = tImgType_sparse_normal;

    memcpy(&m_sparse_hdr.n_hdr, &sparse_hdr, sizeof(sparse_hdr));
    if (sparse_hdr.file_hdr_sz > spr_hdr_len) {
        ////如果读到的头大小与实际大小不符，则跳到实际位置，即设置到sparse_hdr.file_hdr_sz处

        INFO("Skip spr_hdr_len!!");
        int pad_len = sparse_hdr.file_hdr_sz - spr_hdr_len;
        m_sparse_hdr.pading_len = pad_len;

        qint64 pos = f->pos() + pad_len;
        if(!f->seek(pos))
        {
            return false;
        }
    }

    int block_offset = 0;
    qint64 read_offset = f->pos();
    for (int i = 0; i < sparse_hdr.total_chunks; i++) {

        read_len = f->read((char*)&chunk_hdr, sizeof(chunk_hdr));
        if(read_len != chk_hdr_len){

            return false;
        }

        if (sparse_hdr.chunk_hdr_sz > chk_hdr_len) {
            /* Skip the remaining bytes in a header that is longer than
             * we expected.
             */
            INFO("Skip size!!");
            qint64 pos = f->pos() + (sparse_hdr.chunk_hdr_sz - chk_hdr_len);
            f->seek(pos);
        }

        if(chunk_hdr.chunk_type != CHUNK_TYPE_CRC32)
        {
            //CChunk *chk = new CChunk(&chunk_hdr, block_offset, f->pos());
            CChunk *chk = new CChunk(
                        &chunk_hdr,
                        block_offset,
                        f->pos(),
                        sparse_hdr.blk_sz);
            if(!chk) return false;
            if(chk->is_fill())
            {
                unsigned int val ;
                f->read((char*)&val, 1);
                chk->set_fill_val(val);
            }

            m_chk_lst.append(chk);
            block_offset    += chk->block_count();
        }
        else
        {
            m_b_crc32 = true;
        }

        read_offset     += chunk_hdr.total_sz;

        if(!f->seek(read_offset))
        {
            return false;
        }
    }

    if(block_offset != sparse_hdr.total_blks)
    {
        return false;
    }
    else
        return true;
}

bool CAndroidImage::load_samsung_chunk(QFile *f)
{
    INFO("is samsung ");
    //    if(!empty())
    //        return true;

    samsung_sparse_header_t sparse_hdr;
    samsung_chunk_header_t chunk_hdr;

    int spr_hdr_len = SAMSUNG_SPARSE_HEADER_LEN;
    int chk_hdr_len = SAMSUNG_CHUNK_HEADER_LEN;
    //f->seek(0);
    int read_len = f->read((char*)&sparse_hdr, spr_hdr_len);
    if(read_len != spr_hdr_len)
    {
        return false;
    }

    if(sparse_hdr.magic != SPARSE_HEADER_MAGIC)
    {
        return true;
    }

    m_sparse_hdr.type = tImgType_sparse_samsung;
    memcpy(&m_sparse_hdr.s_hdr, &sparse_hdr, sizeof(sparse_hdr));
    if (sparse_hdr.file_hdr_sz > spr_hdr_len) {
        ////如果读到的头大小与实际大小不符，则跳到实际位置，即设置到sparse_hdr.file_hdr_sz处

        int pad_len = sparse_hdr.file_hdr_sz - spr_hdr_len;
        m_sparse_hdr.pading_len = pad_len;
        qint64 pos = f->pos() + (sparse_hdr.file_hdr_sz - spr_hdr_len);
        if(!f->seek(pos))
        {
            return false;
        }
    }

    int block_offset = 0;
    qint64 read_offset = f->pos();
    for (int i = 0; i < sparse_hdr.total_chunks; i++) {

        read_len = f->read((char*)&chunk_hdr, sizeof(chunk_hdr));
        if(read_len != chk_hdr_len){

            return false;
        }

        if (sparse_hdr.chunk_hdr_sz > chk_hdr_len) {
            /* Skip the remaining bytes in a header that is longer than
             * we expected.
             */
            qint64 pos = f->pos() + (sparse_hdr.chunk_hdr_sz - chk_hdr_len);
            f->seek(pos);
        }

        if(chunk_hdr.chunk_type != CHUNK_TYPE_CRC32)
        {
            CChunk *chk = new CChunk(
                        &chunk_hdr,
                        block_offset,
                        f->pos(),
                        sparse_hdr.blk_sz);
            if(!chk) return false;
            if(chk->is_fill())
            {
                unsigned int val ;
                f->read((char*)&val, 1);
                chk->set_fill_val(val);
            }

            m_chk_lst.append(chk);
            block_offset    += chk->block_count();
        }
        else
        {
            m_b_crc32 = true;
        }

        read_offset     += chunk_hdr.total_sz;

        if(!f->seek(read_offset))
        {
            return false;
        }
    }

    if(block_offset != sparse_hdr.total_blks)
    {
        return false;
    }
    else
        return true;
}

bool CAndroidImage::load_marvell_chunk(QFile *f)
{
    INFO("is marvell ");
    marvell_sparse_header_t sparse_hdr;
    marvell_chunk_header_t chunk_hdr;

    int spr_hdr_len = MARVELL_SPARSE_HEADER_LEN;
    int chk_hdr_len = MARVELL_CHUNK_HEADER_LEN;

    //f->seek(0);
    int read_len = f->read((char*)&sparse_hdr, spr_hdr_len);
    if(read_len != spr_hdr_len)
    {
        return false;
    }

    if(sparse_hdr.magic != SPARSE_HEADER_MAGIC)
    {
        return true;
    }

    m_sparse_hdr.type = tImgType_sparse_marvell;
    memcpy(&m_sparse_hdr.m_hdr, &sparse_hdr, sizeof(sparse_hdr));
    if (sparse_hdr.file_hdr_sz > spr_hdr_len) {
        ////如果读到的头大小与实际大小不符，则跳到实际位置，即设置到sparse_hdr.file_hdr_sz处

        int pad_len = sparse_hdr.file_hdr_sz - spr_hdr_len;
        m_sparse_hdr.pading_len = pad_len;
        qint64 pos = f->pos() + (sparse_hdr.file_hdr_sz - spr_hdr_len);

        if(!f->seek(pos))
        {
            return false;
        }
    }

    int block_offset = 0;
    qint64 read_offset = f->pos();

    for (int i = 0; i < sparse_hdr.total_chunks; i++) {

        read_len = f->read((char*)&chunk_hdr, sizeof(chunk_hdr));
        if(read_len != chk_hdr_len){

            return false;
        }

        if (sparse_hdr.chunk_hdr_sz > chk_hdr_len) {
            /* Skip the remaining bytes in a header that is longer than
             * we expected.
             */
            qint64 pos = f->pos() + (sparse_hdr.chunk_hdr_sz - chk_hdr_len);
            f->seek(pos);
        }

        if(chunk_hdr.chunk_type != CHUNK_TYPE_CRC32)
        {
            //CChunk *chk = new CChunk(&chunk_hdr, block_offset, f->pos());
            CChunk *chk = new CChunk(
                        &chunk_hdr,
                        block_offset,
                        f->pos(),
                        sparse_hdr.blk_sz);
            if(!chk) return false;
            if(chk->is_fill())
            {
                unsigned int val ;
                f->read((char*)&val, 1);
                chk->set_fill_val(val);
            }

            m_chk_lst.append(chk);
            block_offset    += chk->block_count();
        }
        else
        {
            m_b_crc32 = true;
        }

        read_offset     += chunk_hdr.total_sz;

        if(!f->seek(read_offset))
        {
            return false;
        }
    }

//    qDebug() << "sparse record blocks:" << sparse_hdr.total_blks;
//    qDebug() << "readed blocks:" << block_offset;

    if(block_offset != sparse_hdr.total_blks)
    {
        return false;
    }
    else
        return true;
}
#endif
