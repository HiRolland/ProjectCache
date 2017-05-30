#include "CFileInfo.h"
#include <Windows.h>
#include <QFile>
#include <QFileInfo>
#include "CMoGuUtilSimp.h"
#include "CAndroidImage.h"

#include "xattr.h"
#include "Logger.h"
static void time_t_2_file_time(time_t t, LPFILETIME pft)
{
    LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
    pft->dwLowDateTime = (DWORD) ll;
    pft->dwHighDateTime = ll >>32;
}

CFileInfo::CFileInfo(CAndroidImage *img,
                     tInodeNum num,
                     u8 type,
                     ext4_inode *ei,
                     const QString &_filename,
                     CFileInfo *_parent,
                     bool is_new)
    : h_img(img)
    , file_type_(type)
    , inode_num_(num)
    , parent_(_parent)
    , filename_(_filename)
    , bool_new(is_new)
    , bool_blocks_uncontinuous(false)
    , file_size_(0)
    , one_sub_folder(NULL)

{
    bool_has_add_op     = false;
    bool_has_del_op     = false;

    bool_build_dir_entries = false;

    set_inode(ei);

    set_modified_time(ei);

    memset(&cap_data, 0, sizeof(cap_data));

    /*在parent中登记此entry**/
    //
    if(parent_)
    {
        parent_->append_child(this, is_new);
    }

    if(inode_->i_extra_isize)
    {
        struct ext4_xattr_ibody_header *hdr;
        struct ext4_xattr_entry *entry;
        char *val;

        hdr = (struct ext4_xattr_ibody_header *) (inode_ + 1);
        entry = (struct ext4_xattr_entry *) (hdr + 1);

        val = (char *) entry + entry->e_value_offs;

        if(!strcmp(entry->e_name, XATTR_SELINUX_SUFFIX)){
            selinux_ = QString::fromUtf8(val, entry->e_value_size);
        }

        while(!IS_LAST_ENTRY(entry))
        {
            entry = EXT4_XATTR_NEXT(entry);

            val = (char *) entry + entry->e_value_offs;
            if(!strcmp(entry->e_name, XATTR_CAPS_SUFFIX))
            {
                memcpy(&cap_data, val, entry->e_value_size);

                break;
            }
        }
    }

    if(selinux_.isEmpty())
    {
        selinux_ = "null";
    }

    if(parent_)
    {
        if(parent_->is_root())
        {
            parent_path_ = QString("/");
        }
        else{
            parent_path_ = parent_->parent_path() + parent_->filename() + QString("/");
        }
    }
    else
    {
        parent_path_ = "";
        //for root dir, the parent dir is itself
        //if(is_root())
        parent_ = this;
    }
}

CFileInfo::~CFileInfo()
{
}

bool CFileInfo::is_root()
{
    return inode_num_ == EXT4_ROOT_INO ? true : false;
}

CFileInfo *CFileInfo::get_one_child_file(const QString &suffix)
{
    if(sub_suffix_file_cache.contains(suffix))
        return sub_suffix_file_cache.value(suffix);

    CFileInfo  * ret = NULL;

    tFileInfoList lst;
    get_all_childs(lst);

    foreach(CFileInfo *cur, lst)
    {
        if(cur->is_file() && !cur->is_new())
        {
            if(suffix.isEmpty())
            {
                ret = cur;
                break;
            }
            else
            {
                if(cur->filename().endsWith(suffix))
                {
                    ret = cur;
                    break;
                }
            }
        }
        else if(cur->is_dir() && !one_sub_folder)
        {
            // to void misc op
            one_sub_folder = cur;
        }
    }

    sub_suffix_file_cache.insert(suffix, ret);

    return NULL;
}

CFileInfo *CFileInfo::get_one_origin_file()
{
    tFileInfoList lst;
    get_all_childs(lst);

    foreach(CFileInfo *cur, lst)
    {
        if(cur->is_file() && !cur->is_new())
            return cur;
    }

    return NULL;
}

