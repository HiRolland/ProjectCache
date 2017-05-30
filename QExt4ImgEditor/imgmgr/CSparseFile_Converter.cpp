#include "CSparseFile.h"
#include <QFile>
#include "sparse_format.h"
#include "CChunk.h"

#include <QString>
#include <QDebug>
bool CSparseFile::load_normal(QFile *f)
{
    if(!empty())
        return true;

    normal_sparse_header_t sparse_hdr;
    normal_chunk_header_t chunk_hdr;

    int spr_hdr_len = NORMAL_SPARSE_HEADER_LEN;
    int chk_hdr_len = NORMAL_CHUNK_HEADER_LEN;
    f->seek(0);
    int read_len = f->read((char*)&sparse_hdr, spr_hdr_len);
    if(read_len != spr_hdr_len)
    {

        return false;
    }

    if(sparse_hdr.magic != SPARSE_HEADER_MAGIC)
    {
        return true;
    }

    if (sparse_hdr.file_hdr_sz > spr_hdr_len) {
        ////如果读到的头大小与实际大小不符，则跳到实际位置，即设置到sparse_hdr.file_hdr_sz处
        qint64 pos = f->pos() + (sparse_hdr.file_hdr_sz - spr_hdr_len);
        if(!f->seek(pos))
        {
            return false;
        }
    }

    int mapped_block_offset = 0;
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
            CChunk *chk = new CChunk(&chunk_hdr, mapped_block_offset, f->pos());
            if(!chk) return false;
            if(chk->is_fill())
            {
                unsigned int val ;
                f->read((char*)&val, 1);
                chk->set_fill_val(val);
            }

            m_chk_lst.append(chk);
            mapped_block_offset    += chk->block_count();
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

    qDebug() << "sparse record blocks:" << sparse_hdr.total_blks;
    qDebug() << "readed blocks:" << mapped_block_offset;

    if(mapped_block_offset != sparse_hdr.total_blks)
    {
        return false;
    }
    else
        return true;
}

bool CSparseFile::load_samsung(QFile *f)
{
    if(!empty())
        return true;

    samsung_sparse_header_t sparse_hdr;
    samsung_chunk_header_t chunk_hdr;

    int spr_hdr_len = SAMSUNG_SPARSE_HEADER_LEN;
    int chk_hdr_len = SAMSUNG_CHUNK_HEADER_LEN;
    f->seek(0);
    int read_len = f->read((char*)&sparse_hdr, spr_hdr_len);
    if(read_len != spr_hdr_len)
    {
        return false;
    }

    if(sparse_hdr.magic != SPARSE_HEADER_MAGIC)
    {
        return true;
    }

    if (sparse_hdr.file_hdr_sz > spr_hdr_len) {
        ////如果读到的头大小与实际大小不符，则跳到实际位置，即设置到sparse_hdr.file_hdr_sz处
        qint64 pos = f->pos() + (sparse_hdr.file_hdr_sz - spr_hdr_len);
        if(!f->seek(pos))
        {
            return false;
        }
    }

    int mapped_block_offset = 0;
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
            CChunk *chk = new CChunk(&chunk_hdr, mapped_block_offset, f->pos());
            if(!chk) return false;
            if(chk->is_fill())
            {
                unsigned int val ;
                f->read((char*)&val, 1);
                chk->set_fill_val(val);
            }

            m_chk_lst.append(chk);
            mapped_block_offset    += chk->block_count();
        }
        else
        {
            m_b_crc32 = true;
        }

        read_offset     += chunk_hdr.total_sz;

        if(!f->seek(read_offset))
        {
            qDebug() << "seek false" << read_offset;
            return false;
        }
    }

    qDebug() << "sparse record blocks:" << sparse_hdr.total_blks;
    qDebug() << "readed blocks:" << mapped_block_offset;
    if(mapped_block_offset != sparse_hdr.total_blks)
    {
        return false;
    }
    else
        return true;
}

