#include "CImageManager.h"
#include <QFileInfo>
#include <QFile>
#include <QDir>
/**
 *添加删除文件和目录, CFileInfo层面
 */


void CImageManager::delete_child(CFileInfo *dir, CFileInfo *child)
{
    if(!dir)
        return;
    /// dir 为空
    ///
    if(dir->delete_child(child))
    {
        if(!dir->is_new())
            origin_dir_changed(dir);
        /**
         * 若child是新添加的文件,在此直接释放掉相应的内测
         *
         */
        if(child->is_new())
            delete_new_added_file(child);
        else
        {
            if(origin_changed_file_lst.contains(child))
            {
                origin_changed_file_lst.removeOne(child);
            }
        }
    }
}

void CImageManager::delete_child(CFileInfo *child)
{
    CFileInfo *parent = child->parent();
    delete_child(parent,child);
}

void CImageManager::replace_file(CFileInfo *file, const QString &loc_filepath)
{
    if(!file->is_file())
        return;

    file->set_file_src_path(loc_filepath);
    if(!file->is_new())
    {
        if(!origin_changed_file_lst.contains(file))
        {
            origin_changed_file_lst.append(file);
        }
    }
}

void CImageManager::origin_dir_changed(CFileInfo *dir)
{
    if(!dir->is_new()&& !origin_changed_dir_lst.contains(dir))
    {
        origin_changed_dir_lst.append(dir);
    }
}

void CImageManager::add_dir_entries_from_local_path(const QString &local_path, CFileInfo *parent)
{
    if(!parent)
        return;
    if(!parent->is_dir())
        return;

    if(!QFileInfo(local_path).exists())
        return;

    //获取本地磁盘locl_path内的文件名称列表
    //
    QFileInfoList fileinfolist = QDir(local_path).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    foreach(const QFileInfo &info, fileinfolist)
    {
        if(info.isDir())
        {
            CFileInfo *new_dir = create_new_folder_in_dir(info.fileName(), parent);
            if(new_dir)
            {
                add_dir_entries_from_local_path(info.absoluteFilePath(), new_dir);
            }
        }
        else{
            create_new_file_in_dir(info.absoluteFilePath(), parent);
        }
    }
}

CFileInfo *CImageManager::add_dir_entry_from_local_folder(const QString &local_folder, CFileInfo *parent)
{
    if(!parent->is_dir())
        return NULL;

    if(!QFileInfo(local_folder).exists())
        return NULL;

    CFileInfo* new_dir = create_new_folder_in_dir(QFileInfo(local_folder).fileName(), parent);

    add_dir_entries_from_local_path(local_folder, new_dir);

    return new_dir;
}

CFileInfo *CImageManager::create_new_folder_in_dir(const QString &filename, CFileInfo *parent)
{
    CFileInfo *dir_ = parent->get_child_by_filename(filename);
    if(dir_ != NULL)
    {
        if(dir_->is_dir())
            return dir_;
        else
            return NULL;
    }
    else{
        //create new

        tInodeNum inodenum = 0;
        ext4_inode* inode = NULL;

        foreach(CBlockGroup* group, block_group_vec)
        {
            inode = group->allocate_inode(inodenum, true);
            if(inode)
            {
                break;
            }
        }

        if(inode)
        {
            //根据类型,选择对应的inode模版初始化inode数据
            //

            //从原有的父目录中(避免使用新建的)选择一个子文件夹，使用该子文件夹的inode进行测试
            //
            CFileInfo *o_dir = parent;
            while(o_dir && o_dir->is_new())
            {
                o_dir = o_dir->parent();
            }

            CFileInfo *folder_template = NULL;
            folder_template = o_dir->get_one_child_folder();

            if(folder_template)
            {
                memcpy(inode, folder_template->inode(), InodeSize);
            }
            else
            {
                memcpy(inode, o_dir->inode(), InodeSize);
            }

            CFileInfo *info = new CFileInfo(&android_img,
                                            inodenum,
                                            EXT4_FT_DIR,
                                            inode,
                                            filename,
                                            parent,
                                            true);

            if(info)
            {
                new_added_dir_list.append(info);

                if(!parent->is_new())
                    origin_dir_changed(parent);
            }

            return info;
        }
        else
        {
            return NULL;
        }
    }
}