CFileInfo *CFileInfo::get_one_child_folder()
{
    if(one_sub_folder)
        return one_sub_folder;

    const tFileInfoList &lst = get_directly_child_list();

    foreach(CFileInfo * cur, lst)
    {
        if(cur->is_dir())
        {
            if(is_root() && cur->filename().contains("lost"))
                continue;
            one_sub_folder = cur;
            break;
        }
    }

    return one_sub_folder;
}

void CFileInfo::set_mode(u16 mode)
{
    u16 b_mode = (0x8000 & inode_->i_mode);
    u16 t_mode = 07777;
    //b_mode &= (~t_mode); /*必须要确定最高位 即第16bit，否则会出现IO错误*/
    inode_->i_mode = (b_mode |(mode & t_mode));
}

void CFileInfo::reset_selinux(const QString &se)
{
    //TODO:清空extent的数据
    if(selinux_ != "null" && selinux_ != se)
    {
        erase_inode_xattr(inode_,  h_img->get_inode_size());

        //qDebug() << "rest linux now..." << se;
        selinux_ = se;

        u32 inode_size = h_img->get_inode_size();

        QByteArray temp = selinux_.toUtf8();
        char * value = temp.data();
        xattr_addto_inode(inode_,
                          EXT4_XATTR_INDEX_SECURITY,
                          XATTR_SELINUX_SUFFIX,
                          value,
                          selinux_.size() + 1, //refer to android source code
                          inode_size);

        if(cap_data.magic_etc)
        {
            xattr_addto_inode(inode_,
                              EXT4_XATTR_INDEX_SECURITY,
                              XATTR_CAPS_SUFFIX,
                              &cap_data,
                              sizeof(cap_data),
                              inode_size);
        }

    }
}

void CFileInfo::set_inode(ext4_inode *ei)
{
    inode_ = ei;

    if(bool_new)
        return;

    file_size_ = ((u64)inode_->i_size_lo + (((u64)inode_->i_size_high)<<32));

    if(is_link())
    {
        return;
    }
    else{
        bool_blocks_uncontinuous = false;

        //判断数据块是否连续
        //
        if(INODE_HAS_EXTENT(inode_) )
        {
            struct ext4_extent_header *header;
            struct ext4_extent *extent;
            header = get_ext4_header(inode_);

            if(header->eh_magic == EXT4_EXT_MAGIC && header->eh_depth == 0 && header->eh_entries == 1)
            {
                extent = EXT_FIRST_EXTENT(header);

                u64 block;

                block = (u64)extent->ee_start_lo;
                block |= ((u64) extent->ee_start_hi << 31) << 1;

                for(int i = 0; i < extent->ee_len; ++i)
                {
                    data_block_nums.append(block + i);
                }
            }
            else{
                bool_blocks_uncontinuous = true;

                //                if(header->eh_depth == 1)
                //                {

                //                }
            }
        }

        if(bool_blocks_uncontinuous)
        {
            //获取所有不连续的块号
            int block_count = DIV_ROUND_UP(file_size_, h_img->get_block_size());

            for(int blkindex = 0; blkindex < block_count; blkindex++)
            {
                tBlockNum blocknum = h_img->get_physic_block_num(inode_, blkindex);
                if(blocknum == -1)
                {
                    break;
                }
                else{
                    data_block_nums.append(blocknum);
                }
            }
        }
    }
}

bool CFileInfo::dump_to(const QString &path_or_file)
{
    if(is_link())
        return false;

    if(is_dir())
    {
        CMoGuUtilSimp::CreateFullPath(path_or_file);

        return true;
    }
    else
    {
        QFile outFile(path_or_file);
        u64 file_len = (u64)inode_->i_size_lo + (((u64)inode_->i_size_high) << 32);
        u32 to_write = 0;

        u32 block_size = h_img->get_block_size();
        if(outFile.open(QIODevice::WriteOnly))
        {
            char *data = (char *)calloc(1,block_size);

            foreach(tBlockNum blknum, data_block_nums)
            {
                h_img->read_disk(data, blknum, 1, block_size);

                to_write = (u32)min(file_len, block_size);

                outFile.write(data, to_write);

                file_len -= to_write;
                if(file_len <=0 )
                    break;
            }

            free(data);

            outFile.flush();
            outFile.close();

            return true;
        }

        return false;
    }
}

