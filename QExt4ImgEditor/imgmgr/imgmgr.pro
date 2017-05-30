QT          += core
QT          -= gui
TARGET      = imgr
TEMPLATE    = app

INCLUDEPATH += ./Log

HEADERS += \
    android_capability.h \
    CAndroidImage.h \
    CAndroidImageExt.h \
    CBlockData.h \
    CBlockGroup.h \
    CChunk.h \
    CFileInfo.h \
    CImageManager.h \
    CMoGuUtilSimp.h \
    CSparseFile.h \
    ext4.h \
    ext4_extents.h \
    ext4_utils.h \
    mogu.h \
    readme.h \
    sparse_format.h \
    typedefs.h \
    unpack_ext4fs.h \
    xattr.h \
    Log/Logger.h

SOURCES += \
    CAndroidImage.cpp \
    CAndroidImageExt.cpp \
    CBlockData.cpp \
    CBlockGroup.cpp \
    CChunk.cpp \
    CFileInfo.cpp \
    CImageManager.cpp \
    CMoGuUtilSimp.cpp \
    CSparseFile.cpp \
    CSparseFile_Block.cpp \
    CSparseFile_Converter.cpp \
    xattr.cpp \
    Log/Logger.cpp \
    main.cpp \
    CAndroidImage_ExtUtil.cpp \
    CAndroidImage_FileInfoUtil.cpp \
    CAndroidImage_IOUtil.cpp \
    CAndroidImage_LoadChunk.cpp \
    CAndroidImage_Obsolete.cpp \
    CAndroidImage_SparseUtil.cpp \
    CAndroidImage_Static.cpp \
    CImageManager_BlockInodeMgr.cpp \
    CImageManager_DataBufferMgr.cpp \
    CImageManager_FileInfoMgr.cpp \
    CImageManager_FlushoBlockMgr.cpp \
    CImageManager_PrepareData.cpp \
    CImageManager_WriteBack.cpp \
    CImageManager_WriteModifiedChunk.cpp
