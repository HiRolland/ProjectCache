#include "CImageManager.h"
#include <QFile>
#include "CChunk.h"

/*
#define WRITE_MODIFIED_CHUNK_BODY(news, orgs, chk, mod_lst, chunk_type_t) \
    do {\
        u32 chunk_cnt = 0; \
        tBlockNum chk_start, chk_last, mdb_start, mdb_last; \
        chk_start = chk->start_block(); \
        chk_last = chk->last_block(); \
        mdb_start = mod_lst.first()->blk_num; \
        mdb_last = mod_lst.last()->blk_num; \
        char *blk_buf = NULL; \
        qint64 f_off = 0; \
        chunk_type_t chunk; \
        chunk.reserved1 = 0; \
        if(chk_start < mdb_start){ \
            chunk.chunk_sz = mdb_start - chk_start; \
            if(chk->is_raw()){\
                chunk.chunk_type = CHUNK_TYPE_RAW; \
                ++chunk_cnt;\
                chunk.total_sz = sizeof(chunk_type_t) + chunk.chunk_sz * block_size(); \
                news->write((char*)&chunk, sizeof(chunk_type_t)); \
                f_off = chk->sparse_data_offset(); \
                orgs->seek(f_off); \
                if(!blk_buf) blk_buf = (char*)calloc(1, block_size()); \
                for(int i = 0; i < chunk.chunk_sz; ++i){  \
                    orgs->read(blk_buf, block_size());  \
                    news->write(blk_buf, block_size());  \
                }  \
            }else if(chk->is_fill()) {  \
                chunk.chunk_type = CHUNK_TYPE_FILL;  \
                ++chunk_cnt;  \
                chunk.total_sz = sizeof(chunk_type_t) + sizeof(u32); \
                news->write((char*)&chunk, sizeof(chunk_type_t));  \
                u32 fill_val = chk->fill_val();  \
                news->write((char*)&fill_val, sizeof(fill_val));  \
            } else if(chk->is_skip()){  \
                chunk.chunk_type = CHUNK_TYPE_DONT_CARE;  \
                ++chunk_cnt;  \
                chunk.total_sz = sizeof(chunk_type_t); \
                news->write((char*)&chunk, sizeof(chunk_type_t));  \
            }  \
        }  \
        int slide_window = 0, next_pos = 0;  \
        tModifiedBlock *cur, *pre, *next, *tmp;  \
        for(int pos = 0; pos < mod_lst.size(); pos++) {  \
            cur = mod_lst.at(pos);   \
            slide_window = 0;  \
            if(cur->data != NULL){  \
                chunk.chunk_type    = CHUNK_TYPE_RAW;  \
                ++chunk_cnt;  \
                next_pos = pos + 1;  \
                pre = cur;  \
                while(next_pos < mod_lst.size()){  \
                    next = mod_lst.at(next_pos);  \
                    if(((pre->blk_num + 1)== next->blk_num) && next->data != NULL ) {  \
                        ++slide_window;  \
                        pre = next;  \
                    } else break;  \
                    ++next_pos;  \
                }  \
                chunk.chunk_sz = slide_window + 1;  \
                chunk.total_sz = sizeof(chunk_type_t) + chunk.chunk_sz * block_size();  \
                news->write((char*)&chunk, sizeof(chunk_type_t));  \
                for(int i = 0; i < chunk.chunk_sz; ++i){ \
                    tmp = mod_lst.at(pos + i); \
                    news->write((char*)tmp->data, block_size()); \
                } \
            } else{ \
                chunk.chunk_type = CHUNK_TYPE_DONT_CARE;  \
                ++chunk_cnt; \
                next_pos = pos + 1; \
                pre = cur; \
                while(next_pos < mod_lst.size()){ \
                    next = mod_lst.at(next_pos); \
                    if(((pre->blk_num + 1)== next->blk_num) && next->data == NULL ){ \
                        ++slide_window; \
                        pre = next; \
                    } else  break; \
                    ++next_pos; \
                } \
                chunk.chunk_sz = slide_window + 1; \
                chunk.total_sz = sizeof(chunk_type_t); \
                news->write((char*)&chunk, sizeof(chunk_type_t)); \
            } \
            pos += slide_window; \
            if(pos < (mod_lst.size() - 1)){ \
                pre = mod_lst.at(pos); \
                next = mod_lst.at(pos + 1);  \
                if((pre->blk_num + 1) != next->blk_num) { \
                    chunk.chunk_sz = next->blk_num - pre->blk_num - 1; \
                    if(chk->is_raw()) { \
                        chunk.chunk_type = CHUNK_TYPE_RAW; \
                        ++chunk_cnt; \
                        chunk.total_sz = sizeof(chunk_type_t) + chunk.chunk_sz * block_size(); \
                        news->write((char*)&chunk, sizeof(chunk_type_t)); \
                        f_off = chk->sparse_data_offset() + (pre->blk_num - chk_start + 1) * block_size(); \
                        orgs->seek(f_off); \
                        if(!blk_buf) blk_buf = (char*)calloc(1, block_size()); \
                        for(int i = 0; i < chunk.chunk_sz; ++i) { \
                            orgs->read(blk_buf, block_size()); \
                            news->write(blk_buf, block_size()); \
                        } \
                    } else if(chk->is_fill()) { \
                        chunk.chunk_type = CHUNK_TYPE_FILL; \
                        ++chunk_cnt; \
                        chunk.total_sz = sizeof(chunk_type_t) + sizeof(u32); \
                        news->write((char*)&chunk, sizeof(chunk_type_t)); \
                        u32 fill_val = chk->fill_val(); \
                        news->write((char*)&fill_val, sizeof(fill_val)); \
                    } else if(chk->is_skip()) { \
                        chunk.chunk_type = CHUNK_TYPE_DONT_CARE; \
                        ++chunk_cnt; \
                        chunk.total_sz = sizeof(chunk_type_t); \
                        news->write((char*)&chunk, sizeof(chunk_type_t)); \
                    } \
                } \
            } \
        } \
        if(mdb_last < chk_last)  { \
            chunk.chunk_sz = chk_last - mdb_last; \
            if(chk->is_raw()) { \
                chunk.chunk_type = CHUNK_TYPE_RAW; \
                ++chunk_cnt; \
                chunk.total_sz = sizeof(chunk_type_t) + chunk.chunk_sz * block_size(); \
                news->write((char*)&chunk, sizeof(chunk_type_t)); \
                f_off = chk->sparse_data_offset() + (mdb_last - chk_start + 1) * block_size(); \
                orgs->seek(f_off); \
                if(!blk_buf) blk_buf = (char*)calloc(1, block_size()); \
                for(int i = 0; i < chunk.chunk_sz; ++i) { \
                    orgs->read(blk_buf, block_size()); \
                    news->write(blk_buf, block_size()); \
                } \
            }  else if(chk->is_fill()) { \
                chunk.chunk_type = CHUNK_TYPE_FILL; \
                ++chunk_cnt; \
                chunk.total_sz = sizeof(chunk_type_t) + sizeof(u32); \
                news->write((char*)&chunk, sizeof(chunk_type_t)); \
                u32 fill_val = chk->fill_val(); \
                news->write((char*)&fill_val, sizeof(fill_val)); \
            } else if(chk->is_skip()) { \
                chunk.chunk_type = CHUNK_TYPE_DONT_CARE; \
                ++chunk_cnt; \
                chunk.total_sz = sizeof(chunk_type_t); \
                news->write((char*)&chunk, sizeof(chunk_type_t)); \
            } \
        } \
        if(blk_buf) free(blk_buf); \
        return chunk_cnt; }while(0)

*/

