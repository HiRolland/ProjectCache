#ifndef LOGGER_H
#define LOGGER_H

#include <QMutex>
#include <QStringList>
#include <QFile>
#include <QStringList>

#include <QDebug>

enum LogType{
    LogType_Warn,
    LogType_Info,
    LogType_Error,
};

class Logger
{
public:
    static Logger *instance();
    ~Logger();

    void log(char * fn, int lnnum, LogType type, char* logformat,...);
    void post_error_log();
private:
    Logger();


    QMutex log_mutex;
    QStringList log_cache;

    QString username;
};

#define ERROR(log_fmt, ...) \
    do{ \
        Logger::instance()->log(__FILE__, __LINE__, LogType_Error, log_fmt, __VA_ARGS__); \
      }while(0)

#define INFO(log_fmt, ...) \
    do{ \
        Logger::instance()->log(__FILE__, __LINE__,LogType_Info, log_fmt, __VA_ARGS__); \
      }while(0)

#define WARN(log_fmt, ...) \
    do{ \
        Logger::instance()->log(__FILE__, __LINE__,LogType_Warn, log_fmt, __VA_ARGS__); \
      }while(0)
#endif // LOGGER_H
