#include "CMoGuUtilSimp.h"
#include <QFileInfo>
#include <QDir>

bool CMoGuUtilSimp::PathFileExsits(const QString &path)
{
    return  QFileInfo(path).exists();
}


void CMoGuUtilSimp::CreateFullPath(const QString &path)
{
    if(!PathFileExsits(path)){
        QDir().mkpath(path);
    }
}
