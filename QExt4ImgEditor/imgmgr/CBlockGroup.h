#ifndef CBLOCKGROUP_H
#define CBLOCKGROUP_H

#include "mogu.h"
#include "CBlockData.h"
#include <QList>
class CAndroidImage;

//
typedef struct _BlockRegion {
    u32 start_block; //start block num of this region
    u32 len; // block count of this region
}BlockRegion;

typedef QList<BlockRegion* > tBlockRegionList;

//
//group管理,提供底层操作
//
/**
 * @brief The CBlockGroup class CAndroidImage中的 block_group_desc逻辑表示
 * 里面数据的说明:
 * index:是在本group内的索引 从0开始
 * block_num和inode_num是全局的,要加以转换
 *
 * 注意:
 * 本类是类CAndroidImage中的数据成员 block_group_desc的逻辑表示,修改此类,将直接修改block_group_desc的内容
 *
 */

class CBlockGroup
{
    friend class CAndroidImage;
public:

    CBlockGroup(CAndroidImage* image, int groupindex);

    ~CBlockGroup();
    /**
     * @brief set_group_desc 初始化
     * @param desc
     */
    void set_group_desc(ext2_group_desc *desc);
    void fill_info();

    inline ext4_super_block* super_block(){return m_sb;}
    inline u8* super_block_data(){return m_super_block_buf;}
    inline u8* block_bitmap(){return m_block_bitmap;}
    inline u8* inode_bitmap(){return m_inode_bitmap;}
    inline u8* inode_table(){return m_inode_table;}
    inline tBlockNum block_bitmap_block_num(){return bg_desc->bg_block_bitmap; }
    inline tBlockNum inode_bitmap_block_num(){return bg_desc->bg_inode_bitmap;}
    inline tBlockNum inode_table_block_num(){return bg_desc->bg_inode_table;}


    inline void set_block_count(int cnt){m_block_count = cnt;}
    inline int block_count(){return m_block_count;}
    /**
     * @brief contains_block 本group内是否包含该块
     * @param block_num
     * @return
     */
    bool contains_block(tBlockNum block_num);

    /**
     * @brief contains_inode 本group内是否包含该inode
     * @param inode_num
     * @return
     */
    bool contains_inode(tInodeNum inode_num);

    /**
     * @brief clear_inode 若inode_num在该group内，则清空并返回true,否则不做任何动作且返回false
     * @param inode_num
     * @return
     */
    bool clear_inode(tInodeNum inode_num, bool isdir);

    /**
     * @brief clear_block block_num,否则不做任何动作且返回false
     * @param block_num
     * @return
     */
    bool clear_block(tBlockNum block_num);


    void clear();
    inline int  get_group_index(){return group_index;}
    inline bool has_changed(){return bool_blockbitmap_changed || bool_inodebitmap_changed;}
    inline bool has_super_block(){return bool_has_super_block;}
    inline bool block_changed(){return bool_blockbitmap_changed;}
    inline bool inode_changed(){return bool_inodebitmap_changed;}
    void display();

    inline const tInodeIndexInGroupArray& get_inode_changed_index(){return mod_inode_idxs;}

    /**
     * @brief allocate_contiguous_blocks 在本group中分配能容纳len长度的block.成功返回ture否则返回false
     * @param len
     * @param first_block_num 返回成功分配的第一个block_num(全局的block号)
     * @param n_blocks 返回块数
     * @return
     */
    bool allocate_contiguous_blocks(int len, tBlockNum &first_block_num, int &n_blocks);

    /**
     * @brief allcate_blocks 从本group中分配联系的n_blocks，成功则返回起始的block号(全局)否则返回0
     * @param n_blocks
     * @return
     */
    tBlockNum allocate_contiguous_blocks(int n_blocks);

    /**
     * @brief allocate_inode 分配一个'空的'inode;若成功则返回ext4_inode* 同时返回inode_num否则 返回NULL, inode_num 返回 0
     * @param inode_num
     * @return
     */
    ext4_inode*  allocate_inode(tInodeNum &inode_num, bool for_dir);


    ext4_inode*  get_inode(tInodeNum num);

    int free_inodes_count(){return bg_desc->bg_free_inodes_count ;}
    int free_blocks_count(){return free_blocks();}

    /**
     * @brief recal_checksum 最后要计算一下该值
     */
    void recal_checksum();


    //void add_new_blockdata(CBlockData *data);

