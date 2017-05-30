#include "CAndroidImage.h"
#include <QFile>

// TODO: move to a usefull cppc
bool CAndroidImage::seek_to_sparse_hdr(QFile& in)
{
#define BUF_SIZE 4096
    char buf[BUF_SIZE] = {0};
    sparse_header_t sparse_hdr;
    int offset = 0;
    qint64 read_len;
    int ok = 0;

    int limit = BUF_SIZE - sizeof(sparse_hdr);
    do
    {
        read_len = in.read(buf, BUF_SIZE);
        if(read_len >= sizeof(sparse_hdr))
        {
            for(offset = 0; offset <= limit; ++offset)
            {
                memcpy(&sparse_hdr, buf + offset, sizeof(sparse_hdr));

                if(sparse_hdr.magic == SPARSE_HEADER_MAGIC &&
                        (sparse_hdr.file_hdr_sz == 28 || sparse_hdr.file_hdr_sz == 32)
                        )
                {
                    ok = 1;
                    break;
                }
            }

            if(!ok)
            {
                offset = 1 - sizeof(sparse_hdr);
            }
            else
            {
                offset = offset - BUF_SIZE;
            }

            //cur_pos = in.pos()
            in.seek(in.pos() + offset);

            if(ok)
            {
                return true;
            }
        }
        else
        {
            break;
        }

    } while (1);

    in.seek(0);
    return false;
}
#if 0
bool CAndroidImage::cache_normal_sparse()
{
    qint64 read_len;
    normal_sparse_header_t sparse_hdr;
    normal_chunk_header_t chunk_hdr;
    int total_blocks;
    int unknown_count = 0;

    read_len = img_file_handler.read((char*)&sparse_hdr, NORMAL_SPARSE_HEADER_LEN);
    if(read_len != NORMAL_SPARSE_HEADER_LEN)
    {
        INFO("read faild");
        return false;
    }

    if(sparse_hdr.magic != SPARSE_HEADER_MAGIC)
    {
        return true;
    }

    qint64 offset;//每个chunk的起始位置

    bool_is_sparse = true;
    if (sparse_hdr.file_hdr_sz > NORMAL_SPARSE_HEADER_LEN) {
        ////如果读到的头大小与实际大小不符，则跳到实际位置，即设置到sparse_hdr.file_hdr_sz处
        qint64 pos = img_file_handler.pos() + (sparse_hdr.file_hdr_sz - NORMAL_SPARSE_HEADER_LEN);
        if(!img_file_handler.seek(pos))
        {
            return false;
        }
    }

    int_sparse_chunk_count = sparse_hdr.total_chunks;
    total_blocks = 0;
    offset = sparse_hdr.file_hdr_sz;

    sparse_chunk_array = (struct chunk_index *)calloc(sparse_hdr.total_chunks * sizeof(struct chunk_index));
    if (sparse_chunk_array == NULL) {
        return false;
    }

    for(qint64 i = 0; i < (int)sparse_hdr.total_chunks; ++i)
    {
        read_len = img_file_handler.read((char*)&chunk_hdr, NORMAL_CHUNK_HEADER_LEN);
        if(read_len != NORMAL_CHUNK_HEADER_LEN){

            RELEASE_POINTER(sparse_chunk_array);

            return false;
        }
        if (chunk_hdr.chunk_type == CHUNK_TYPE_RAW
                || chunk_hdr.chunk_type == CHUNK_TYPE_FILL
                || chunk_hdr.chunk_type == CHUNK_TYPE_DONT_CARE)
        {
            if ((chunk_hdr.chunk_type == CHUNK_TYPE_RAW && chunk_hdr.total_sz != (sparse_hdr.chunk_hdr_sz + (chunk_hdr.chunk_sz * sparse_hdr.blk_sz)))
                    || (chunk_hdr.chunk_type == CHUNK_TYPE_FILL && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz + sizeof(u32))
                    || (chunk_hdr.chunk_type == CHUNK_TYPE_DONT_CARE && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz))
            {

                RELEASE_POINTER(sparse_chunk_array);
                return false;
            }

            sparse_chunk_array[i].chunk_type  = chunk_hdr.chunk_type;

            /**
             *该chunk对应的原img中的第一个block的位置
             *
             */
            sparse_chunk_array[i].start_bytes = total_blocks * sparse_hdr.blk_sz;
            sparse_chunk_array[i].chunk_size  = chunk_hdr.chunk_sz * sparse_hdr.blk_sz;
            sparse_chunk_array[i].data_offset = offset + sparse_hdr.chunk_hdr_sz;
            ////更新当前总的block数,逻辑数据,
            total_blocks += chunk_hdr.chunk_sz;
            ////更新当前位置,在sparse内的偏移
            offset += chunk_hdr.total_sz;//
        }
        else {

            unknown_count++;

            if (unknown_count > int_sparse_chunk_count / 2) {

                RELEASE_POINTER(sparse_chunk_array);
                return false;
            }
            i--;
            offset += NORMAL_CHUNK_HEADER_LEN;
            continue;
        }

        if(!img_file_handler.seek(offset))
        {
            RELEASE_POINTER(sparse_chunk_array);
            return false;
        }
    }

    return true;
}

