#include "CSparseFile.h"
#include <QFile>
#include <QString>
#include <QDebug>
#include "sparse_format.h"
#include "CChunk.h"

CSparseFile::CSparseFile(const QString &fn)
{
    m_img_fn = fn;
    m_blk_buf = NULL;
}

CSparseFile::~CSparseFile()
{
    clear();
}

void CSparseFile::clear_chunk_list()
{
    foreach(CChunk *ck, m_chk_lst)
    {
        delete ck;
    }

    m_chk_lst.clear();
}


void CSparseFile::clear()
{

    clear_chunk_list();
    m_img_fn.clear();

    free(m_blk_buf);
    m_blk_buf = NULL;
}

bool CSparseFile::load_sparse()
{
    if(!empty())
        return true;

    qint64 read_len;
    sparse_header_t sparse_hdr;
    chunk_header_t chunk_hdr;

    //QFile img(m_img_fn);

    m_fd.setFileName(m_img_fn);
    if(!m_fd.open(QIODevice::ReadOnly))
        return false;

    bool loaded  = false;
    read_len = m_fd.read((char*)&sparse_hdr, SPARSE_HEADER_LEN);
    if(read_len == SPARSE_HEADER_LEN && sparse_hdr.magic == SPARSE_HEADER_MAGIC)
    {
        m_block_sz = sparse_hdr.blk_sz;

        if(sparse_hdr.file_hdr_sz > SPARSE_HEADER_LEN)
        {
            m_fd.seek(sparse_hdr.file_hdr_sz);
            m_fd.read((char*)&chunk_hdr, sizeof(chunk_header_t));
            if ((chunk_hdr.chunk_type == CHUNK_TYPE_RAW && chunk_hdr.total_sz != (sparse_hdr.chunk_hdr_sz + (chunk_hdr.chunk_sz * sparse_hdr.blk_sz)))
                    || (chunk_hdr.chunk_type == CHUNK_TYPE_FILL && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz + sizeof(u32))
                    || (chunk_hdr.chunk_type == CHUNK_TYPE_DONT_CARE && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz))
            {
                m_sparse_type = tSparseType_Marvell;
                qDebug() << "is marvell";
                //
            }
            else{
                m_sparse_type = tSparseType_Samsung;
                qDebug() << "is samsung";
                //
            }
        }
        else
        {
            m_sparse_type = tSparseType_Normal;
            qDebug() << "is normal";
        }

        switch (m_sparse_type) {
        case tSparseType_Normal:
            loaded = load_normal(&m_fd);
            break;
        case tSparseType_Samsung:
            loaded = load_samsung(&m_fd);
            break;
        case tSparseType_Marvell:
            loaded = load_marvell(&m_fd);
            break;
        default:
            break;
        }
    }

    if(!loaded)
    {
        clear_chunk_list();

        return false;
    }

    return true;
}

void CSparseFile::create_sparse(const QString &ext_fn, const QString &sparse_fn)
{

}
