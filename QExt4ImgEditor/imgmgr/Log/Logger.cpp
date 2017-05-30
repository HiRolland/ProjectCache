#include "Logger.h"
#include <QDateTime>
#include <QTextStream>
#include <QDebug>
#include "CProgramDataManager.h"
#include <Windows.h>
#include "CVersion.h"
#include "CCurlHttpHelper.h"
#include <QSysInfo>
#include "CCurlInitilizer.h"
Logger::Logger()
{
    CCurlInitilizer::CallMeOneTime();
}

Logger *Logger::instance()
{
    static Logger logger;
    return &logger;
}

Logger::~Logger()
{
}

void Logger::log(char *fn, int lnnum, LogType type, char *logformat,...)
{
#define BUF_LEN  512

    char buf[BUF_LEN] = {0};
    int _size;
    va_list args;
    va_start(args, logformat);
    _size = vsnprintf(buf, BUF_LEN, logformat, args);
    va_end(args);
    QString prefix;
    switch(type)
    {
    case LogType_Error:
        prefix = QString("ERROR: %1 %2 :").arg(fn).arg(lnnum);
        break;
    case LogType_Warn:
        prefix = QString("WARN:%1 %2:").arg(fn).arg(lnnum);
        break;
    case LogType_Info:
        prefix = QString("INFO:%1 %2:").arg(fn).arg(lnnum);
        break;
    }

#ifdef _DEBUG
    qDebug() << prefix << QString(buf).trimmed();
#else

    log_cache.append(prefix + QString(buf));
    if(log_cache.size() > 10)
    {
        log_cache.removeFirst();
    }
#endif

}

#define POST_URL QString("http://www.yzmg.com/")
void Logger::post_error_log()
{
#ifdef _DEBUG

#else
    if(CProgramDataManager::Instance()->HasLogined())
    {
        username = CProgramDataManager::Instance()->GetLoginUserName();
    }
    else{
        DWORD size=0;
        GetUserName(NULL,&size);
        wchar_t *name=new wchar_t[size];
        if(GetUserName(name,&size))
        {
            username = QString::fromStdWString(name);
        }
        delete [] name;
    }

    tKeyValueMap map;
    map.insert(QString("username"),username);
    map.insert(QString("version"), VERSION_STR);
    map.insert(QString("errlog"),log_cache.join(";"));

    CCurlHttpHelper().HttpPost(POST_URL + QString("zhushou/errlog.php/Index/post"), map);
#endif
}