u32 CImageManager::write_modified_chunk_normal(QFile *news, QFile *orgs,  CChunk *chk, const tModBlkLst &mod_lst)
{
    u32 chunk_cnt = 0;

    tBlockNum chk_start, chk_last, mdb_start, mdb_last;
    chk_start = chk->start_block();
    chk_last = chk->last_block();

    mdb_start = mod_lst.first()->blk_num;
    mdb_last = mod_lst.last()->blk_num;

    char *blk_buf = NULL;
    qint64 f_off = 0;
    //起始
    //
    normal_chunk_header_t chunk;
    chunk.reserved1 = 0;

    if(chk_start < mdb_start)
    {
        //前面有几块没有改动,保持不变
        //
        chunk.chunk_sz = mdb_start - chk_start;

        if(chk->is_raw())
        {
            chunk.chunk_type = CHUNK_TYPE_RAW;
            ++chunk_cnt;

            chunk.total_sz = NORMAL_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();
            news->write((char*)&chunk, sizeof(normal_chunk_header_t));

            f_off = chk->sparse_data_offset();
            orgs->seek(f_off);

            if(!blk_buf) blk_buf = (char*)calloc(1, block_size());
            for(u32 i = 0; i < chunk.chunk_sz; ++i)
            {
                orgs->read(blk_buf, block_size());
                news->write(blk_buf, block_size());
            }
        }
        else if(chk->is_fill())
        {
            chunk.chunk_type = CHUNK_TYPE_FILL;
            ++chunk_cnt;

            chunk.total_sz = NORMAL_CHUNK_HEADER_LEN + sizeof(u32);
            news->write((char*)&chunk, sizeof(normal_chunk_header_t));

            u32 fill_val = chk->fill_val();
            news->write((char*)&fill_val, sizeof(fill_val));
        }
        else if(chk->is_skip())
        {
            chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
            ++chunk_cnt;
            chunk.total_sz = NORMAL_CHUNK_HEADER_LEN;

            news->write((char*)&chunk, sizeof(normal_chunk_header_t));
        }
    }

    //中间
    //

    int slide_window = 0, next_pos = 0;

    tModifiedBlock *cur, *pre, *next, *tmp;
    for(int pos = 0; pos < mod_lst.size(); pos++)
    {
        cur = mod_lst.at(pos);
        slide_window = 0;

        if(cur->data != NULL)
        {
            chunk.chunk_type    = CHUNK_TYPE_RAW;
            ++chunk_cnt;

            //移动窗口,探测连续的块
            next_pos = pos + 1;
            pre = cur;
            while(next_pos < mod_lst.size())
            {
                next = mod_lst.at(next_pos);

                if(((pre->blk_num + 1)== next->blk_num) && next->data != NULL )
                {
                    ++slide_window;
                    pre = next;
                }
                else
                {
                    break;
                }

                ++next_pos;
            }

            chunk.chunk_sz = slide_window + 1;
            chunk.total_sz = NORMAL_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();

            news->write((char*)&chunk, sizeof(normal_chunk_header_t));

            for(u32 i = 0; i < chunk.chunk_sz; ++i)
            {
                tmp = mod_lst.at(pos + i);
                news->write((char*)tmp->data, block_size());
            }
        }
        else
        {
            //删除的统一设置为skip
            //
            chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
            ++chunk_cnt;

            next_pos = pos + 1;
            pre = cur;
            while(next_pos < mod_lst.size())
            {
                next = mod_lst.at(next_pos);
                if(((pre->blk_num + 1)== next->blk_num) && next->data == NULL )
                {
                    ++slide_window;
                    pre = next;
                }
                else
                {
                    break;
                }

                ++next_pos;
            }

            chunk.chunk_sz = slide_window + 1;
            chunk.total_sz = NORMAL_CHUNK_HEADER_LEN;

            news->write((char*)&chunk, sizeof(normal_chunk_header_t));
        }
        //根据滑动窗口大小调整位置
        //
        pos += slide_window;
        //pos指向当前已经处理了的最后一个

        // 处理空窗
        // 确认剩下的不止一个,若还剩一个则不存在空窗
        //
        if(pos < (mod_lst.size() - 1))
        {
            pre = mod_lst.at(pos);// 当前已经被处理的最后一个块
            next = mod_lst.at(pos + 1);

            if((pre->blk_num + 1) < next->blk_num)
            {
                //存在空窗
                chunk.chunk_sz = next->blk_num - pre->blk_num - 1;

                if(chk->is_raw())
                {
                    chunk.chunk_type = CHUNK_TYPE_RAW;
                    ++chunk_cnt;

                    chunk.total_sz = NORMAL_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();
                    news->write((char*)&chunk, sizeof(normal_chunk_header_t));

                    f_off = chk->sparse_data_offset() + (pre->blk_num - chk_start + 1) * block_size();
                    orgs->seek(f_off);

                    if(!blk_buf) blk_buf = (char*)calloc(1, block_size());
                    for(int i = 0; i < chunk.chunk_sz; ++i)
                    {
                        orgs->read(blk_buf, block_size());
                        news->write(blk_buf, block_size());
                    }
                }
                else if(chk->is_fill())
                {
                    chunk.chunk_type = CHUNK_TYPE_FILL;
                    ++chunk_cnt;
                    chunk.total_sz = NORMAL_CHUNK_HEADER_LEN + sizeof(u32);

                    news->write((char*)&chunk, sizeof(normal_chunk_header_t));
                    u32 fill_val = chk->fill_val();
                    news->write((char*)&fill_val, sizeof(fill_val));
                }
                else if(chk->is_skip())
                {
                    chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
                    ++chunk_cnt;
                    chunk.total_sz = NORMAL_CHUNK_HEADER_LEN;

                    news->write((char*)&chunk, sizeof(normal_chunk_header_t));
                }
            }
        }
    }

    //最后
    //
    if(mdb_last < chk_last)
    {
        //后面有几块没有改动,保持不变
        //
        //设置块数
        //
        chunk.chunk_sz = chk_last - mdb_last;

        if(chk->is_raw())
        {
            chunk.chunk_type = CHUNK_TYPE_RAW;
            ++chunk_cnt;

            chunk.total_sz = NORMAL_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();
            news->write((char*)&chunk, sizeof(normal_chunk_header_t));

            f_off = chk->sparse_data_offset() + (mdb_last - chk_start + 1) * block_size();
            orgs->seek(f_off);

            if(!blk_buf) blk_buf = (char*)calloc(1, block_size());
            for(int i = 0; i < chunk.chunk_sz; ++i)
            {
                orgs->read(blk_buf, block_size());
                news->write(blk_buf, block_size());
            }
        }
        else if(chk->is_fill())
        {
            chunk.chunk_type = CHUNK_TYPE_FILL;
            ++chunk_cnt;

            chunk.total_sz = NORMAL_CHUNK_HEADER_LEN + sizeof(u32);
            news->write((char*)&chunk, sizeof(normal_chunk_header_t));

            u32 fill_val = chk->fill_val();
            news->write((char*)&fill_val, sizeof(fill_val));
        }
        else if(chk->is_skip())
        {
            chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
            ++chunk_cnt;
            chunk.total_sz = NORMAL_CHUNK_HEADER_LEN;

            news->write((char*)&chunk, sizeof(normal_chunk_header_t));
        }
    }

    if(blk_buf) free(blk_buf);
    return chunk_cnt;

}