void CFileInfo::build_dir_tree_if_need()
{
    //make sure child list has been inited
    if(!bool_new && !bool_build_dir_entries)
    {
        get_directly_child_list();
    }
}

void CFileInfo::get_all_childs(tFileInfoList &list)
{
    const tFileInfoList& clist = get_directly_child_list();

    foreach(CFileInfo *info, clist)
    {
        list.append(info);
        if(info->is_dir())
        {
            info->get_all_childs(list);
        }
    }
}

const tFileInfoList &CFileInfo::get_directly_child_list()
{
    if(!bool_new &&
            !bool_build_dir_entries)
    {
        h_img->build_dir_entry_fileinfo(this);

        bool_build_dir_entries = true;
    }

    return child_list;
}

bool CFileInfo::append_child(CFileInfo *child, bool is_new)
{
    if(is_new)
    {
        if(contains(child->filename()))
        {
            return false;
        }
    }


    if(!child_list.contains(child))
    {
        //child_list.append(child);
        int ipos = 0;
        foreach(CFileInfo *f, child_list)
        {
            if(child->filename() > f->filename())
                ipos++;
        }
        child_list.insert(ipos, child);

        if(is_new && !bool_has_add_op)
            bool_has_add_op = true;

        return true;
    }

    return false;

}

bool CFileInfo::delete_child(CFileInfo *child)
{
    if(child_list.contains(child))
    {
        child_list.removeOne(child);

        /**
         * 只将原有的文件记录在删除列表中，对新添加的由CImageManager 负责删除
         *
         */
        if(!child->is_new() && !delete_child_list.contains(child))
        {
            delete_child_list.append(child);
            bool_has_del_op = true;
        }

        return true;
    }

    return false;
}

void CFileInfo::dump_all_childs(const QString &local_path)
{
    if(!is_dir())
        return;

    QString out_path = local_path;
    out_path.replace('\\', '/');
    if(out_path.endsWith('/'))
    {
        out_path.chop(1);
    }

    if(!CMoGuUtilSimp::PathFileExsits(out_path))
    {
        CMoGuUtilSimp::CreateFullPath(out_path);
    }

    tFileInfoList childs;

    get_all_childs(childs);
    foreach(CFileInfo* cur, childs)
    {
        cur->dump_file_path(out_path);
    }

}

bool CFileInfo::contains(const QString &filename)
{

    build_dir_tree_if_need();

    foreach(CFileInfo *cur, child_list)
    {
        if(cur->filename() == filename)
            return true;
    }

    return false;
}

bool CFileInfo::contains_type(const QString &suffix)
{
    if(!is_dir())
        return false;
    QString sfx = suffix;
    if(!sfx.startsWith('.'))
        sfx.prepend('.');

    const tFileInfoList &lst = get_directly_child_list();
    foreach(CFileInfo * f, lst)
    {
        if(f && f->is_file() && f->filename().endsWith(sfx))
        {
            return true;
        }
    }

    return false;

}

bool CFileInfo::store_apk(bool &androver5)
{
    androver5 = false;

    if(!is_dir())
        return false;

    const tFileInfoList &lst = get_directly_child_list();
    foreach(CFileInfo * f, lst)
    {
        if(f && f->is_file() && f->filename().endsWith(".apk"))
        {
            QString fn = QFileInfo(f->filename()).fileName();
            fn.chop(4);

            if(fn == filename())
            {
                androver5 = true;
            }

            return true;
        }
    }

    return false;
}

bool CFileInfo::has_apk_file()
{
    return contains_type(".apk");
}

