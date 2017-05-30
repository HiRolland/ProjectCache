#ifndef CANDROIDIMAGE_H
#define CANDROIDIMAGE_H

#include "mogu.h"
#include "CBlockGroup.h"
enum tImgType{
    tImgType_unkown                 = 0x00,
    tImgType_ext4                   = 0x01,
    tImgType_sparse_normal          = 0x02,
    tImgType_sparse_marvell         = 0x03,
    tImgType_sparse_samsung         = 0x04,
    tImgType_sparse_cpd_normal      = 0x05,
    tImgType_sparse_cpd_marvell     = 0x06,
    tImgType_sparse_cpd_samsung     = 0x07,
    tImgType_sparse_signed          = 0x08,
    tImgType_yaffs2                 = 0x09,
    tImgType_ubifs                  = 0x0a,
};


struct chunk_index {
    __le16	chunk_type;		/* 0xCAC1 -> raw; 0xCAC2 -> fill; 0xCAC3 -> don't care */
    __le32	chunk_size;		/* in bytes in output image */
    __le64	start_bytes;	/* start offset in the output image */
    __le64	data_offset;	/* offset in the input image*/
};

struct tImageInfo
{
    tImgType type;
    int     is_sparse; // 0 is non-sparse else yes
    //    __le16	file_hdr_sz;	/* 28 bytes for first revision of the file format */
    //    __le16	chunk_hdr_sz;

};


/**
 * ext4 的img说明:
 * android的img中，ext_ext2_group_desc描述组和super_block 是稀疏模式放,
 * 即：在每个group的开头二者要么同时放,要么同时不放;
 * 不放的话,开头就是block_bitmap和inode_bitmap
 *
 */

/**
 * 项目说明:
 * 目前,对应system.img我们只需要记录两类文件夹的内容: lib 和 app, priv-app
 *
 */

struct sparse_hdr
{
//        tImgType_sparse_normal          = 0x02,
//        tImgType_sparse_marvell         = 0x03,
//        tImgType_sparse_samsung         = 0x04,
    tImgType type;//
    union
    {
        normal_sparse_header_t n_hdr;
        samsung_sparse_header_t s_hdr;
        marvell_sparse_header_t m_hdr;
    };
    int pading_len;
};

class QTextStream;
class CImageManager;
class CFileInfo;
class CBlockGroup;
class CChunk;
class CAndroidImage
{
    friend class CImageManager;
    friend class CBlockGroup;
    friend class CFileInfo;
public:
    ~CAndroidImage();
    CAndroidImage();

public:
    static bool get_img_sparse_info_bytes(const QString& img_name,
                                          struct tImageInfo& info,
                                          qint64& free_bytes,
                                          qint64& total_bytes);
    static qint64 get_img_free_bytes(const QString& img_name);
    static bool get_sparse_info(const QString& img_name, struct tImageInfo& info);
    static int get_sparse_header_offset(const QString&img_name);
    static int get_sigend_img_hdr_offset(const QString& img_name);
    static void test_signed_img(const QString& imgname);
public:
    bool set_image_filename(const QString& name);

    bool mount();

    bool umount();

    /**
     * @brief get_root_folder 获取根目录下某个文件夹的所有内容
     * @param foldername
     * @return
     */
    CFileInfo *get_root_dir_file(const QString& filename);

    inline const tBlockGroupVec & get_group_vec(){return block_group_vec;}

    inline bool is_mnt(){return bool_mnt;}

    u32 get_free_blocks_count(){return s_real_free_blocks;}

    CFileInfo* get_root_fileinfo(){return root_fileinfo;}

    /**
     * @brief is_journal_block 判断是否是日志块
     * @param num
     * @return
     */
    bool is_journal_block(tBlockNum num);

    bool is_sparse_image(){return bool_is_sparse;}
private:
    /**
     * @brief build_folder_entry_info 填充dir的目录项的fileinfo. recurise为true,则递归初始化
     *
     * @param dir 的is_new必须为false,否则不做任何动作
     */
    void build_dir_entry_fileinfo(CFileInfo *dir);

    void build_dir_entry_fileinfo(char *block_data, CFileInfo *dir);
private:
    /***************************
     *  for read
     *
     **************************/
//    bool cache_normal_sparse();
//    bool cache_marvell_sparse();
//    bool cache_samsung_sparse();


//    /**
//     * @brief cache_signed_sparse 对于签名的信息，在读取数据时,不需要再加偏移,这里已经记录了
//     * @return
//     */
//    bool cache_signed_sparse();

