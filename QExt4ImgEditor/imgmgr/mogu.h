#ifndef MOGU_H
#define MOGU_H
#include <QFile>
#include <QString>
#include <QVector>
#include <QList>
#include "sparse_format.h"

#include "ext4.h"
#include "ext4_utils.h"
#include "ext4_extents.h"

#define EXT4_ALLOCATE_FAILED (u32)(~0)


/* start from 0*/
typedef quint64 tBlockNum;

/* start from 1*/
typedef u32     tInodeNum;
#define INVALID_INODE_NUM 0

/* start from 0*/
typedef u32 tBlockIndexInGroup;
/* start from 0*/
typedef u32 tInodeIndexInGroup;

typedef QVector<tBlockNum> tBlockNumArray;
typedef QVector<tInodeNum> tInodeNumArray;
typedef QVector<tBlockIndexInGroup> tBlockIndexInGroupArray;
typedef QVector<tInodeIndexInGroup> tInodeIndexInGroupArray;

typedef QList<tBlockNum> tBlockNumList;
typedef QList<tInodeNum> tInodeNumList;
typedef QList<tBlockIndexInGroup> tBlockIndexInGroupList;
typedef QList<tInodeIndexInGroup> tInodeIndexInGroupList;



#endif // MOGU_H
