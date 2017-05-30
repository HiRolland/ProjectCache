#ifndef CIMAGEMANAGER_H
#define CIMAGEMANAGER_H
#include "mogu.h"
#include "CAndroidImage.h"
#include "CFileInfo.h"
#include "CBlockData.h"
#include <QList>
#include "Logger.h"

/**
 * @brief The tFragementBlock struct
 * 连续的块分配记录
 */
typedef struct _BlockMemoryBuffer
{
    u32 blocks;
    u8 data[0];
}BlockMemoryBuffer;
typedef QList<BlockMemoryBuffer* > tBlockMemoryBufferLst;
class CAndroidImage;

/**
 * @brief The CImageManager class
 * img的编辑管理类,负责分配,收回inode,block;
 * 负责创建和管理新加文件(夹)的CFileInfo对象
 * (对于原有的CFileInfo由CAndroidImage负责创建和管理)
 *
 */

class CImageHandlerExt;
class CImageManager
{
public:
    CImageManager();
    ~CImageManager();

    //void test();
    bool mount_image(const QString& filename);
    bool save_image(const QString& outimg );

    bool cleanup();

    /**
     * @brief delete_child 从目录 dir中删除文件或文件夹 child
     * @param dir
     * @param child
     */
    void delete_child(CFileInfo *dir, CFileInfo *child);

    void delete_child(CFileInfo *child);

    /**
     * @brief replace_file 替换原有文件
     * @param file
     * @param loc_filepath
     */
    void replace_file(CFileInfo * file, const QString& loc_filepath);

    inline CFileInfo * get_root_folder(){return android_img.get_root_fileinfo();}
    inline CFileInfo * get_app_folder(){return app_folder;}
    inline CFileInfo * get_lib_folder(){return lib_folder;}
    inline CFileInfo * get_vendor_folder(){return vendor_folder;}
    inline CFileInfo * get_build_prop(){return build_prop;}
    inline CFileInfo * get_prive_app(){return priv_app;}

    u64 img_volume(){return android_img.get_img_volume();}
    u32 block_size(){return android_img.get_block_size();}
    u64 free_blocks(){return android_img.get_free_blocks_count();}

    CAndroidImage *get_android_img(){return &android_img;}

    /**
     * @brief add_dir_entries_from_local_path 将本地磁盘上 local_path中的所有的内容放在parent目录下
     * 即,如果要想app文件夹下添加apk或文件夹,则将所有的apk和文件夹先统一放在一个目录下
     * 如: D:/new ,然后将D:/new 传递给本函数, 其中 new 不参与创建,即,相当于将D:/new中的内容拷贝到 image的app目录下
     *
     * @param local_path
     * @param parent
     */
    void add_dir_entries_from_local_path(const QString& local_path,
                                         CFileInfo *parent);

    /**
     * @brief add_dir_entry_from_local_folder 在parent内以local_folder的文件夹名创建一个新目录,并将local_folder的内容
     * 添加在这个新创建的目录中
     * @param local_folder
     * @param parent
     * @return
     */
    CFileInfo* add_dir_entry_from_local_folder(const QString& local_folder,
                                               CFileInfo *parent);
    /**
     * @brief create_new_folder_in_dir 在目录dir中创建新的文件夹,并返回;
     *  如果dir中存在同名的文件夹,则直接返回原目录对应的CFileInfo且不会新建;
     *  如果存在同名的文件,则返回NULL
     * @param filename
     * @param dir
     * @return
     */
    CFileInfo *create_new_folder_in_dir(const QString& filename,
                                         CFileInfo *parent);

    /**
     * @brief create_new_file_in_dir 在dir中,添加新的文件,并返回,loc_filepath,是新文件在本地的绝度路径
     *  如果dir中存在同名的文件夹,则返回NULL;
     *  如果存在同名的文件,则返回原文件对应的CFileInfo且不会新建;由调用者确定是否替换文件的内容@see replace_file
     * @param loc_filepath
     * @param dir
     * @return
     */
    CFileInfo *create_new_file_in_dir(const QString& loc_filepath,
                                       CFileInfo *parent);