CFileInfo *CImageManager::create_new_file_in_dir(const QString &loc_filepath, CFileInfo *parent)
{
    if(!QFile(loc_filepath).exists())
    {
        return NULL;
    }
    QString filename = QFileInfo(loc_filepath).fileName();

    CFileInfo *fi = parent->get_child_by_filename(filename);
    if(fi != NULL)
    {
        if(fi->is_file())
        {
            replace_file(fi, loc_filepath);
            return fi;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        tInodeNum inodenum = 0;
        ext4_inode* inode = NULL;

        foreach(CBlockGroup* group, block_group_vec)
        {
            inode = group->allocate_inode(inodenum, false);
            if(inode)
            {
                break;
            }

        }

        if(inode)
        {

            //根据类型,选择对应的inode模版初始化inode数据
            //

            //从原有的父目录中(避免使用新建的)选择一个子文件夹，
            // 使用该子文件夹的inode进行测试
            //
            CFileInfo *o_dir = parent;
            while(o_dir->is_new())
            {
                o_dir = o_dir->parent();
            }

            CFileInfo *file_template = NULL;

            QString suffix = QFileInfo(filename).suffix();

            if(!suffix.isEmpty())
                suffix.prepend('.');

            file_template = o_dir->get_one_child_file(suffix);
            while(!file_template)
            {

                if(o_dir)
                {
                    file_template = o_dir->get_one_origin_file();
                    o_dir = o_dir->parent();
                }
                else
                {
                    break;
                }

            }

            if(!file_template)
            {
                //FIXME:
                if(filename.endsWith(".so") && bool_so_inode_inited)
                    memcpy(inode, lib_so_inode_example, InodeSize);
                else if(bool_apk_inode_inited)
                    memcpy(inode, apk_inode_example, InodeSize);
                else
                {
                    //do something else
                    if(o_dir)
                    {
                        memcpy(inode, o_dir->inode(), InodeSize);
                    }
                    else
                    {
                        memcpy(inode, get_root_folder()->inode(), InodeSize);
                    }

                    inode->i_mode = UNIX_S_IFREG;

                }
            }
            else
            {
                memcpy(inode, file_template->inode(), InodeSize);
            }

            inode->i_links_count = 1;

            CFileInfo *info = new CFileInfo(&android_img,
                                            inodenum,
                                            EXT4_FT_REG_FILE,
                                            inode,
                                            filename,
                                            parent,
                                            true);

            //inode->i_mode = 35309;//(UNIX_S_IRWXU | UNIX_S_IRWXG | UNIX_S_IRWXO);

            if(info){
                info->set_file_src_path(loc_filepath);
                new_added_file_list.append(info);

                if(!parent->is_new())
                    origin_dir_changed(parent);
            }
            else
            {
                INFO("add new file failed %s", filename.toAscii().data());
            }

            return info;
        }
        else
        {
            INFO("should not run here!");
            return NULL;
        }
    }


}

CFileInfo *CImageManager::create_new_link_in_dir(const QString &filename, const QString& link_to, CFileInfo *dir)
{
    tInodeNum inodenum = 0;
    ext4_inode* inode = NULL;

    foreach(CBlockGroup* group, block_group_vec)
    {
        inode = group->allocate_inode(inodenum, false);
        if(inode)
        {
            break;
        }
    }

    if(inode)
    {
        //根据类型,选择对应的inode模版初始化inode数据
        //

        //TODO:
        memcpy(inode, apk_inode_example, InodeSize);
        inode->i_links_count = 1;
        inode->i_mode = UNIX_S_IFLNK ;

        CFileInfo *info = new CFileInfo(&android_img,
                                        inodenum,
                                        EXT4_FT_SYMLINK,
                                        inode,
                                        filename,
                                        dir,
                                        true);



        if(info)
        {
            new_added_link_list.append(info);
            info->set_link_to(link_to);


            if(!dir->is_new())
                origin_dir_changed(dir);
        }


        return info;
    }
    else
    {
        //should not run here!
        return NULL;
    }
}


void CImageManager::clear_new_fileinfo_lst()
{
    foreach(CFileInfo * info, new_added_dir_list)
    {
        delete info;
    }
    new_added_dir_list.clear();

    foreach(CFileInfo * info, new_added_file_list)
    {
        delete info;
    }
    new_added_file_list.clear();

    foreach(CFileInfo * info, new_added_link_list)
    {
        delete info;
    }
    new_added_link_list.clear();
}


