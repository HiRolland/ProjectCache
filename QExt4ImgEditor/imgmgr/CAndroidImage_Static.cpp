#include "CAndroidImage.h"
#include <QTextStream>
#include "CFileInfo.h"
//#include "CYaffs2Parser.h"
#include <QDebug>
#define SEC_IMG_MAGIC                (0x53535353)
#define FB_IMG_MAGIC                 (0x46424642)
#define SEC_IMG_HDR_SZ               (64)

#define FIX_FB_PADDING_HEADER_SIZE 0x4000 //default is 16KB
typedef struct _SEC_FB_HEADER
{
    unsigned int magic_num;
    unsigned int hdr_ver;
    unsigned int hash_count;
    unsigned int chunk_size;
    unsigned char part_name[32];
    unsigned int orig_img_size;
    unsigned char reserved[12];
} SEC_FB_HEADER;

typedef struct _SEC_IMG_HEADER
{
    unsigned int magic_num;

    /* After WK1151, the size of customer name will be changed from 16
       bytes to 32 bytes due to customer's request. To distinguish between the
       old version and new version, the simplest way is to check the value of
       signature_length.

       Flash tool downloads images by using new format
       => Tool can find the image is old because signature_length is all 0x00.
          Therefore, Flash tool will automatically apply old image format */

    /* After WK1151 */
    unsigned char cust_name [32];
    unsigned int img_ver;
    unsigned int img_len;
    unsigned int img_off;

    unsigned int s_off;
    unsigned int s_len;

    unsigned int sig_off;
    unsigned int sig_len;

    /* Before WK1151 */
#if 0
    unsigned char cust_name [16];
    unsigned int img_ver;
    unsigned int img_len;
    unsigned int img_off;

    unsigned int s_off;
    unsigned int s_len;

    unsigned int sig_off;
    unsigned int sig_len;
    unsigned char dummy [16];
#endif


} SEC_IMG_HEADER;

void CAndroidImage::test_signed_img(const QString &imgname)
{
    QFile img(imgname);
    int offset = -1;
    if(img.open(QIODevice::ReadOnly))
    {
        img.seek(0);

        do
        {
            SEC_FB_HEADER fb_header;
            img.read((char*)&fb_header, sizeof(fb_header));

            if(fb_header.magic_num != FB_IMG_MAGIC)
            {
                break;
            }
            else
            {
            }

            offset = FIX_FB_PADDING_HEADER_SIZE;
            SEC_IMG_HEADER img_header;
            while(img.seek(offset))
            {
                memset(&img_header, 0, sizeof(SEC_IMG_HEADER));
                img.read((char*)&img_header, sizeof(img_header));

                if(img_header.magic_num != SEC_IMG_MAGIC)
                {
                    break;
                }
                else{
                    offset += img_header.s_len + 64;
                }
            }

        }while(0);


        img.close();
    }
}

bool CAndroidImage::get_img_sparse_info_bytes(const QString &img_name,
                                              tImageInfo &info,
                                              qint64& free_bytes,
                                              qint64& total_bytes)
{
    free_bytes = 0;
    total_bytes = 0;
    if(!get_sparse_info(img_name, info))
        return false;

    if(info.type == tImgType_yaffs2)
    {

        return true;
    }
    else
    {
        CAndroidImage temp;
        temp.set_image_filename(img_name);
        if(temp.mount())
        {
            info.is_sparse = temp.img_info.is_sparse;
            info.type = temp.img_info.type;

            free_bytes = temp.get_free_blocks_count() * temp.get_block_size();

            total_bytes = temp.get_img_volume();
            temp.umount();
            return true;
        }
    }


    return false;
}

qint64 CAndroidImage::get_img_free_bytes(const QString &img_name)
{

    tImageInfo info;
    if(!get_sparse_info(img_name, info))
        return 0;
    if(info.type == tImgType_yaffs2)
    {

    }
    else
    {

        CAndroidImage temp;
        temp.set_image_filename(img_name);
        if(temp.mount())
        {
            qint64 ret = temp.get_free_blocks_count() * temp.get_block_size();

            temp.umount();
            return ret;
        }
    }
    return 0;
}