u32 CImageManager::write_modified_chunk_samsung(QFile *news, QFile *orgs,  CChunk *chk, const tModBlkLst &mod_lst)
{
#if 0
    WRITE_MODIFIED_CHUNK_BODY(news, orgs, chk, mod_lst, samsung_chunk_header_t);
#else
    u32 chunk_cnt = 0;

    tBlockNum chk_start, chk_last, mdb_start, mdb_last;
    chk_start = chk->start_block();
    chk_last = chk->last_block();

    mdb_start = mod_lst.first()->blk_num;
    mdb_last = mod_lst.last()->blk_num;

    char *blk_buf = NULL;
    qint64 f_off = 0;
    //起始
    //
    samsung_chunk_header_t chunk;
    chunk.reserved1 = 0;

    if(chk_start < mdb_start)
    {
        //前面有几块没有改动,保持不变
        //
        chunk.chunk_sz = mdb_start - chk_start;

        if(chk->is_raw())
        {
            chunk.chunk_type = CHUNK_TYPE_RAW;
            ++chunk_cnt;

            chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();
            news->write((char*)&chunk, sizeof(samsung_chunk_header_t));

            f_off = chk->sparse_data_offset();
            orgs->seek(f_off);

            if(!blk_buf) blk_buf = (char*)calloc(1, block_size());
            for(u32 i = 0; i < chunk.chunk_sz; ++i)
            {
                orgs->read(blk_buf, block_size());
                news->write(blk_buf, block_size());
            }
        }
        else if(chk->is_fill())
        {
            chunk.chunk_type = CHUNK_TYPE_FILL;
            ++chunk_cnt;

            chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN + sizeof(u32);
            news->write((char*)&chunk, sizeof(samsung_chunk_header_t));
            u32 fill_val = chk->fill_val();
            news->write((char*)&fill_val, sizeof(fill_val));
        }
        else if(chk->is_skip())
        {
            chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
            ++chunk_cnt;
            chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN;

            news->write((char*)&chunk, sizeof(samsung_chunk_header_t));
        }
    }

    //中间
    //

    int slide_window = 0, next_pos = 0;

    tModifiedBlock *cur, *pre, *next, *tmp;
    for(int pos = 0; pos < mod_lst.size(); pos++)
    {
        cur = mod_lst.at(pos);
        slide_window = 0;

        if(cur->data != NULL)
        {
            chunk.chunk_type    = CHUNK_TYPE_RAW;
            ++chunk_cnt;

            //移动窗口,探测连续的块
            next_pos = pos + 1;
            pre = cur;
            while(next_pos < mod_lst.size())
            {
                next = mod_lst.at(next_pos);

                if(((pre->blk_num + 1)== next->blk_num) && next->data != NULL )
                {
                    ++slide_window;
                    pre = next;
                }
                else
                {
                    break;
                }

                ++next_pos;
            }

            chunk.chunk_sz = slide_window + 1;
            chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();

            news->write((char*)&chunk, sizeof(samsung_chunk_header_t));

            for(u32 i = 0; i < chunk.chunk_sz; ++i)
            {
                tmp = mod_lst.at(pos + i);
                news->write((char*)tmp->data, block_size());
            }
        }
        else
        {
            //删除的统一设置为skip
            //
            chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
            ++chunk_cnt;

            pre = cur;
            next_pos = pos + 1;
            while(next_pos < mod_lst.size())
            {
                next = mod_lst.at(next_pos);
                if(((pre->blk_num + 1)== next->blk_num) && next->data == NULL )
                {
                    ++slide_window;
                    pre = next;
                }
                else
                {
                    break;
                }

                ++next_pos;
            }

            chunk.chunk_sz = slide_window + 1;
            chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN;

            news->write((char*)&chunk, sizeof(samsung_chunk_header_t));
        }
        //根据滑动窗口大小调整位置
        //
        pos += slide_window;
        //pos指向当前已经处理了的最后一个

        // 处理空窗
        // 确认剩下的不止一个,若还剩一个则不存在空窗
        //
        if(pos < (mod_lst.size() - 1))
        {
            pre = mod_lst.at(pos);// 当前已经被处理的最后一个块
            next = mod_lst.at(pos + 1);

            if((pre->blk_num + 1) < next->blk_num)
            {
                //存在空窗
                chunk.chunk_sz = next->blk_num - pre->blk_num - 1;

                if(chk->is_raw())
                {
                    chunk.chunk_type = CHUNK_TYPE_RAW;
                    ++chunk_cnt;

                    chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();
                    news->write((char*)&chunk, sizeof(samsung_chunk_header_t));

                    f_off = chk->sparse_data_offset() + (pre->blk_num - chk_start + 1) * block_size();
                    orgs->seek(f_off);

                    if(!blk_buf) blk_buf = (char*)calloc(1, block_size());
                    for(u32 i = 0; i < chunk.chunk_sz; ++i)
                    {
                        orgs->read(blk_buf, block_size());
                        news->write(blk_buf, block_size());
                    }
                }
                else if(chk->is_fill())
                {
                    chunk.chunk_type = CHUNK_TYPE_FILL;
                    ++chunk_cnt;
                    chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN + sizeof(u32);

                    news->write((char*)&chunk, sizeof(samsung_chunk_header_t));
                    u32 fill_val = chk->fill_val();
                    news->write((char*)&fill_val, sizeof(fill_val));
                }
                else if(chk->is_skip())
                {
                    chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
                    ++chunk_cnt;
                    chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN;

                    news->write((char*)&chunk, sizeof(samsung_chunk_header_t));
                }
            }
        }
    }

    //最后
    //
    if(mdb_last < chk_last)
    {
        //后面有几块没有改动,保持不变
        //
        //设置块数
        //
        chunk.chunk_sz = chk_last - mdb_last;

        if(chk->is_raw())
        {
            chunk.chunk_type = CHUNK_TYPE_RAW;
            ++chunk_cnt;

            chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();
            news->write((char*)&chunk, sizeof(samsung_chunk_header_t));

            f_off = chk->sparse_data_offset() + (mdb_last - chk_start + 1) * block_size();
            orgs->seek(f_off);

            if(!blk_buf) blk_buf = (char*)calloc(1, block_size());
            for(u32 i = 0; i < chunk.chunk_sz; ++i)
            {
                orgs->read(blk_buf, block_size());
                news->write(blk_buf, block_size());
            }
        }
        else if(chk->is_fill())
        {
            chunk.chunk_type = CHUNK_TYPE_FILL;
            ++chunk_cnt;

            chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN + sizeof(u32);
            news->write((char*)&chunk, sizeof(samsung_chunk_header_t));

            u32 fill_val = chk->fill_val();
            news->write((char*)&fill_val, sizeof(fill_val));
        }
        else if(chk->is_skip())
        {
            chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
            ++chunk_cnt;
            chunk.total_sz = SAMSUNG_CHUNK_HEADER_LEN;
            news->write((char*)&chunk, sizeof(samsung_chunk_header_t));
        }
    }

    if(blk_buf) free(blk_buf);
    return chunk_cnt;
#endif
}