    CFileInfo *create_new_link_in_dir(const QString& filename,
                                      const QString& link_to,
                                      CFileInfo *dir);
private:

    void init_base_fileinfo();

    /**
     * @brief origin_dir_changed 登记被修改的原始目录
     * @param dir
     */
    void origin_dir_changed(CFileInfo *dir);

    /**
     * @brief delete_block
     * @param num
     */
    void delete_block(tBlockNum num);

    void delete_inode(tInodeNum num, bool isdir);


    bool is_block_used(tBlockNum num);
    void mark_block_used(tBlockNum num);
    /**
     * @brief allocate_continus_blocks 从image中分配连续的nblocks个块
     * @param nblocks
     * @return
     */
    tBlockNum allocate_continus_blocks(int nblocks);

    /**
     * @brief allocate_blocks 分配nblock个块,但不连续,各个段存在ret中
     * @param nblocks
     * @param ret
     */
    bool allocate_partial_blocks(int nblocks, tBlockRegionList &ret);


    /**
     * @brief do_allocate_inode_extent_contigous_block 分配连续的blocks个数据块给inode,
     *                  size只用作记录文件长度
     * @param inode
     * @param size 只用来更新inode中size的数据项
     * @param blocks
     * @return
     */
    tBlockNum do_allocate_inode_extent_contigous_block(ext4_inode * inode, u64 size);

    /**
     * @brief do_allocate_inode_extent_partial_block 分配不连续的blocks个数据块,size只用来记录文件大小
     * @param inode
     * @param size 只用来更新inode中size的数据项
     * @param blocks
     * @param ret_lst
     * @return
     */
    bool do_allocate_inode_extent_partial_block(ext4_inode *inode, u64 size,  tBlockRegionList &ret_lst);


    /**
     * @brief allocate_block_buffer 分配内存数据块,用于缓存变更的block的数据
     * @param blocks
     * @param block_size
     * @return
     */
    BlockMemoryBuffer * allocate_block_buffer(u32 blocks, u32 block_size = 4096);


    void clear_block_buffer_list();
private:


    //添加操作
    //
    void update_origin_dir_into_img(CFileInfo *dir);

    void update_origin_file_into_img(CFileInfo *file);

    bool add_new_dir_into_img(CFileInfo *dir);

    bool add_new_file_into_img(CFileInfo *file);

    /**
     * @brief get_dir_entry_size 获取dir的目录项的大小,及目录个数
     * @param len
     * @param dirs
     * @param dir
     */
    void get_dir_entry_size(u64 &len, u32& dirs, CFileInfo *dir);

    /**
     * @brief fill_dir_data_block 构建dir的目录项内容,填充dir的数据块
     * @param data
     * @param block_size
     * @param blocks
     * @param dir
     */
    void fill_dir_data_block(u8 *data, u32 block_size, u32 blocks, CFileInfo *dir);


    /**
     * @brief allocate_dir_blocks_and_fill_data 分别dir的数据块,并填充数据
     * @param inode
     * @param dir
     * @param size
     * @param blocks
     * @param block_size
     * @return
     */
    bool allocate_dir_blocks_and_fill_data(ext4_inode *inode,
                            CFileInfo *dir,
                            u64 size, u32 blocks,
                            u32 block_size = 4096);
    /**
     * @brief allocate_file_blocks_and_fill_data 分配file的数据块,并填充数据
     * @param inode
     * @param file_path
     * @param file_size
     * @param blocks
     * @param block_size
     * @return
     */
    bool allocate_file_blocks_and_fill_data(ext4_inode *inode,
                             const QString& file_path,
                             u64 file_size,
                             u32 blocks,
                             u32 block_size = 4096);
private:
    //原始被改变的目录
    //
    tFileInfoList origin_changed_dir_lst;
    tFileInfoList origin_changed_file_lst;