void CFileInfo::get_has_apk_child_folder(tFileInfoList &has_app_folders)
{
    if(!is_dir())
        return ;

    if(has_apk_file() && !filename().startsWith('.'))
    {
        has_app_folders.append(this);
    }

    bool is_ad5 = false;
    const tFileInfoList &lst = get_directly_child_list();
    foreach(CFileInfo * f, lst)
    {
        is_ad5 = false;
        if(f->is_dir() && !f->filename().startsWith('.'))
        {
            if(f->store_apk(is_ad5))
            {
                //若f是一个android5类型的app目录(目录名和apk文件名(不带.apk)一样)
                //
                if(is_ad5)
                {
                    if(!has_app_folders.contains(this))
                        has_app_folders.append(this);
                }
                else
                    has_app_folders.append(f);
            }
            else
            {
                f->get_has_apk_child_folder(has_app_folders);
            }
        }
    }
}


bool CFileInfo::exsit(const QString &filename)
{
    foreach(CFileInfo *cur, child_list)
    {
        if(cur->filename() == filename)
            return true;
    }

    return false;
}

CFileInfo *CFileInfo::get_child_by_filename(const QString &name)
{
    if(!is_dir())
        return NULL;

    build_dir_tree_if_need();

    foreach(CFileInfo *cur, child_list)
    {
        if( cur->filename() == name)
            return cur;
    }

    return NULL;
}

///**
// * @brief CFileInfo::has_apk_lessthan_2_layers 判断两级目录
// * @return
// */
//bool CFileInfo::has_apk_less_than_2_layers()
//{
//    if(!is_dir())
//        return false;
//    const tFileInfoList &lst = get_directly_child_list();
//    foreach(CFileInfo *cur, lst)
//    {
//        if(cur->is_file())
//        {
//            if(cur->filename().endsWith(".apk"))
//                return true;
//        }
//        else
//        {
//            //判断次级目录,不能用递归,只需检测次级目录即可
//            //
//            foreach(CFileInfo *child, cur->get_directly_child_list())
//            {
//                if(child->is_file())
//                {
//                    if(child->filename().endsWith(".apk"))
//                        return true;
//                }
//            }
//        }
//    }

//    return false;
//}


void CFileInfo::set_file_src_path(const QString &path)
{
    new_file_src_path = path;
    file_size_ = QFileInfo(path).size();
}

void CFileInfo::set_modified_time(ext4_inode *ei)
{
    FILETIME time;
    time_t_2_file_time(ei->i_mtime, &time);
    SYSTEMTIME stUTC, stLocal;

    char buf[32] = {0};


    // Convert the last-write time to local time.
    FileTimeToSystemTime(&time, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    sprintf(buf, "%02d/%02d/%d  %02d:%02d",
            stLocal.wYear, stLocal.wMonth, stLocal.wDay,
            stLocal.wHour, stLocal.wMinute);
    modify_time= QString(buf);


}

void CFileInfo::clear()
{
    parent_      = NULL;
    inode_       = NULL;
    inode_num_   = 0;
    data_block_nums.clear();
    child_list.clear();

    bool_blocks_uncontinuous = false;
    bool_has_add_op     = false;
    bool_has_del_op     = false;
    bool_build_dir_entries = false;
}

QString CFileInfo::file_path()
{
    return (parent_path_ + filename_);
}

bool CFileInfo::dump_file_path(const QString &out_path)
{

    QString path = out_path;
    path.replace('\\', '/');

    if(!path.endsWith('/'))
        path.append('/');

    path.append(parent_path_);

    if(!CMoGuUtilSimp::PathFileExsits(path))
    {
        CMoGuUtilSimp::CreateFullPath(path);
    }

    path.append(filename_);

    return dump_to(path);
}

bool CFileInfo::dump_file_only(const QString &out_path)
{
    QString path = out_path;
    path.replace('\\', '/');

    if(!path.endsWith('/'))
        path.append('/');

    path.append(filename_);

    // //qDebug() << "apk path:" << path;

    return dump_to(path);
}