bool CAndroidImage::cache_marvell_sparse()
{
    qint64 read_len;
    marvell_sparse_header_t sparse_hdr;
    marvell_chunk_header_t chunk_hdr;
    int total_blocks;
    int unknown_count = 0;

    read_len = img_file_handler.read((char*)&sparse_hdr,
                                     MARVELL_SPARSE_HEADER_LEN);
    if(read_len != MARVELL_SPARSE_HEADER_LEN)
    {
        INFO("read faild");
        return false;
    }

    if(sparse_hdr.magic != SPARSE_HEADER_MAGIC)
    {
        return true;
    }

    qint64 offset;//每个chunk的起始位置

    bool_is_sparse = true;
    if (sparse_hdr.file_hdr_sz > MARVELL_SPARSE_HEADER_LEN) {
        ////如果读到的头大小与实际大小不符，则跳到实际位置，即设置到sparse_hdr.file_hdr_sz处
        qint64 pos = img_file_handler.pos() + (sparse_hdr.file_hdr_sz - MARVELL_SPARSE_HEADER_LEN);
        if(!img_file_handler.seek(pos))
        {
            return false;
        }
    }

    int_sparse_chunk_count = sparse_hdr.total_chunks;
    total_blocks = 0;
    offset = sparse_hdr.file_hdr_sz;

    sparse_chunk_array = (struct chunk_index *)calloc(sparse_hdr.total_chunks * sizeof(struct chunk_index));
    if (sparse_chunk_array == NULL) {
        return false;
    }

    for(qint64 i = 0; i < (int)sparse_hdr.total_chunks; ++i)
    {
        read_len = img_file_handler.read((char*)&chunk_hdr, MARVELL_CHUNK_HEADER_LEN);
        if(read_len != MARVELL_CHUNK_HEADER_LEN){

            RELEASE_POINTER(sparse_chunk_array);

            return false;
        }
        if (chunk_hdr.chunk_type == CHUNK_TYPE_RAW
                || chunk_hdr.chunk_type == CHUNK_TYPE_FILL
                || chunk_hdr.chunk_type == CHUNK_TYPE_DONT_CARE)
        {
            if ((chunk_hdr.chunk_type == CHUNK_TYPE_RAW && chunk_hdr.total_sz != (sparse_hdr.chunk_hdr_sz + (chunk_hdr.chunk_sz * sparse_hdr.blk_sz)))
                    || (chunk_hdr.chunk_type == CHUNK_TYPE_FILL && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz + sizeof(u32))
                    || (chunk_hdr.chunk_type == CHUNK_TYPE_DONT_CARE && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz))
            {

                RELEASE_POINTER(sparse_chunk_array);
                return false;
            }

            sparse_chunk_array[i].chunk_type  = chunk_hdr.chunk_type;

            /**
             *该chunk对应的原img中的第一个block的位置
             *
             */
            sparse_chunk_array[i].start_bytes = total_blocks * sparse_hdr.blk_sz;
            sparse_chunk_array[i].chunk_size  = chunk_hdr.chunk_sz * sparse_hdr.blk_sz;
            sparse_chunk_array[i].data_offset = offset + sparse_hdr.chunk_hdr_sz;
            ////更新当前总的block数,逻辑数据,
            total_blocks += chunk_hdr.chunk_sz;
            ////更新当前位置,在sparse内的偏移
            offset += chunk_hdr.total_sz;//
        }
        else {

            unknown_count++;

            if (unknown_count > int_sparse_chunk_count / 2) {

                RELEASE_POINTER(sparse_chunk_array);
                return false;
            }
            i--;
            offset += MARVELL_CHUNK_HEADER_LEN;
            continue;
        }

        if(!img_file_handler.seek(offset))
        {
            RELEASE_POINTER(sparse_chunk_array);
            return false;
        }
    }

    return true;
}

