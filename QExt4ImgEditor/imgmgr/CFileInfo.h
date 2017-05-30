#ifndef CFILEINFO_H
#define CFILEINFO_H

#include "mogu.h"
#include <QStringList>
#include <QList>
#include <QHash>

#include "android_capability.h"
/**
 * @brief The CFileInfo class
 *
 */
class CFileInfo;
class CAndroidImage;
typedef QList<CFileInfo* >  tFileInfoList;
class CFileInfo
{
    friend class CAndroidImage;
public:
    //defualt is dir
    explicit CFileInfo(CAndroidImage* img,
                       tInodeNum num,
                       u8 type,
                       struct ext4_inode *ei,
                       const QString& _filename,
                       CFileInfo *_parent = NULL,
                       bool is_new = false);
    ~CFileInfo();

    inline bool is_dir(){return file_type_ == EXT4_FT_DIR;}
    inline bool is_file(){return file_type_ == EXT4_FT_REG_FILE;}
    inline bool is_link(){return file_type_ == EXT4_FT_SYMLINK;}
    inline u8 file_type(){return file_type_;}
    bool is_root();

    /**
     * @brief get_one_child_file获取一个suffix类型的文件，且是一个原有的
     * @param suffix
     * @return
     */
    CFileInfo *get_one_child_file(const QString& suffix);

    /**
     * @brief get_one_origin_file 获取目录中一个原始文件，不关系类型
     * @return
     */
    CFileInfo *get_one_origin_file();
    /**
     * @brief get_one_child_folder 获取一个子目录的文件夹的CFileInfo
     * @return
     */
    CFileInfo *get_one_child_folder();
    /**
     * @brief is_new 是否新添加的文件,
     * 新添加的由CImageManager管理, 否则由CAndroidImage管理
     *
     * @return
     */
    bool is_new(){return bool_new;}


    /**
     * @brief set_link_to 若是软连接对象，则用于设置连接目标,否则设置没有任何意义
     * @param to
     */
    inline void set_link_to(const QString& to){link_target = to;}
    inline const QString& get_link_to(){return link_target;}

    inline qint64 inode_num(){return inode_num_;}
    inline const QString& filename(){return filename_;}
    inline CFileInfo * parent(){return is_root() ? NULL :parent_;}
    inline ext4_inode* inode(){return inode_;}
    inline u16 mode(){return inode_->i_mode & 07777;}
    inline u16 uid(){return inode_->i_uid;}
    inline u16 gid(){return inode_->i_gid;}

    //TODO:test
    inline void set_gid_bit(){inode_->i_mode |= UNIX_S_ISGID;}
    inline void set_uid_bit(){inode_->i_mode |= UNIX_S_ISUID;}
    inline void set_sticky_bit(){inode_->i_mode |= UNIX_S_ISVTX;}
    void set_mode(u16 mode);
    inline void set_uid(u16 uid){inode_->i_uid = uid;}
    inline void set_gid(u16 gid){inode_->i_gid = gid;}

    inline const QString& selinux(){return selinux_;}
    void reset_selinux(const QString& se);/*{selinux = se;}*/

    /**
     * @brief get_start_block_num 获取起始的块号block num
     * @return
     */
    inline tBlockNum first_data_block_num(){return data_block_nums.first();}

    /**
     * @brief block_count 设置文件数据所占的块数
     * @return
     */
    inline int block_count(){return data_block_nums.size();}
    void clear();
    bool empty(){return inode_num_ == 0;}


    /**
     * @brief parent_path 获取文件路径,不包含本文件(夹)名,以 '/'结尾
     * @return
     */
    inline const QString& parent_path(){return parent_path_;}

    /**
     * @brief get_file_path获取文件路径,包含本文件(夹)名
     * @return
     */
    QString file_path();

    /**
     * @brief dump_full_path 导出文件到 out_path, 自动创建相应的父目录
     * @param out_path
     */
    bool dump_file_path(const QString& out_path);

    /**
     * @brief dump_file_only 导出文件到目录out_path下,不管其父目录
     * @param out_path
     */
    bool dump_file_only(const QString& out_path);

    /**
     * @brief get_all_childs 获取所有的子文件夹和文件,包括所有层次, 但返回结果不包含自身
     * @param list
     *
     */
    void get_all_childs(tFileInfoList &list);


    /**
     * @brief get_directly_childs 获取直接包含的子文件夹和文件
     * @return
     */
    const tFileInfoList& get_directly_child_list();

