#ifndef ANDROID_CAPABILITY_H
#define ANDROID_CAPABILITY_H

#include "mogu.h"

#define VFS_CAP_REVISION_MASK 0xFF000000
#define VFS_CAP_REVISION_SHIFT 24
#define VFS_CAP_FLAGS_MASK ~VFS_CAP_REVISION_MASK
#define VFS_CAP_FLAGS_EFFECTIVE 0x000001
#define VFS_CAP_REVISION_1 0x01000000
#define VFS_CAP_U32_1 1
#define XATTR_CAPS_SZ_1 (sizeof(__le32)*(1 + 2*VFS_CAP_U32_1))
#define VFS_CAP_REVISION_2 0x02000000
#define VFS_CAP_U32_2 2
#define XATTR_CAPS_SZ_2 (sizeof(__le32)*(1 + 2*VFS_CAP_U32_2))
#define XATTR_CAPS_SZ XATTR_CAPS_SZ_2
#define VFS_CAP_U32 VFS_CAP_U32_2
#define VFS_CAP_REVISION VFS_CAP_REVISION_2

struct vfs_cap_data {
 __le32 magic_etc;
 struct {
 __le32 permitted;
 __le32 inheritable;
 } data[VFS_CAP_U32];
};

#endif // ANDROID_CAPABILITY_H