bool CAndroidImage::cache_samsung_sparse()
{
    qint64 read_len;
    samsung_sparse_header_t sparse_hdr;
    samsung_chunk_header_t chunk_hdr;
    int total_blocks;
    int unknown_count = 0;

    read_len = img_file_handler.read((char*)&sparse_hdr, SAMSUNG_SPARSE_HEADER_LEN);
    if(read_len != SAMSUNG_SPARSE_HEADER_LEN)
    {
        INFO("read faild");
        return false;
    }

    if(sparse_hdr.magic != SPARSE_HEADER_MAGIC)
    {
        return true;
    }

    qint64 offset;//每个chunk的起始位置

    bool_is_sparse = true;
    if (sparse_hdr.file_hdr_sz >  SAMSUNG_SPARSE_HEADER_LEN) {
        ////如果读到的头大小与实际大小不符，则跳到实际位置，即设置到sparse_hdr.file_hdr_sz处
        qint64 pos = img_file_handler.pos() + (sparse_hdr.file_hdr_sz - SAMSUNG_SPARSE_HEADER_LEN);
        if(!img_file_handler.seek(pos))
        {
            return false;
        }
    }

    int_sparse_chunk_count = sparse_hdr.total_chunks;
    total_blocks = 0;
    offset = sparse_hdr.file_hdr_sz;

    sparse_chunk_array = (struct chunk_index *)calloc(sparse_hdr.total_chunks * sizeof(struct chunk_index));
    if (sparse_chunk_array == NULL) {
        return false;
    }

    for(qint64 i = 0; i < (int)sparse_hdr.total_chunks; ++i)
    {
        read_len = img_file_handler.read((char*)&chunk_hdr, SAMSUNG_CHUNK_HEADER_LEN);
        if(read_len != SAMSUNG_CHUNK_HEADER_LEN){

            RELEASE_POINTER(sparse_chunk_array);

            return false;
        }
        if (chunk_hdr.chunk_type == CHUNK_TYPE_RAW
                || chunk_hdr.chunk_type == CHUNK_TYPE_FILL
                || chunk_hdr.chunk_type == CHUNK_TYPE_DONT_CARE)
        {
            if ((chunk_hdr.chunk_type == CHUNK_TYPE_RAW && chunk_hdr.total_sz != (sparse_hdr.chunk_hdr_sz + (chunk_hdr.chunk_sz * sparse_hdr.blk_sz)))
                    || (chunk_hdr.chunk_type == CHUNK_TYPE_FILL && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz + sizeof(u32))
                    || (chunk_hdr.chunk_type == CHUNK_TYPE_DONT_CARE && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz))
            {

                RELEASE_POINTER(sparse_chunk_array);
                return false;
            }

            sparse_chunk_array[i].chunk_type  = chunk_hdr.chunk_type;

            /**
             *该chunk对应的原img中的第一个block的位置
             *
             */
            sparse_chunk_array[i].start_bytes = total_blocks * sparse_hdr.blk_sz;
            sparse_chunk_array[i].chunk_size  = chunk_hdr.chunk_sz * sparse_hdr.blk_sz;
            sparse_chunk_array[i].data_offset = offset + sparse_hdr.chunk_hdr_sz;
            ////更新当前总的block数,逻辑数据,
            total_blocks += chunk_hdr.chunk_sz;
            ////更新当前位置,在sparse内的偏移
            offset += chunk_hdr.total_sz;//
        }
        else {

            unknown_count++;

            if (unknown_count > int_sparse_chunk_count / 2) {

                RELEASE_POINTER(sparse_chunk_array);
                return false;
            }
            i--;
            offset += SAMSUNG_CHUNK_HEADER_LEN;
            continue;
        }

        if(!img_file_handler.seek(offset))
        {
            RELEASE_POINTER(sparse_chunk_array);
            return false;
        }
    }

    return true;
}