    /**
     * 所有新创建的CFileInfo有CImageManager负责创建和删除
     * img中原有的文件对应的CFileInfo有CAndroidImage负责创建和删除
     *
     */
    tFileInfoList new_added_dir_list;
    tFileInfoList new_added_file_list;
    tFileInfoList new_added_link_list;
    /**
     * @brief clear_new_fileinfo_lst 清空上述的三个链表
     */
    void clear_new_fileinfo_lst();


    //缓存新的block的数据块
    tBlockMemoryBufferLst block_buffer_lst;
    /**
     * @brief register_blockdata 用于保证preload_blocks中的内容是安装block num是升序
     * @param data
     */
    bool register_blockdata(CBlockData *data);


    void clear_new_block_data_list();

    /**
     * @brief m_mod_blockdata_ascend_lst 缓存所有的blockdata
     * 按从小到大顺序排列
     */
    tBlockDataList m_mod_blockdata_ascend_lst;

private:
    CAndroidImage android_img;
    // android_img中的blockgroupvec引用
    //
    const tBlockGroupVec&  block_group_vec;

    CFileInfo * app_folder;
    CFileInfo * lib_folder;
    CFileInfo * vendor_folder;
    CFileInfo * build_prop;
    CFileInfo * priv_app;

    // 文件夹,apk和so文件的inode模版,
    // 要更新的有两个地方,一是文件大小, 二是指向的block
    //
    enum{
        InodeSize = 256,
    };
    char folder_inode_example[InodeSize];
    char apk_inode_example[InodeSize];
    char lib_so_inode_example[InodeSize];

    bool bool_folder_inode_inited;
    bool bool_apk_inode_inited;
    bool bool_so_inode_inited;
public:
    void add_changed_inode(tInodeNum num);

private:
    /**
     * @brief prepare_removed_block_data
     * 设置将要删除的数据块的data_block
     */
    void prepare_removed_block_data();
    /**
     * @brief delete_new_added_file 删除新添加的文件
     * @param child
     */
    void delete_new_added_file(CFileInfo *fi);

    /**
     * @brief delete_origin_file 删除img中原有的文件
     * @param file
     */
    void delete_origin_file(CFileInfo * fi);

    //最终写回
    //
    bool prepare_update_data();

    /**
     * @brief complement_group_new_data_block
     * 根据各个块组的情况补全各自的new_data block
     */
    void complement_group_new_data_block();


    /**
     * @brief build_modified_block_list 调用者负责释放list中的指针,
     * 将CBlockData转换为一个一个tModBlock并
     * 添加到lst后面
     * @param data
     * @param lst
     */
    void build_modified_block_list(tModBlkLst& lst);

    /**
     * @brief process_origin_dir_removed_entries 处理原dir中删除的项
     * @param dir
     */
    void process_origin_dir_removed_entries(CFileInfo *dir);

    bool flush_data_into_ext4_img(const QString& new_ext);
    bool flush_data_into_sparse_img(const QString& sparse_fn);
    u32 write_multi_chunks(QFile *news,
                                 QFile *orgs,
                                 const QList<CChunk*>& chk_lst,
                                 const tModBlkLst& all_mod_lst);
    void write_origin_chunk(QFile *news,
                            QFile *orgs,
                            CChunk *chk);

    u32  write_modified_chunk(QFile *news,
                    QFile *orgs,
                    CChunk *chk,
                    const tModBlkLst& mod_lst);
    u32  write_modified_chunk_normal(QFile *news,
                    QFile *orgs,
                    CChunk *chk,
                    const tModBlkLst& mod_lst);
    u32  write_modified_chunk_samsung(QFile *news,
                    QFile *orgs,
                     CChunk *chk,
                    const tModBlkLst& mod_lst);
    u32  write_modified_chunk_marvell(QFile *news,
                    QFile *orgs,
                    CChunk *chk,
                    const tModBlkLst& mod_lst);
private:
    friend class CImageHandlerExt;
    void set_image_handler_ext(CImageHandlerExt *ext){m_pack_hdl_ext = ext;}
    CImageHandlerExt *m_pack_hdl_ext;
};


#endif // CIMAGEMANAGER_H