u32 CImageManager::write_modified_chunk_marvell(QFile *news,
                                                QFile *orgs,
                                                CChunk *chk,
                                                const tModBlkLst &mod_lst)
{
#if 0
    WRITE_MODIFIED_CHUNK_BODY(news, orgs, chk, mod_lst, marvell_chunk_header_t);
#else
    u32 chunk_cnt = 0;

    tBlockNum chk_start, chk_last, mdb_start, mdb_last;
    chk_start = chk->start_block();
    chk_last = chk->last_block();

    mdb_start = mod_lst.first()->blk_num;
    mdb_last = mod_lst.last()->blk_num;

    char *blk_buf = NULL;
    qint64 f_off = 0;
    //起始
    //
    marvell_chunk_header_t chunk;
    chunk.reserved1 = 0;

    if(chk_start < mdb_start)
    {
        //前面有几块没有改动,保持不变
        //
        chunk.chunk_sz = mdb_start - chk_start;

        if(chk->is_raw())
        {
            chunk.chunk_type = CHUNK_TYPE_RAW;
            ++chunk_cnt;

            chunk.total_sz = MARVELL_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();
            news->write((char*)&chunk, sizeof(marvell_chunk_header_t));

            f_off = chk->sparse_data_offset();
            orgs->seek(f_off);

            if(!blk_buf) blk_buf = (char*)calloc(1, block_size());
            for(u32 i = 0; i < chunk.chunk_sz; ++i)
            {
                orgs->read(blk_buf, block_size());
                news->write(blk_buf, block_size());
            }
        }
        else if(chk->is_fill())
        {
            chunk.chunk_type = CHUNK_TYPE_FILL;
            ++chunk_cnt;
            u32 fill_val = chk->fill_val();
            chunk.total_sz = MARVELL_CHUNK_HEADER_LEN + sizeof(u32);
            news->write((char*)&chunk, sizeof(marvell_chunk_header_t));

            news->write((char*)&fill_val, sizeof(fill_val));
        }
        else if(chk->is_skip())
        {
            chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
            ++chunk_cnt;
            chunk.total_sz = MARVELL_CHUNK_HEADER_LEN;

            news->write((char*)&chunk, sizeof(marvell_chunk_header_t));
        }
    }

    //中间
    //

    int slide_window = 0, next_pos = 0;

    tModifiedBlock *cur, *pre, *next, *tmp;
    for(int pos = 0; pos < mod_lst.size(); pos++)
    {
        cur = mod_lst.at(pos);
        slide_window = 0;

        if(cur->data != NULL)
        {
            chunk.chunk_type    = CHUNK_TYPE_RAW;
            ++chunk_cnt;

            //移动窗口,探测连续的块
            next_pos = pos + 1;
            pre = cur;
            while(next_pos < mod_lst.size())
            {
                next = mod_lst.at(next_pos);

                if(((pre->blk_num + 1)== next->blk_num) && next->data != NULL )
                {
                    ++slide_window;
                    pre = next;
                }
                else
                {
                    break;
                }

                ++next_pos;
            }

            chunk.chunk_sz = slide_window + 1;
            chunk.total_sz = MARVELL_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();

            news->write((char*)&chunk, sizeof(marvell_chunk_header_t));

            for(u32 i = 0; i < chunk.chunk_sz; ++i)
            {
                tmp = mod_lst.at(pos + i);
                news->write((char*)tmp->data, block_size());
            }
        }
        else
        {
            //删除的统一设置为skip
            //
            chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
            ++chunk_cnt;

            next_pos = pos + 1;
            pre = cur;
            while(next_pos < mod_lst.size())
            {
                next = mod_lst.at(next_pos);
                if(((pre->blk_num + 1)== next->blk_num) && next->data == NULL )
                {
                    ++slide_window;
                    pre = next;
                }
                else
                {
                    break;
                }

                ++next_pos;
            }

            chunk.chunk_sz = slide_window + 1;
            chunk.total_sz = MARVELL_CHUNK_HEADER_LEN;

            news->write((char*)&chunk, sizeof(marvell_chunk_header_t));
        }
        //根据滑动窗口大小调整位置
        //
        pos += slide_window;
        //pos指向当前已经处理了的最后一个

        // 处理空窗
        // 确认剩下的不止一个,若还剩一个则不存在空窗
        //
        if(pos < (mod_lst.size() - 1))
        {
            pre = mod_lst.at(pos);// 当前已经被处理的最后一个块
            next = mod_lst.at(pos + 1);

            if((pre->blk_num + 1) < next->blk_num)
            {
                //存在空窗
                chunk.chunk_sz = next->blk_num - pre->blk_num - 1;

                if(chk->is_raw())
                {
                    chunk.chunk_type = CHUNK_TYPE_RAW;
                    ++chunk_cnt;

                    chunk.total_sz = MARVELL_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();
                    news->write((char*)&chunk, sizeof(marvell_chunk_header_t));

                    f_off = chk->sparse_data_offset() + (pre->blk_num - chk_start + 1) * block_size();
                    orgs->seek(f_off);

                    if(!blk_buf) blk_buf = (char*)calloc(1, block_size());
                    for(int i = 0; i < chunk.chunk_sz; ++i)
                    {
                        orgs->read(blk_buf, block_size());
                        news->write(blk_buf, block_size());
                    }
                }
                else if(chk->is_fill())
                {
                    chunk.chunk_type = CHUNK_TYPE_FILL;
                    ++chunk_cnt;

                    chunk.total_sz = MARVELL_CHUNK_HEADER_LEN + sizeof(u32);

                    news->write((char*)&chunk, sizeof(marvell_chunk_header_t));
                    u32 fill_val = chk->fill_val();
                    news->write((char*)&fill_val, sizeof(fill_val));
                }
                else if(chk->is_skip())
                {
                    chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
                    ++chunk_cnt;
                    chunk.total_sz = MARVELL_CHUNK_HEADER_LEN;

                    news->write((char*)&chunk, sizeof(marvell_chunk_header_t));
                }
            }
        }
    }

    //最后
    //
    if(mdb_last < chk_last)
    {
        //后面有几块没有改动,保持不变
        //
        //设置块数
        //
        chunk.chunk_sz = chk_last - mdb_last;

        if(chk->is_raw())
        {
            chunk.chunk_type = CHUNK_TYPE_RAW;
            ++chunk_cnt;

            chunk.total_sz = MARVELL_CHUNK_HEADER_LEN + chunk.chunk_sz * block_size();
            news->write((char*)&chunk, sizeof(marvell_chunk_header_t));

            f_off = chk->sparse_data_offset() + (mdb_last - chk_start + 1) * block_size();
            orgs->seek(f_off);

            if(!blk_buf) blk_buf = (char*)calloc(1, block_size());
            for(u32 i = 0; i < chunk.chunk_sz; ++i)
            {
                orgs->read(blk_buf, block_size());
                news->write(blk_buf, block_size());
            }
        }
        else if(chk->is_fill())
        {
            chunk.chunk_type = CHUNK_TYPE_FILL;
            ++chunk_cnt;
            chunk.total_sz = MARVELL_CHUNK_HEADER_LEN + sizeof(u32);

            news->write((char*)&chunk, sizeof(marvell_chunk_header_t));

            u32 fill_val = chk->fill_val();
            news->write((char*)&fill_val, sizeof(fill_val));
        }
        else if(chk->is_skip())
        {
            chunk.chunk_type = CHUNK_TYPE_DONT_CARE;
            ++chunk_cnt;
            chunk.total_sz = MARVELL_CHUNK_HEADER_LEN;

            news->write((char*)&chunk, sizeof(marvell_chunk_header_t));
        }
    }

    if(blk_buf) free(blk_buf);
    return chunk_cnt;
#endif
}