bool CAndroidImage::cache_signed_sparse()
{
    qint64 read_len;
    normal_sparse_header_t sparse_hdr;
    normal_chunk_header_t chunk_hdr;
    int total_blocks;

    read_len = img_file_handler.read((char*)&sparse_hdr, NORMAL_SPARSE_HEADER_LEN);
    if(read_len != NORMAL_SPARSE_HEADER_LEN)
    {
        return false;
    }

    if(sparse_hdr.magic != SPARSE_HEADER_MAGIC)
    {
        return true;
    }

    qint64 offset;//每个chunk的起始位置

    bool_is_sparse = true;
    if (sparse_hdr.file_hdr_sz > NORMAL_SPARSE_HEADER_LEN) {
        ////如果读到的头大小与实际大小不符，则跳到实际位置，即设置到sparse_hdr.file_hdr_sz处
        qint64 pos = img_file_handler.pos() + (sparse_hdr.file_hdr_sz - NORMAL_SPARSE_HEADER_LEN);
        if(!img_file_handler.seek(pos))
        {
            return false;
        }
    }

    int_sparse_chunk_count = sparse_hdr.total_chunks;
    total_blocks = 0;
    offset = sparse_hdr.file_hdr_sz;

    sparse_chunk_array = (struct chunk_index *)calloc(sparse_hdr.total_chunks * sizeof(struct chunk_index));
    if (sparse_chunk_array == NULL) {
        return false;
    }

    for(qint64 i = 0; i < (int)sparse_hdr.total_chunks; ++i)
    {
        if(seek_to_chunk_hdr(img_file_handler, sparse_hdr.chunk_hdr_sz, sparse_hdr.blk_sz))
        {
            offset = img_file_handler.pos();


            sparse_chunk_array[i].data_offset = offset + sparse_hdr.chunk_hdr_sz;
            //
            img_file_handler.read((char*)&chunk_hdr, CHUNK_HEADER_LEN);
            sparse_chunk_array[i].chunk_type = chunk_hdr.chunk_type;
            sparse_chunk_array[i].start_bytes = total_blocks * sparse_hdr.blk_sz;
            sparse_chunk_array[i].chunk_size = chunk_hdr.chunk_sz * sparse_hdr.blk_sz;
            total_blocks += chunk_hdr.chunk_sz;

            img_file_handler.seek(offset + chunk_hdr.total_sz);

        }
    }

    return true;
}



bool CAndroidImage::seek_to_chunk_hdr(QFile& in, int chk_hdr_sz, int chk_blk_sz)
{
#define BUF_SIZE 4096
    char buf[BUF_SIZE] = {0};
    chunk_header_t chunk_hdr;
    int offset = 0;
    qint64 read_len;
    int ok = 0;

    int limit = BUF_SIZE - sizeof(chunk_hdr);
    do
    {
        memset(buf, 0, BUF_SIZE);
        read_len = in.read(buf, BUF_SIZE);
        if(read_len >= sizeof(chunk_hdr))
        {
            ok = 0;
            for(offset = 0; offset <= limit; ++offset)
            {
                memcpy(&chunk_hdr, buf + offset, sizeof(chunk_hdr));

                if ((chunk_hdr.chunk_type == CHUNK_TYPE_RAW && chunk_hdr.total_sz == (chk_hdr_sz + (chunk_hdr.chunk_sz * chk_blk_sz)))
                        || (chunk_hdr.chunk_type == CHUNK_TYPE_FILL && chunk_hdr.total_sz == chk_hdr_sz+ sizeof(u32))
                        || (chunk_hdr.chunk_type == CHUNK_TYPE_DONT_CARE && chunk_hdr.total_sz == chk_hdr_sz))

                {
                    ok = 1;
                    break;
                }
            }

            if(!ok)
            {
                offset = 1 - sizeof(chunk_hdr);
            }
            else
            {
                offset = offset - BUF_SIZE;
            }

            in.seek(in.pos() + offset);
            if(ok)
            {
                return true;
            }
        }
        else
        {
            break;
        }
    } while (1);

    return false;
}