    /**
     * @brief inode_num_2_block_num inode号所在的块号,如果inode_num对应的inode不在本group中,则返回0
     * @param inode_num
     * @return
     */
    tBlockNum inode_num_2_block_num(tInodeNum inode_num);

    /**
     * @brief inode_index_2_block_num 本group中第index的inode所在的块号
     * @param index
     * @return
     */
    tBlockNum inode_index_2_block_num(tInodeIndexInGroup index);


    /**
     * @brief allocate_fitted_contiguous_blocks 别本group中最大限度能满足need_blocks块数的 一段连续的块号
     * @param list
     * @param allocated
     * @param need_blocks 需求的块数
     * @return 实际分配的块数
     */
    u32 allocate_fitted_contiguous_blocks(tBlockRegionList & lst, u32 need_blocks);

    /**
     * @brief add_changed_inode 标记改变的inode
     * @param num
     * @return
     */
    bool add_changed_inode(tInodeNum num);

    /**
     * @brief prepare_released_blocks 获取要清空的block列表
     *
     */
    void get_released_blocks(tBlockNumList &lst);

    inline void set_has_jnl_block(bool has_jnl){bool_has_jnl_block = has_jnl;}
    inline bool has_jnl_block(){return bool_has_jnl_block;}

//    /**
//     * @brief update_into_image 将变更写回image,更新super block, desc, bitmaps, inode table;且清空要删除的block
//     * 对于新分配的block及其内容由 @class CBlockData记录,最终由CimageManager写回硬盘
//     */
//    void write_into_image();


    bool is_this_block_used(tBlockNum num);

    void mark_block_used(tBlockNum num);
private:
    CAndroidImage *h_img;

    void allocate_block_by_index(tInodeIndexInGroup index);
    void free_block_by_index(tInodeIndexInGroup index);

    void set_block_bitmap(tBlockIndexInGroup index, int value/*0 or 1*/);
    void set_inode_bitmap(tInodeIndexInGroup index, int value/*0 or 1*/);

    bool is_inode_used(tInodeIndexInGroup index);
    bool is_block_used(tBlockIndexInGroup index);

    bool is_bitmap_used(u8* bitmap, u32 bit);
    void set_bitmap(u8 *bitmap, u32 bit);
    void clear_bitmap(u8 *bitmap, u32 bit);

    /**
     * @brief inode_index_to_inode_num 将本group内的第index(从0计数)个inode的索引,即index转换成inode_num
     * @param index
     * @return
     */
    tInodeNum inode_index_to_inode_num(tInodeIndexInGroup index);

    /**
     * @brief block_index_to_block_num 将本group内的第index(从0计数)个block的索引,即index转换成block_num
     * @param index
     * @return
     */
    tBlockNum block_index_to_block_num(tBlockIndexInGroup index);

    /**
     * @brief inode_num_to_inode_index 将inode_num转换成本group内inode表的索引
     *      该函数不区分group
     * @param inode_num
     * @return
     */
    tInodeIndexInGroup inode_num_to_inode_index(tInodeNum inode_num);

    /**
     * @brief block_num_to_block_index 将block_num转换成本group内块的索引,若该block_num不在本group内,则返回 -1
     * @param block_num
     * @return
     */
    tBlockIndexInGroup block_num_to_block_index(tBlockNum block_num);

private:
    void prepare_inode_table();
    //由set_block_bitmap 和 set_inode_bitmap更新即可
    //bool bool_changed;
    //
    bool bool_blockbitmap_changed;
    bool bool_inodebitmap_changed;
    bool bool_has_super_block;
    bool bool_has_jnl_block;

    int group_index;
    struct ext2_group_desc  *bg_desc;
    int m_block_count;//最后一块不一定是默认值
    //must free
    u8 *bitmaps;
    u8 *m_inode_table;

    //don't free, point to bitmaps
    u8 *m_block_bitmap;
    u8 *m_inode_bitmap;


    tBlockIndexInGroupList new_del_blocks;
    tInodeIndexInGroupArray mod_inode_idxs;


    //是否保留1个快 ？？ by shentao
    //
    inline int free_blocks(){return bg_desc->bg_free_blocks_count > 1 ? (bg_desc->bg_free_blocks_count - 1) : 0;}
    inline void increase_free_blocks(){bg_desc->bg_free_blocks_count++;}
    inline void decrease_free_blocks(){bg_desc->bg_free_blocks_count--;}

    u8               *m_super_block_buf;
    ext4_super_block *m_sb;
    //u8* group_desc_buf;

};

typedef QVector<CBlockGroup* > tBlockGroupVec;
#endif // CBLOCKGROUP_H
