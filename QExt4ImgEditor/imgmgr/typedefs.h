#ifndef TYPEDEFS_H
#define TYPEDEFS_H
//#include <sys/types.h>

typedef unsigned long long u64;
typedef signed long long s64;
typedef unsigned int u32;
typedef unsigned short int u16;
typedef unsigned char  u8;

#define __le64 u64
#define __le32 u32
#define __le16 u16

#define __be64 u64
#define __be32 u32
#define __be16 u16

#define __u64 u64
#define __u32 u32
#define __u16 u16
#define __u8 u8

#define UNIX_S_IFMT  00170000
#define UNIX_S_IFSOCK 0140000
#define UNIX_S_IFLNK  0120000
#define UNIX_S_IFREG  0100000
#define UNIX_S_IFBLK  0060000
#define UNIX_S_IFDIR  0040000
#define UNIX_S_IFCHR  0020000
#define UNIX_S_IFIFO  0010000
#define UNIX_S_ISUID  0004000
#define UNIX_S_ISGID  0002000
#define UNIX_S_ISVTX  0001000

#define UNIX_S_ISLNK(m)      (((m) & UNIX_S_IFMT) == UNIX_S_IFLNK)
#define UNIX_S_ISREG(m)      (((m) & UNIX_S_IFMT) == UNIX_S_IFREG)
#define UNIX_S_ISDIR(m)      (((m) & UNIX_S_IFMT) == UNIX_S_IFDIR)
#define UNIX_S_ISCHR(m)      (((m) & UNIX_S_IFMT) == UNIX_S_IFCHR)
#define UNIX_S_ISBLK(m)      (((m) & UNIX_S_IFMT) == UNIX_S_IFBLK)
#define UNIX_S_ISFIFO(m)     (((m) & UNIX_S_IFMT) == UNIX_S_IFIFO)
#define UNIX_S_ISSOCK(m)     (((m) & UNIX_S_IFMT) == UNIX_S_IFSOCK)

#define UNIX_S_IRWXU 00700
#define UNIX_S_IRUSR 00400
#define UNIX_S_IWUSR 00200
#define UNIX_S_IXUSR 00100
//#define UNIX_S_IRWXU (UNIX_S_IRUSR | UNIX_S_IWUSR | UNIX_S_IXUSR)

#define UNIX_S_IRWXG 00070
#define UNIX_S_IRGRP 00040
#define UNIX_S_IWGRP 00020
#define UNIX_S_IXGRP 00010
//#define UNIX_S_IRWXG (UNIX_S_IRGRP | UNIX_S_IWGRP | UNIX_S_IXGRP)

#define UNIX_S_IRWXO 00007
#define UNIX_S_IROTH 00004
#define UNIX_S_IWOTH 00002
#define UNIX_S_IXOTH 00001
//#define UNIX_S_IRWXO (UNIX_S_IROTH | UNIX_S_IWOTH | UNIX_S_IXOTH)

#define UNIX_S_ISUID 0004000
#define UNIX_S_ISGID 0002000
#define UNIX_S_ISVTX 0001000

#endif // TYPEDEFS_H
