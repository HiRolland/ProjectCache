#ifndef README
#define README
/**
  说明:用与处理android ext4文件系统的img
  对应sparse格式的img要先转换成raw格式的img

  img的格式是:
    1, 超级块|所有的组描述符号|
    2, block bitmap| inode bitmap | inode table |  blocks
   ext2 fs把硬盘按固定尺寸分为若干个block，固定个block组成一个group
   每个group都有上面的 2 内容
   Group组号的为0, 3N,5N,7N其中N=0,1,2,3的，group，每组开头是1标志的内容，接下来的部分是2

   //其中,只有group0的前1024bytes会保存,读写group0的时候,要注意偏移
   //除了读写group0的superblock时,将偏移量设置为1024,其他均正常设置偏移量,即:block_size


   //extent 树
   每个extent
    struct ext4_extent {
    //逻辑块号告诉我们，这个exten相对于文件t起始位置
    __le32  ee_block;   // extent覆盖的第一个数据块
    __le16  ee_len;     // extent覆盖的数据块的数目
    __le16  ee_start_hi; // 实际存储的物理数据块的高16位
    __le32  ee_start_lo;    // 物理数据块的低32位
    }
    每个extent的ee_len 是16bit,最多表示 2^15 * 4k = 128M(第16位用作标记),即,一个exent能表示128M大小的文件
    在inode里,最多有3个exent,即,inode中最多表示 128 *3 = 384M的文件

    再大,就需要构建extent树了,即,inode中的是extent树放3个 extent_index
    每个extent_index指向一个block,这个block存放extent,一个blokc上最多放340个extent,
    这样一个block的extent能表示的最大的文件是 340 * 2^15 * 4 k = 42 GB

    那1层的extent树就最多(inode 上三个extent_index都用上了)能表示 42 * 3= 126GB ~


    但对应我们的应用场景来说,不会超过300M的文件,即,牵扯不到


    //扩展属性
    当前的应用下,selinux + cap 共计不到100byte,inode的剩余空间足够存放
    10 + 7 + 25 + 8 = 68 字节

    //删除操作
    1,文件:
        a)将所有的 block 号在bitmap中设置为0
        b)相应的dir_entry清空
        c)将对应的inode清空
        d)情况对应的bitmap
    2,目录:
      a)统计其所有的子文件和目录的block,
        清空所有子文件和目录的inode
        重置对应的bitmap
        注意:不需要逐个子文件(目录)清理
      b)清除对应的dir_entry
      c)将对应的inode清空
      d)情况对应的bitmap

    //添加文件(文件夹)操作
    1,父目录调整:
        a),判断父目录的block上是否有足够的位置存放要增加的文件的dir_entry
        b),若不足够,则另申请 (原有块数 + 1)的块,拷贝原有的block数据到申请的block上
      然后释放原有的块,同时更新inode中的extent
    2,分别inode,并申请块号:注意根据我们的应用场景,新加文件大小会控制在100M以内,所以
    inode中的extent的高度设置为 0

*/
#include <QList>

struct super_block{
    qint64 s_start;//起始地址
    qint64 s_len;//长度
};


struct group_desc{

};

#endif // README

