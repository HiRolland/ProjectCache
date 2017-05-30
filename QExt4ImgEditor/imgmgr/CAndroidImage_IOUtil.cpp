#include "CAndroidImage.h"

qint64 CAndroidImage::read_disk(char *ptr, tBlockNum offset_blocks, int n_blocks, int block_size)
{
    qint64 offset, rd, len;
    if(offset_blocks < 0)
        return -1;

    if(bool_is_sparse) {
        return read_chunk_data(ptr, offset_blocks, n_blocks, block_size);
    } else {
        offset = (qint64)(offset_blocks * block_size);
        if(!img_file_handler.seek(offset))
        {
            return -1;
        }

        len = n_blocks * block_size;
        rd = img_file_handler.read(ptr, len);
        if(len != rd)
        {
            return -1;
        }
    }

    return rd;
}

bool CAndroidImage::write_disk(char *data, qint64 data_len, tBlockNum offset_blocks, int block_size)
{

    if(bool_is_sparse)
        return false;
    else
    {
        qint64 offset, wr;
        offset = (qint64)(offset_blocks * block_size);
        if(!img_file_handler.seek(offset))
        {
            return false;
        }

        wr = img_file_handler.write(data, data_len);

        if(wr != data_len)
        {
            return false;
        }

        return true;
    }
}

bool CAndroidImage::write_disk(char *data, int data_len, tBlockNum offset_blocks, int offset_on_block, int block_size)
{
    if(!data)
        return true; // yep,return true

    if(bool_is_sparse)
        return false;
    else
    {
        qint64 offset, wr;
        offset = (qint64)(offset_blocks * block_size + offset_on_block);
        if(!img_file_handler.seek(offset))
        {
            return false;
        }

        wr = img_file_handler.write(data, data_len);

        if(wr != data_len)
        {
            return false;
        }

        return true;
    }
}

bool CAndroidImage::clear_disk(tBlockNum offset_blocks, int n_blocks, int block_size)
{
    if(!n_blocks )
        return false;

    if(bool_is_sparse)
        return false;
    else
    {
        qint64 offset;
        offset = (qint64)(offset_blocks * block_size);
        if(!img_file_handler.seek(offset))
        {
            return false;
        }

        char* data = (char*) calloc(1, block_size);

        do{
            img_file_handler.write((char*)data, block_size);
        }while(--n_blocks > 0);

        return true;
    }
}

