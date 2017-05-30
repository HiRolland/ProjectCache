#include "CAndroidImage.h"

#include <QTextStream>
#include "CFileInfo.h"

#include "Logger.h"



/////////////////////////////////////////////////////////////////////
/// \brief CAndroidImage::CAndroidImage
/// \param img_name
///
///

CAndroidImage::CAndroidImage()
    : bool_img_ready(false)
{
    init();
}

bool CAndroidImage::set_image_filename(const QString &name)
{
    if(bool_mnt)
    {
        return false;
    }
    //重新初始化
    init();
    image_file_name = name;
    bool_img_ready = QFile(image_file_name).exists();
    if(!bool_img_ready)
    {
        return false;
    }

    return true;
}

CAndroidImage::~CAndroidImage()
{
    umount();
}

bool CAndroidImage::mount()
{
    INFO("mount now");
    if(!bool_img_ready)
    {
        INFO("img not ready!");
        return false;
    }

    if(bool_mnt ) return true;

    int sparse_hdr_offset = 0;
    get_sparse_info(image_file_name, img_info);
    if(img_info.type == tImgType_unkown)
    {
        INFO("unkown format format!");
        return false;
    }
    else
    {
        INFO("is sparse format!");
        if(img_info.type == tImgType_sparse_cpd_normal)
        {
            img_info.type = tImgType_sparse_normal;
            sparse_hdr_offset = CAndroidImage::get_sparse_header_offset(image_file_name);
        }
        else if(img_info.type == tImgType_sparse_cpd_marvell)
        {
            img_info.type = tImgType_sparse_marvell;
            sparse_hdr_offset = CAndroidImage::get_sparse_header_offset(image_file_name);
        }
        else if(img_info.type == tImgType_sparse_cpd_samsung)
        {
            INFO("is samsung sparse!!");
            img_info.type = tImgType_sparse_samsung;
            sparse_hdr_offset = CAndroidImage::get_sparse_header_offset(image_file_name);
        }
        else if(img_info.type == tImgType_sparse_signed)
        {
            sparse_hdr_offset = CAndroidImage::get_sigend_img_hdr_offset(image_file_name);
        }

    }

    if(sparse_hdr_offset < 0)
    {
        INFO("bad sparse hdr offset ...");
        return false;
    }

    //open img now
    img_file_handler.setFileName(image_file_name);
    if(!img_file_handler.open(QIODevice::ReadWrite |QIODevice::Unbuffered))
    {
        INFO("ERROR:img open failed!");
        return false;
    }

    if(img_info.type == tImgType_ext4)
    {
        sparse_hdr_offset = 0;
        bool_is_sparse = false;

        INFO("this is a ext4 fs image.");
    }
    else
    {
        bool_is_sparse = true;

        INFO("sparse hdr offset : %d", sparse_hdr_offset);
        //
        //seek to right pos
        img_file_handler.seek(sparse_hdr_offset);

        switch(img_info.type)
        {
        case tImgType_sparse_normal:
            //if(!cache_normal_sparse()) return false;
            load_normal_chunk(&img_file_handler);
            break;
        case tImgType_sparse_marvell:
            //cache_marvell_sparse();
            load_marvell_chunk(&img_file_handler);
            break;
        case tImgType_sparse_samsung:
            //cache_samsung_sparse();
            load_samsung_chunk(&img_file_handler);
            break;
            //        case tImgType_sparse_signed:
            //            cache_signed_sparse();
            //            break;
        default:
            break;
        }

        //reset the pos
        img_file_handler.seek(0);

    }


    return _mnt();
}

//////////////////////////////////////////////////////////////////
/// \brief CAndroidImage::init
//////////////////////////////////////////////////////////////////////
void CAndroidImage::init()
{
    m_jnl_start_block   = 0;
    m_jnl_block_cnt     = 0;

    bool_has_xattr          = false;
    bool_is_sparse          = false;
    bool_mnt                = false;
    block_group_desc_data   = NULL;
    bool_img_ready          = false;

    s_img_len = 0;

    s_inode_size = 0;
    s_block_size = 0;

    s_total_blocks = 0;
    s_total_groups = 0;
    s_total_inodes = 0;
    s_journal_blocks = 0;

    s_bg_desc_blocks = 0;
    s_inodes_per_group = 0;
    s_blocks_per_group = 0;
    s_inode_table_blocks = 0;

    s_feat_ro_compat;
    s_feat_compat;
    s_feat_incompat;
    s_bg_desc_reserve_blocks;

    s_real_free_blocks  = 0;

    m_jnl_start_block = 0;
    m_jnl_block_cnt = 0;
    m_jnl_size = 0;
}