bool CAndroidImage::get_sparse_info(const QString &img_name, tImageInfo &info)
{
    qint64 read_len;
    sparse_header_t sparse_hdr;
    chunk_header_t chunk_hdr;

    bool bRet = false;

    memset(&info, 0, sizeof(tImageInfo));
    info.type = tImgType_unkown;

    QFile img(img_name);
    if(img.open(QIODevice::ReadOnly))
    {
        read_len = img.read((char*)&sparse_hdr, SPARSE_HEADER_LEN);
        if(read_len == SPARSE_HEADER_LEN && sparse_hdr.magic == SPARSE_HEADER_MAGIC)
        {
            info.is_sparse = 1;
            bRet = true;

            if(sparse_hdr.file_hdr_sz > SPARSE_HEADER_LEN)
            {
                img.seek(sparse_hdr.file_hdr_sz);
                img.read((char*)&chunk_hdr, sizeof(chunk_header_t));
                if ((chunk_hdr.chunk_type == CHUNK_TYPE_RAW && chunk_hdr.total_sz != (sparse_hdr.chunk_hdr_sz + (chunk_hdr.chunk_sz * sparse_hdr.blk_sz)))
                        || (chunk_hdr.chunk_type == CHUNK_TYPE_FILL && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz + sizeof(u32))
                        || (chunk_hdr.chunk_type == CHUNK_TYPE_DONT_CARE && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz))
                {
                    info.type = tImgType_sparse_marvell;
                }
                else{
                    info.type = tImgType_sparse_samsung;
                }
            }
            else
            {
                info.type = tImgType_sparse_normal;
            }

            img.close();

            return true;
        }
        else
        {
            struct ext4_super_block sb;
            img.seek(1024);
            img.read((char*)&sb, sizeof(sb));

            if(sb.s_magic == EXT4_SUPER_MAGIC)
            {
                info.type = tImgType_ext4;

                img.close();
                return true;
            }
            else{
#define BUF_SIZE 4096
                char buf[BUF_SIZE] = {0};

                img.seek(0);
                img.read(buf, BUF_SIZE);

                int limit = BUF_SIZE - sizeof(sparse_hdr);
                bool b_found = false;
                for(int offset = 0; offset < limit; ++offset)
                {
                    memcpy(&sparse_hdr, buf + offset, sizeof(sparse_hdr));

                    if(sparse_hdr.magic == SPARSE_HEADER_MAGIC &&
                            (sparse_hdr.file_hdr_sz == 28 || sparse_hdr.file_hdr_sz == 32) )
                    {
                        b_found = true;


                        info.type = tImgType_sparse_cpd_normal;

                        if(sparse_hdr.file_hdr_sz > SPARSE_HEADER_LEN)
                        {
                            img.seek(sparse_hdr.file_hdr_sz + offset);

                            img.read((char*)&chunk_hdr, sizeof(chunk_header_t));
                            if ((chunk_hdr.chunk_type == CHUNK_TYPE_RAW && chunk_hdr.total_sz != (sparse_hdr.chunk_hdr_sz + (chunk_hdr.chunk_sz * sparse_hdr.blk_sz)))
                                    || (chunk_hdr.chunk_type == CHUNK_TYPE_FILL && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz + sizeof(u32))
                                    || (chunk_hdr.chunk_type == CHUNK_TYPE_DONT_CARE && chunk_hdr.total_sz != sparse_hdr.chunk_hdr_sz))
                            {
                                info.type = tImgType_sparse_cpd_marvell;
                            }
                            else
                            {
                                info.type = tImgType_sparse_cpd_samsung;
                            }
                        }

                        break;
                    }
                }

                if(b_found)
                {
                    img.close();
                    return true;
                }
            }
        }
#if 1
        {
            SEC_FB_HEADER fb_header;
            img.seek(0);
            img.read((char*)&fb_header, sizeof(fb_header));

            if(fb_header.magic_num == FB_IMG_MAGIC)
            {
                img.close();
                info.type = tImgType_sparse_signed;
                return true;
            }
        }
#endif

        img.close();
    }

    //check if aliyun
//    if(CYaffs2Parser().is_yaffs2(img_name))
//    {
//        info.type = tImgType_yaffs2;
//        return true;
//    }


    return bRet;
}

int CAndroidImage::get_sparse_header_offset(const QString &img_name)
{
    QFile img(img_name);

    int offset = -1;
    if(img.open(QIODevice::ReadOnly))
    {

        if(seek_to_sparse_hdr(img))
        {
            offset = img.pos();
        }

        img.close();
    }

    return offset;
}

int CAndroidImage::get_sigend_img_hdr_offset(const QString &img_name)
{
    QFile img(img_name);
    int offset = -1;
    if(img.open(QIODevice::ReadOnly))
    {
        img.seek(0);

        do
        {
            SEC_FB_HEADER fb_header;
            img.read((char*)&fb_header, sizeof(fb_header));

            if(fb_header.magic_num != FB_IMG_MAGIC)
            {
                break;
            }

            if(!img.seek(FIX_FB_PADDING_HEADER_SIZE))
                break;

            SEC_IMG_HEADER img_header;
            img.read((char*)&img_header, sizeof(img_header));

            if(img_header.magic_num != SEC_IMG_MAGIC)
            {
                break;
            }

            offset = FIX_FB_PADDING_HEADER_SIZE + img_header.img_off;
        }while(0);


        img.close();
    }


    ////qDebug() <<"ali offset " << offset;
    return offset;
}