bool CSparseFile::load_marvell(QFile *f)
{
    marvell_sparse_header_t sparse_hdr;
    marvell_chunk_header_t chunk_hdr;

    int spr_hdr_len = MARVELL_SPARSE_HEADER_LEN;
    int chk_hdr_len = MARVELL_CHUNK_HEADER_LEN;

    f->seek(0);
    int read_len = f->read((char*)&sparse_hdr, spr_hdr_len);
    if(read_len != spr_hdr_len)
    {
        return false;
    }

    if(sparse_hdr.magic != SPARSE_HEADER_MAGIC)
    {
        return true;
    }

    if (sparse_hdr.file_hdr_sz > spr_hdr_len) {
        ////如果读到的头大小与实际大小不符，则跳到实际位置，即设置到sparse_hdr.file_hdr_sz处
        qint64 pos = f->pos() + (sparse_hdr.file_hdr_sz - spr_hdr_len);
        if(!f->seek(pos))
        {
            return false;
        }
    }

    int mapped_block_offset = 0;
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
            CChunk *chk = new CChunk(&chunk_hdr, mapped_block_offset, f->pos());
            if(!chk) return false;
            if(chk->is_fill())
            {
                unsigned int val ;
                f->read((char*)&val, 1);
                chk->set_fill_val(val);
            }

            m_chk_lst.append(chk);
            mapped_block_offset    += chk->block_count();
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

    qDebug() << "sparse record blocks:" << sparse_hdr.total_blks;
    qDebug() << "readed blocks:" << mapped_block_offset;

    if(mapped_block_offset != sparse_hdr.total_blks)
    {
        return false;
    }
    else
        return true;
}

void CSparseFile::dump_raw_chunk(CChunk *chk, QFile *in_sparse, QFile *out_ext4)
{
    int blks = chk->block_count();
    if(!in_sparse->seek(chk->sparse_data_offset()))
        return;

    while (blks > 0) {
        char * buf = block_buffer();
        in_sparse->read(buf, block_size());
        out_ext4->write(buf, block_size());
        blks --;
    }

}

void CSparseFile::dump_fill_chunk(CChunk *chk, QFile *in_sparse, QFile *out_ext4)
{
    int blks = chk->block_count();
    if (blks > 0)
    {
        char * buf = block_buffer(chk->fill_val());

        while(blks > 0)
        {
            out_ext4->write(buf, block_size());

            blks --;
        }
    }
}

void CSparseFile::dump_skip_chunk(CChunk *chk, QFile *in_sparse, QFile *out_ext4)
{
    //do nothing
    int blks = chk->block_count();
    if (blks > 0)
    {
        char * buf = block_buffer();

        while(blks > 0)
        {
            out_ext4->write(buf, block_size());

            blks --;
        }
    }
    return;
}

void CSparseFile::create_raw_chunk(CChunk *chk, QFile *in_ext4, QFile *out_sparse)
{
    char *buf = block_buffer();
    int start_block = chk->start_block();
    int blk_num = chk->block_count();

    switch (m_sparse_type) {
    case tSparseType_Normal:
    {
        normal_chunk_header_t chunk_header;
        /* Finally we can safely emit a chunk of data */
        chunk_header.chunk_type = CHUNK_TYPE_RAW;
        chunk_header.reserved1 = 0;
        chunk_header.chunk_sz = chk->block_count();
        chunk_header.total_sz = sizeof(chunk_header) + chk->block_count() * block_size();

        out_sparse->write((char*)&chunk_header, sizeof(chunk_header));
    }
        break;
    case tSparseType_Samsung:
    {
        samsung_chunk_header_t chunk_header;
        /* Finally we can safely emit a chunk of data */
        chunk_header.chunk_type = CHUNK_TYPE_RAW;
        chunk_header.reserved1 = 0;
        chunk_header.chunk_sz = chk->block_count();
        chunk_header.total_sz = sizeof(chunk_header) + chk->block_count() * block_size();

        out_sparse->write((char*)&chunk_header, sizeof(chunk_header));
    }
        break;
    case tSparseType_Marvell:
    {
        marvell_chunk_header_t chunk_header;
        /* Finally we can safely emit a chunk of data */
        chunk_header.chunk_type = CHUNK_TYPE_RAW;
        chunk_header.reserved1 = 0;
        chunk_header.chunk_sz = chk->block_count();
        chunk_header.total_sz = sizeof(chunk_header) + chk->block_count() * block_size();

        out_sparse->write((char*)&chunk_header, sizeof(chunk_header));
    }
        break;
    default:
        break;
    }


    qint64 pos;
    qint64 cur_block;
    for(int idx = 0; idx < blk_num; ++idx)
    {
        cur_block = start_block + idx;
        pos = cur_block * block_size();

        if(in_ext4->seek(pos))
        {
            in_ext4->read(buf, block_size());
            out_sparse->write(buf, block_size());
        }
    }

}