void CAndroidImage::detect_jnl_block_info(tBlockNum num)
{
    ext4_inode *inode_ = read_inode(num);
    if(inode_)
    {
        //判断数据块是否连续
        //
        if(INODE_HAS_EXTENT(inode_) )
        {
            struct ext4_extent_header *header;
            struct ext4_extent *extent;
            header = get_ext4_header(inode_);

            if(header->eh_magic == EXT4_EXT_MAGIC
                    && header->eh_depth == 0 && header->eh_entries == 1)
            {
                extent = EXT_FIRST_EXTENT(header);

                u64 block;

                block = (u64)extent->ee_start_lo;
                block |= ((u64) extent->ee_start_hi << 31) << 1;

                m_jnl_start_block = block;
                m_jnl_block_cnt = extent->ee_len;
            }
        }
    }
}

bool CAndroidImage::_mnt()
{
    if(bool_mnt) return true;
    struct ext4_super_block sb;

    qint64 ret;
    int group_desc_bytes, group_desc_blocks;
    char *tmpbuf;
    int inodes_per_group;
    int inode_size;
    int block_size;
    u32 inodes_count;
    u32 total_groups;

    //跳过1024字节读取内容
    ret = read_disk((char*)&sb, 1, 1, 1024);//读取第1024-2047个字节的数据

    if(ret < 1024)
    {
        INFO("Read Data Less than 1024.");
        return false;//表示失败
    }
    if(sb.s_magic != EXT4_SUPER_MAGIC) {
        INFO("not ext4 super magic");
        return false;
    }

    memcpy(&super_block, &sb, sizeof(sb));

    block_size			= (EXT4_MIN_BLOCK_SIZE) << (sb.s_log_block_size);
    inodes_per_group	= sb.s_inodes_per_group;
    inodes_count		= sb.s_inodes_count;
    inode_size			= (sb.s_rev_level == EXT4_GOOD_OLD_REV)? EXT4_GOOD_OLD_INODE_SIZE : sb.s_inode_size;
    total_groups		= DIV_ROUND_UP(((u64)sb.s_blocks_count_hi << 32) + sb.s_blocks_count_lo, sb.s_blocks_per_group);

    if(sb.s_feature_compat & EXT4_FEATURE_COMPAT_EXT_ATTR)
    {
        bool_has_xattr = true;
    }

    if(bool_has_xattr){
        INFO("Image Has XAttr.");
    }
    else{
        INFO("Image Does not has XAttr.");
    }

    //total blocks
    s_total_blocks             = ((u64)sb.s_blocks_count_hi << 32) +sb.s_blocks_count_lo;
    s_total_inodes             = sb.s_inodes_count;
    s_total_groups             = total_groups;

    s_inode_size               = sb.s_inode_size;
    s_block_size               = 1024 << sb.s_log_block_size;
    s_img_len                  =   (u64)s_total_blocks*(u64)s_block_size;

    s_blocks_per_group         = sb.s_blocks_per_group;
    s_inodes_per_group         = sb.s_inodes_per_group;

    s_inode_table_blocks       = DIV_ROUND_UP(s_inodes_per_group * s_inode_size, s_block_size);;

    s_feat_ro_compat           = sb.s_feature_ro_compat;
    s_feat_compat              = sb.s_feature_compat;
    s_feat_incompat            = sb.s_feature_incompat;
    s_bg_desc_reserve_blocks   = sb.s_reserved_gdt_blocks;

    //desc				= (struct ext2_group_desc *)calloc(total_groups, sizeof(struct ext2_group_desc));
    group_desc_bytes	= sizeof(struct ext2_group_desc) * total_groups;
    group_desc_blocks	= DIV_ROUND_UP(group_desc_bytes, block_size);

    s_bg_desc_blocks           = group_desc_blocks;

    tmpbuf				= (char *)calloc(1, group_desc_blocks * block_size);
    if( NULL == tmpbuf)
    {
        return false;
    }

    block_group_desc_data = (struct ext2_group_desc *)calloc(s_total_groups, sizeof(struct ext2_group_desc));// calloc(s_block_size, block_group_desc_data_blocks);
    if (!block_group_desc_data)
        return false;

    //从正常的位置读取,
    if((block_size/1024) <= 1)
        read_disk(tmpbuf, 2, group_desc_blocks, block_size);
    else
        read_disk(tmpbuf, 1, group_desc_blocks, block_size);

    memcpy(block_group_desc_data, tmpbuf, group_desc_bytes);
    free(tmpbuf);

    INFO("Mount Image Ok!\n");
    INFO("Image Base Info:\n");
    INFO("  blocks per group: %d\n", s_blocks_per_group);
    INFO("  inodes per group: %d\n", s_inodes_per_group);
    INFO("  total blocks: %d\n", s_total_blocks);
    INFO("  total inodes: %d\n", s_total_inodes);
    INFO("  total groups: %d\n", s_total_groups);
    INFO("  block group blocks: %d\n", s_bg_desc_blocks);
    INFO("  block group reserve blocks: %d\n", s_bg_desc_reserve_blocks);
    INFO("  inode table blocks: %d\n", s_inode_table_blocks);
    INFO("  free blocks: %d\n", super_block.s_free_blocks_count_lo);
    INFO("  free inodes: %d\n", super_block.s_free_inodes_count);

    m_b_has_jnl = false;

    if(sb.s_feature_compat & EXT4_FEATURE_COMPAT_HAS_JOURNAL)
    {
        m_jnl_start_block = -1;
        m_jnl_block_cnt = 0;
        detect_jnl_block_info(super_block.s_journal_inum);
        m_b_has_jnl = true;

        m_jnl_size = m_jnl_block_cnt * s_block_size;
    }

    detect_layout_ext();

    get_root_file_list();

//    tFileInfoList lst = root_fileinfo->get_directly_child_list();
//    foreach(CFileInfo *i, lst)
//    {
//        qDebug() << i->filename() << i->get_data_block_nums().count();
//        if(!i->get_data_block_nums().isEmpty())
//        {
//            qDebug() << i->get_data_block_nums().first();
//        }

//    }


    bool_mnt = true;

    return true;
}