    /**
     * @brief delete_origin_child删除直接子文件或目录
     * @param child child不是直接文件,则不做任何动作
     * 注意: 若child是img中原始的文件,从child_list删除的同时,也将child添加到删除列表中
     *      若child是新添加的文件夹,则仅仅从child_list中删除该child
     */
    bool delete_child(CFileInfo *child);


    bool if_del(){return bool_has_del_op;}
    bool if_add(){return bool_has_add_op;}

    /**
     * @brief dump_all_childs如果这是一个目录,将所有的内容导出到 local_path
     */
    void dump_all_childs(const QString& local_path);

    /**
     * @brief has_uncontinues_block 判断数据块是连续
     * @return
     */
    bool has_uncontinues_block(){return bool_blocks_uncontinuous;}

    const tBlockNumArray& get_data_block_nums(){return data_block_nums;}

    CFileInfo *get_child_by_filename(const QString& name);

    bool has_childs(){return !child_list.isEmpty();}

    /**
     * @brief get_delete_list 只会记录原有的被删除的文件夹,对于新添加的则不会记录
     * @return
     */
    inline tFileInfoList & get_delete_list(){return delete_child_list;}

    /**
     * @brief change_filename 只有新加的文件才可以重命名
     * @param filename
     */
    void rename(const QString& new_name){if(is_new()) filename_ = new_name;}

    inline bool has_built_dir_entries(){return bool_build_dir_entries;}
    inline void mark_built_dir_entries(){bool_build_dir_entries = true;}

    /**
     * @brief set_file_src_path 设置文件在本地的路径,新文件和原始文件均可以设置
     * @param path
     */
    void set_file_src_path(const QString& path);//{/*if(is_new() && is_file()) */new_file_src_path = path;}
    inline const QString& get_file_src_path(){return new_file_src_path;}

    inline void set_dir_new_src_location(const QString& loc){dir_new_src_location = loc;}
    inline const QString& get_dir_new_src_location(){return dir_new_src_location;}

    inline u64 file_size(){return file_size_;}
    inline const QString& last_modified_time(){return modify_time;}

    /**
     * @brief contains 判断是否存在；首先调用get_child_list,获取所有的子文件目录,
     * @param filename
     * @return
     */
    bool contains(const QString& filename);

    bool contains_type(const QString& suffix);

public:
    //针对apk检测的一些函数
    /**
     * @brief store_apk 判断是否含apk,
     * @param androver5 若apk的文件名(不带后缀)与此目录名一致，则为true 否则为false
     * @return
     */
    bool store_apk(bool& androver5);
    /**
     * @brief has_apk_file 是否含有apk
     * @return
     */
    bool has_apk_file();

    /**
     * @brief get_has_app_child_folder 获取包含apk的所有子目录，
     *                                  已经针对安卓5.0以上格式做了处理，
     * @param has_app_folders
     */
    void get_has_apk_child_folder(tFileInfoList &has_app_folders);


private:


    /**
     * @brief exsit 判断是否登记；
     * 判断当前的child_list中是否包含filename,不会调用get_child_list
     * @param filename
     * @return
     */
    bool exsit(const QString& filename);

    void set_modified_time(ext4_inode * ei);

   // void enroll_origin_child(CFileInfo *child);
    //called by constructor
    bool append_child(CFileInfo *child, bool is_new = false);

    void set_inode(struct ext4_inode *ei);

    bool dump_to(const QString& path_or_file);


    void build_dir_tree_if_need();

private:
    CAndroidImage       *h_img;
    struct ext4_inode  *inode_;
    tInodeNum           inode_num_;
    CFileInfo          *parent_;

    tBlockNumArray      data_block_nums;

    u8                  file_type_;

    QString             filename_;
    QString             link_target;
    QString             parent_path_;

    QString selinux_;
    struct vfs_cap_data cap_data;
    //
    //新添加文件的数据源
    //
    QString new_file_src_path;
    QString dir_new_src_location;

    tFileInfoList       child_list;
    tFileInfoList       delete_child_list;

    tBlockNum extent_block;

    u64 file_size_;

    QString modify_time;

    /**
     * @brief suffix_file_cache 用户获取相应的file
     */
    QHash<QString, CFileInfo *> sub_suffix_file_cache;
    CFileInfo *one_sub_folder;

    bool bool_blocks_uncontinuous;
    bool bool_new;
    bool bool_has_del_op;
    bool bool_has_add_op;

    /**
     * @brief bool_build_childs 是否已经获取了目录项
     */
    bool bool_build_dir_entries;

};

#endif // CFILEINFO_H