void CSparseFile::create_fill_chunk(CChunk *chk, QFile *in_ext4, QFile *out_sparse)
{

    switch (m_sparse_type) {
    case tSparseType_Normal:
    {
        normal_chunk_header_t chunk_header;
        /* Finally we can safely emit a chunk of data */
        chunk_header.chunk_type = CHUNK_TYPE_FILL;
        chunk_header.reserved1 = 0;
        chunk_header.chunk_sz = chk->block_count();
        chunk_header.total_sz = sizeof(chunk_header) + chk->block_count() * block_size();

        out_sparse->write((char*)&chunk_header, sizeof(chunk_header));
    }
        break;
    case tSparseType_Samsung:
    {
        samsung_chunk_header_t chunk_header;
        /* Finally we can safely emit a chunk of data */
        chunk_header.chunk_type = CHUNK_TYPE_RAW;
        chunk_header.reserved1 = 0;
        chunk_header.chunk_sz = chk->block_count();
        chunk_header.total_sz = sizeof(chunk_header) + chk->block_count() * block_size();

        out_sparse->write((char*)&chunk_header, sizeof(chunk_header));
    }
        break;
    case tSparseType_Marvell:
    {
        marvell_chunk_header_t chunk_header;
        /* Finally we can safely emit a chunk of data */
        chunk_header.chunk_type = CHUNK_TYPE_RAW;
        chunk_header.reserved1 = 0;
        chunk_header.chunk_sz = chk->block_count();
        chunk_header.total_sz = sizeof(chunk_header) + chk->block_count() * block_size();

        out_sparse->write((char*)&chunk_header, sizeof(chunk_header));
    }
        break;
    default:
        break;
    }
}

void CSparseFile::create_skip_chunk(CChunk *chk, QFile *in_ext4, QFile *out_sparse)
{

    switch (m_sparse_type) {
    case tSparseType_Normal:
    {
        normal_chunk_header_t chunk_header;
        /* Finally we can safely emit a chunk of data */
        chunk_header.chunk_type = CHUNK_TYPE_RAW;
        chunk_header.reserved1 = 0;
        chunk_header.chunk_sz = chk->block_count();
        chunk_header.total_sz = sizeof(chunk_header) + chk->block_count() * block_size();

        out_sparse->write((char*)&chunk_header, sizeof(chunk_header));
    }
        break;
    case tSparseType_Samsung:
    {
        samsung_chunk_header_t chunk_header;
        /* Finally we can safely emit a chunk of data */
        chunk_header.chunk_type = CHUNK_TYPE_RAW;
        chunk_header.reserved1 = 0;
        chunk_header.chunk_sz = chk->block_count();
        chunk_header.total_sz = sizeof(chunk_header) + chk->block_count() * block_size();

        out_sparse->write((char*)&chunk_header, sizeof(chunk_header));
    }
        break;
    case tSparseType_Marvell:
    {
        marvell_chunk_header_t chunk_header;
        /* Finally we can safely emit a chunk of data */
        chunk_header.chunk_type = CHUNK_TYPE_RAW;
        chunk_header.reserved1 = 0;
        chunk_header.chunk_sz = chk->block_count();
        chunk_header.total_sz = sizeof(chunk_header) + chk->block_count() * block_size();

        out_sparse->write((char*)&chunk_header, sizeof(chunk_header));
    }
        break;
    default:
        break;
    }
}

char *CSparseFile::block_buffer(unsigned int init)
{
    if(!m_blk_buf)
    {
        m_blk_buf = (char*)malloc(block_size());
    }

    memset(m_blk_buf, init, block_size());

    return m_blk_buf;
}



void CSparseFile::convert_to_ext4(const QString &out_ext_fn)
{
    if(empty())
        return;

    QFile ext4_out(out_ext_fn);
    //QFile sparse_in(m_img_fn);

    if(!ext4_out.open(QIODevice::WriteOnly))
        return;

    m_fd.seek(0);
    foreach(CChunk *chk, m_chk_lst)
    {

        if(chk->is_raw())
        {
            dump_raw_chunk(chk, &m_fd, &ext4_out);
        }
        else if(chk->is_fill())
        {
            dump_fill_chunk(chk, &m_fd, &ext4_out);
        }
        else if(chk->is_skip())
        {
            dump_skip_chunk(chk, &m_fd, &ext4_out);
        }
    }

    ext4_out.close();
}

void CSparseFile::create_sparse_on_ext4(const QString &in_ext4_fn, const QString &out_sparse_fn)
{
    if(empty())
        return;

    QFile ext4(in_ext4_fn);
    QFile sparse(out_sparse_fn);

    if(!ext4.open(QIODevice::ReadOnly))
        return;

    if(!sparse.open(QIODevice::WriteOnly))
        return;

    switch(m_sparse_type)
    {
    case tSparseType_Normal:
        break;
    case tSparseType_Samsung:
        break;
    case tSparseType_Marvell:
        break;
    }

    foreach(CChunk *chk, m_chk_lst)
    {

        if(chk->is_raw())
        {
            create_raw_chunk(chk, &ext4, &sparse);
        }
        else if(chk->is_fill())
        {
            dump_fill_chunk(chk, &ext4, &sparse);
        }
        else if(chk->is_skip())
        {
            dump_skip_chunk(chk, &ext4, &sparse);
        }
    }

    ext4.close();
    sparse.close();
}