qint64 CAndroidImage::read_chunk_data(char *ptr, tBlockNum offset_blocks, int n_blocks, int block_size)
{
    int low, mid, high;
    qint64 offset, file_off;

    qint64 len;
    qint64 read_len;
    int idx = 0;

    chunk_index *cur_chunk = NULL;
    if (ptr == NULL)
        return -1;

    offset = offset_blocks * block_size;//chunk 位置
    len = n_blocks * block_size;
    ////查找并读取
    low = 0;
    high = int_sparse_chunk_count;
    ////LOG(QString("sparse chunk count: %1").arg(high));
    while (low <= high) {
        mid = (low + high) / 2;
        cur_chunk = &sparse_chunk_array[mid];
        ////offset是否位于该chunk之中
        if (offset >= cur_chunk->start_bytes
                &&
                offset < (cur_chunk->start_bytes + cur_chunk->chunk_size)
                )
        {
            ////在第一个找到的chunk中的相对位置
            u64 rel_offset = offset - cur_chunk->start_bytes;
            ////在当前chunk中读取的数据长度
            u32 read_in_chunk;
            ////根据chunk类型，读取镜像数据或者设置数据为0
            while (len > 0) {
                u32 fill_data;
                int i, fill_count;
                ////读取chunk_size与len中较小的
                cur_chunk = &sparse_chunk_array[mid + idx];

                read_in_chunk = cur_chunk->chunk_size;

                if (len < read_in_chunk)
                    read_in_chunk = len;

                switch (cur_chunk->chunk_type) {
                case CHUNK_TYPE_RAW:
                {
                    file_off = (qint64)(cur_chunk->data_offset + rel_offset);
                    if(!img_file_handler.seek(file_off))
                    {
                        INFO("Raw Chunk Seek Failed ");
                        return -1;
                    }
                    read_len = img_file_handler.read(ptr, read_in_chunk);
                    if(read_len != read_in_chunk)
                    {
                        INFO("Read Raw Failed.");
                        return -1;
                    }
                }
                    break;
                case CHUNK_TYPE_FILL:
                {
                    //打包时的填充代码只填充同1个
                    //write_chunk_fill(struct output_file *out, u64 off, u32 fill_val, int len)
                    /*{
                      *    ...
                            for (i = 0; i < (s_block_size / sizeof(u32)); i++) {
                                  fill_buf[i] = fill_val;
                            }
                           ...
                       }
                     */
                    file_off = (qint64)(cur_chunk->data_offset + rel_offset);
                    if(!img_file_handler.seek(file_off))
                        return -1;

                    read_len = img_file_handler.read((char*)&fill_data, sizeof(fill_data));
                    if(read_len != sizeof(fill_data))
                        return -1;

                    fill_count = read_in_chunk / sizeof(fill_data);

                    for (i = 0; i < fill_count; i++)
                        ((u32 *)ptr)[i] = fill_data;

                    break;
                }
                case CHUNK_TYPE_DONT_CARE:
                    //chunk中不包含有效数据，直接置零
                    memset(ptr, 0, read_in_chunk);
                    break;
                default:
                    break;
                }
                //更新指针
                ptr += read_in_chunk;
                //更新剩余长度
                len -= read_in_chunk;
                //如果未读完则读取下一个chunk
                idx++;
                rel_offset = 0;
            }

            return  n_blocks * block_size;
        }
        else if (sparse_chunk_array[mid].start_bytes > offset)
            high = mid - 1;
        else
            low = mid + 1;
    }

    return -1;
}

#endif