    static bool seek_to_sparse_hdr(QFile& in);
    static bool seek_to_chunk_hdr(QFile& in, int chk_hdr_sz, int chk_blk_sz);
    /**
     * @brief read_chunk_data 从第sparse_chunk_array中的offset_blocks块偏移地址处的chunk开读取 nblocks
     * @param offset_blocks
     * @param n_blocks
     * @param block_size
     */
    qint64 read_chunk_data(char *ptr, tBlockNum offset_blocks, int n_blocks, int block_size);

    /**
     * @brief read_disk 从offset_blocks块开始读去 n_blocks 块数据到 ptr中
     * @param ptr
     * @param offset_blocks
     * @param n_blocks
     * @param block_size 块的大小,同时用作文件偏移数单位
     * @return
     */
    qint64 read_disk( char *ptr, tBlockNum offset_blocks, int n_blocks,int block_size);


    /**
     * @brief write_disk 从offset_blocks开始写入 data数据
     *
     * @param data
     * @param data_len
     * @param offset_blocks
     * @param block_size
     * @return
     */
    bool write_disk(char * data, qint64 data_len, tBlockNum offset_blocks, int block_size);

    /**
     * @brief write_disk 从offset_blocks块的第 offset_on_block位置开始开始写入 data数据
     * @param data
     * @param data_len
     * @param offset_blocks
     * @param offset_on_block
     * @param block_size
     */
    bool write_disk(char * data, int data_len, tBlockNum offset_blocks, int offset_on_block, int block_size);
    /**
     * @brief read_disk 从offset_blocks开始的位置的 n_blocks个块 清0
     * @param ptr
     * @param offset_blocks
     * @param n_blocks
     * @param block_size 块的大小,同时用作文件偏移数单位
     * @return
     */
    bool clear_disk(tBlockNum offset_blocks, int n_blocks,int block_size);


    /**
     * @brief read_inode the caller has the respondbility to release the memery space
     * @param inode_num
     * @return
     */
    struct ext4_inode *read_inode(tInodeNum inode_num);

    /**
     * @brief read_inode_ext
     * @param inode_num
     * @param inode_buffer
     * @param inode_buffer_size
     * @return
     */
    bool read_inode(tInodeNum inode_num, struct ext4_inode *inode_buffer);

    /**Ext4 Helpers **/
    /**
     * @brief extent_binarysearch
     * @param header
     * @param lbn 块号
     * @param isallocated
     * @return
     */
    qint64 extent_binarysearch(struct ext4_extent_header *header, u64 lbn, int isallocated);

    /**
     * @brief extent_to_logical 通过extents方式找到想要的块号
     * @param ino
     * @param lbn 逻辑块号索引 0.1.2 ...即 inode的第几个block
     * @return
     */
    tBlockNum extent_to_logical(struct ext4_inode *ino, u64 lbn);

    /**
     * @brief fileblock_to_logical 取inode指定的第lbn块的数据, 通过传统的ext3的三级索引的方式
     * @param ino
     * @param lbn 逻辑块号索引 0.1.2 ...即 inode的第几个block
     * @return
     */
    tBlockNum fileblock_to_logical(struct ext4_inode *ino, u32 lbn);

    /**
     * @brief read_data_block 通过inode找到这个文件的block_num的块并将此块的信息读取出来
     * @param inode
     * @param block_num
     * @param buf
     * @return
     */
    int read_data_block(struct ext4_inode *inode, tBlockNum block_num, char * buf);

    /**
     * @brief read_selinux_from_inode_directly 读取selinux属性,
     * @param inode
     * @param buffer
     */
    void read_selinux_from_inode_directly(struct ext4_inode *inode, char * buffer);
    /**
     * @brief read_selinux_from_inode_block 读取selinux属性,inode中不一定能放完整
     * @param inode
     * @param buffer
     */
    void read_selinux_from_inode_block(struct ext4_inode *inode, char * buffer);

    /**
     * @brief read_selinux get selinux wrapper
     * @param inode
     */
    void read_selinux(struct ext4_inode *inode, char *buf);


    struct ext4_super_block* get_super_block(){return &super_block;}
    struct ext2_group_desc* get_group_desc_data(){return block_group_desc_data;}

    ext4_inode * read_inode_from_group(tInodeNum num);
private:
    tBlockNum get_physic_block_num(struct ext4_inode *inode, tBlockNum block_num);

    //
    //使用低位足够了,不应关心高32位
    //
    void allocate_blocks(int blocks);//{super_block.s_free_blocks_count_lo -= blocks;}
    void release_blocks(int blocks);//{super_block.s_free_blocks_count_lo += blocks;}