bool CAndroidImage::is_journal_block(tBlockNum num)
{
    return (num >= m_jnl_start_block)
            &&
            (num < (m_jnl_start_block + m_jnl_block_cnt));
}

bool CAndroidImage::umount()
{
    if(!bool_mnt)
        return false;

    memset(&m_sparse_hdr, 0, sizeof(m_sparse_hdr));

    RELEASE_POINTER(block_group_desc_data);

    clear_sparse_info();

    foreach(CFileInfo *info, file_info_list)
    {
        delete info;
    }
    file_info_list.clear();

    foreach(CBlockGroup *group, block_group_vec)
    {
        delete group;
    }
    block_group_vec.clear();

    bool_has_xattr = false;

    if(img_file_handler.isOpen())
        img_file_handler.close();

    m_jnl_start_block   = 0;
    m_jnl_block_cnt     = 0;

    s_img_len = 0;

    s_inode_size = 0;
    s_block_size = 0;

    s_total_blocks = 0;
    s_total_groups = 0;
    s_total_inodes = 0;
    s_journal_blocks = 0;

    s_bg_desc_blocks = 0;
    s_inodes_per_group = 0;
    s_blocks_per_group = 0;
    s_inode_table_blocks = 0;

    s_feat_ro_compat = 0;
    s_feat_compat = 0;
    s_feat_incompat = 0;
    s_bg_desc_reserve_blocks = 0;

    bool_mnt = false;
    bool_img_ready = false;
    bool_is_sparse = false;

    return true;
}

void CAndroidImage::test()
{
    return;
}
void CAndroidImage::allocate_blocks(int blocks)
{
    super_block.s_free_blocks_count_lo -= blocks;
    foreach(CBlockGroup *group, block_group_vec)
    {
        if(group && group->has_super_block())
        {
            group->super_block()->s_free_blocks_count_lo -= blocks;
        }
    }
}
void CAndroidImage::release_blocks(int blocks)
{
    super_block.s_free_blocks_count_lo += blocks;
    foreach(CBlockGroup *group, block_group_vec)
    {
        if(group && group->has_super_block())
        {
            group->super_block()->s_free_blocks_count_lo += blocks;
        }
    }
}

void CAndroidImage::allocate_inodes(int inodes)
{
    super_block.s_free_inodes_count -= inodes;
    foreach(CBlockGroup *group, block_group_vec)
    {
        if(group && group->has_super_block())
        {
            group->super_block()->s_free_inodes_count -= inodes;
        }
    }
}
void CAndroidImage::release_inodes(int inodes)
{
    super_block.s_free_inodes_count += inodes;
    foreach(CBlockGroup *group, block_group_vec)
    {
        if(group && group->has_super_block())
        {
            group->super_block()->s_free_inodes_count += inodes;
        }
    }
}
