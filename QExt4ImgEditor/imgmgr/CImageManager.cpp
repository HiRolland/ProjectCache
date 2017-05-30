#include "CImageManager.h"

#include "Logger.h"
#include "ext4_utils.h"



/**
 * @brief CImageManager::CImageManager
 *
 *  负责管理对AndroidImage的操作
 *
 */
CImageManager::CImageManager()
    : block_group_vec(android_img.get_group_vec())
    , app_folder(NULL)
    , lib_folder(NULL)
    , vendor_folder(NULL)
    , build_prop(NULL)
    , priv_app(NULL)
    , bool_folder_inode_inited(false)
    , bool_apk_inode_inited(false)
    , bool_so_inode_inited(false)
    , m_pack_hdl_ext(NULL)
{
}

CImageManager::~CImageManager()
{
    cleanup();
}

void CImageManager::init_base_fileinfo()
{
    app_folder      = android_img.get_root_dir_file("app");
    vendor_folder   = android_img.get_root_dir_file("vendor");
    lib_folder      = android_img.get_root_dir_file("lib");
    build_prop      = android_img.get_root_dir_file("build.prop");
    priv_app        = android_img.get_root_dir_file("priv-app");

    //初始化文件夹的inode模版
    //
    u32 inode_size = android_img.get_inode_size();
    if(app_folder)
    {
        bool_folder_inode_inited = true;
        memcpy(folder_inode_example, app_folder->inode(), inode_size);
        INFO("init folder inode template by app folder");
    }


    //初始化apk的inode模版
    //
    if(app_folder)
    {
        tFileInfoList lst;
        app_folder->get_all_childs(lst);
        CFileInfo *apk = NULL;
        foreach(CFileInfo *cur, lst)
        {
            if(cur->filename().endsWith(".apk"))
            {
                apk = cur;
                break;
            }
        }

        if(apk)
        {
            bool_apk_inode_inited = true;
            memcpy(apk_inode_example, apk->inode(), inode_size);

            INFO("init apk inode template.");
        }
    }

    //初始化化libso文件的inode模版
    //
    if(lib_folder)
    {
        tFileInfoList lst;
        lib_folder->get_all_childs(lst);
        CFileInfo * lib_so = NULL;
        foreach (CFileInfo * cur, lst) {
            if(cur->filename().endsWith(".so"))
            {
                lib_so = cur;
                break;
            }
        }

        if(lib_so)
        {
            bool_so_inode_inited = true;
            memcpy(lib_so_inode_example, lib_so->inode(), inode_size);
            INFO("init lib inode template.");
        }
    }
}


bool CImageManager::mount_image(const QString &filename)
{
    if(android_img.is_mnt())
    {
        return false;
    }
    else{
        android_img.set_image_filename(filename);
        if(android_img.mount())
        {
            init_base_fileinfo();

            return true;
        }

        return false;
    }
}

bool CImageManager::save_image(const QString &outimg)
{
    prepare_update_data();

    bool bret = false;
    if(android_img.is_sparse_image())
    {
        INFO("flush into sparse img");
        bret = flush_data_into_sparse_img(outimg);
    }
    else
    {
        bret = flush_data_into_ext4_img(outimg);
    }


    cleanup();

    return true;
}

bool CImageManager::cleanup()
{
    if(android_img.is_mnt())
    {
        INFO("umount image now!");
        android_img.umount();

        INFO("do clear work after umount image!");
        clear_block_buffer_list();

        clear_new_block_data_list();

        clear_new_fileinfo_lst();

        app_folder = NULL;
        lib_folder = NULL;
        vendor_folder = NULL;
        build_prop = NULL;

        bool_folder_inode_inited = false;
        bool_apk_inode_inited = false;
        bool_so_inode_inited = false;

        origin_changed_dir_lst.clear();
        origin_changed_file_lst.clear();

        block_buffer_lst.clear();

        return true;
    }

    return false;
}