    void allocate_inodes(int inodes);//{super_block.s_free_inodes_count -= inodes;}
    void release_inodes(int inodes);//{super_block.s_free_inodes_count += inodes;}

    void init();

    //bool _prepare_mnt();
    bool _mnt();

    bool detect_layout_ext();
private:
    tImageInfo img_info;
    QString             image_file_name;
    QFile               img_file_handler;

    bool                bool_has_xattr;
    bool                bool_is_sparse;
    bool                bool_mnt;
    bool                bool_img_ready;

    //保存所有的group信息
    struct ext4_super_block     super_block;
    struct ext2_group_desc     *block_group_desc_data;
    tBlockGroupVec              block_group_vec;

    /**
     * @brief file_info_list 所有获取的原始的文件对应的CFileInfo均存贮在这里,其他地方只需引用即可
     *
     * 所有新创建的CFileInfo有CImageManager负责创建和删除
     * img中原有的文件对应的CFileInfo有CAndroidImage负责创建和删除
     *
     */
    QList<CFileInfo* >          file_info_list;


    CFileInfo *create_fileinfo(u8 file_type,
                               tInodeNum inode_num,
                               const QString& filename,
                               CFileInfo * parent);

    /**
     * @brief get_root_file_list 获取根目录下的所有文件的文件名和对应的inode号
     */
    void get_root_file_list();

    CFileInfo * root_fileinfo;

    //added new 2016/10/25
private:
    bool load_normal_chunk(QFile *f);
    bool load_samsung_chunk(QFile *f);
    bool load_marvell_chunk(QFile *f);

    CChunk *find_chunk_on_blocknum(int blk_num);

    void init_sparse_info();
    void clear_sparse_info();

    void detect_jnl_block_info(tBlockNum num);
private:
    //for sparse
    bool is_normal_sparse(){return img_info.type == tImgType_sparse_normal;}
    bool is_samsung_sparse(){return img_info.type == tImgType_sparse_samsung;}
    bool is_marvell_sparse(){return img_info.type == tImgType_sparse_marvell;}
    sparse_hdr* get_sparse_hdr(){return &m_sparse_hdr;}

    QFile *file_handler(){return &img_file_handler;}
    const QList<CChunk*>& get_chunk_list(){return m_chk_lst;}
private:
    CChunk *m_search_cache;
    bool m_b_crc32;
    QList<CChunk*> m_chk_lst;

    sparse_hdr m_sparse_hdr;
    qint64 m_jnl_start_block;
    qint64 m_jnl_block_cnt;
    qint64 m_jnl_size;
    bool m_b_has_jnl;
public :
    inline u64 get_img_volume(){return s_img_len;}

    inline u32 get_inode_size(){return s_inode_size;}
    inline u32 get_block_size(){return s_block_size;}

    inline u64 get_total_blocks(){return s_total_blocks;}
    inline u32 get_total_groups(){return s_total_groups;}
    inline u32 get_total_inodes(){return s_total_inodes;}
    inline u32 get_journal_blocks(){return s_journal_blocks;}

    /**
     * @brief get_bg_desc_blocks 这个数值有时不准确
     * @return
     */
    inline u32 get_bg_desc_blocks() {return s_bg_desc_blocks;}
    inline u32 get_inodes_per_group(){return s_inodes_per_group;}
    inline u32 get_blocks_per_group(){return s_blocks_per_group;}
    inline u32 get_inode_table_blocks(){return s_inode_table_blocks;}

    inline u32 get_bg_desc_reserve_blocks(){return s_bg_desc_reserve_blocks;}
private:
    qint64 m_file_size;

    u64 s_img_len;

    u32 s_inode_size;
    u32 s_block_size;

    u64 s_total_blocks;
    u32 s_total_groups;
    u32 s_total_inodes;
    u32 s_journal_blocks;

    u32 s_bg_desc_blocks;
    u32 s_inodes_per_group;
    u32 s_blocks_per_group;
    u32 s_inode_table_blocks;

    u16 s_feat_ro_compat;
    u16 s_feat_compat;
    u16 s_feat_incompat;
    u32 s_bg_desc_reserve_blocks;

    qint64 s_real_free_blocks;
private:
    static  u64 ext_to_block(struct ext4_extent *extent)
    {
        u64 block;

        block = (u64)extent->ee_start_lo;
        block |= ((u64) extent->ee_start_hi << 31) << 1;

        return block;
    }


    void test();
};
#define RELEASE_POINTER(ptr) do{ if(ptr) free(ptr); ptr = NULL;}while(0)


#endif // CANDROIDIMAGE_H
