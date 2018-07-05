#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

#ifdef WIN32
#define __func__ __FUNCTION__
#endif

// This define must be before including "cpplog.hpp"
#define LOG_LEVEL(level, logger) CustomLogMessage(__FILE__, __func__, __LINE__, (level), logger).getStream()
#include "cpplog.h"

class CustomLogMessage : public cpplog::LogMessage
{
public:
    CustomLogMessage(const char* file, const char* function,
        unsigned int line, cpplog::loglevel_t logLevel,
        cpplog::BaseLogger &outputLogger)
        : cpplog::LogMessage(file, line, logLevel, outputLogger, false),
        m_function(function)
    {
        InitLogMessage();
    }

    static const char* shortLogLevelName(cpplog::loglevel_t logLevel)
    {
        switch( logLevel )
        {
        case LL_TRACE: return "T";
        case LL_DEBUG: return "D";
        case LL_INFO:  return "I";
        case LL_WARN:  return "W";
        case LL_ERROR: return "E";
        case LL_FATAL: return "F";
        default:       return "O";
        };
    }

protected:
    virtual void InitLogMessage()
    {
        char timeMsg[32];
#ifdef WIN32
        SYSTEMTIME lt;
        GetLocalTime(&lt);
        sprintf(timeMsg, "%04d/%02d/%02d %02d:%02d:%02d.%03d",
            lt.wYear,
            lt.wMonth,
            lt.wDay,
            lt.wHour,
            lt.wMinute,
            lt.wSecond,
            lt.wMilliseconds
            );
#else
        struct tm tmloc;
        cpplog::helpers::slocaltime(&tmloc, &m_logData->messageTime);

        sprintf(timeMsg, "%04d/%02d/%02d %02d:%02d:%02d",
            tmloc.tm_year + 1900,
            tmloc.tm_mon + 1,
            tmloc.tm_mday,
            tmloc.tm_hour,
            tmloc.tm_min,
            tmloc.tm_sec
            );
#endif
        m_logData->stream
            << "["
            << timeMsg
            << "]["
            << shortLogLevelName(m_logData->level)
            << "] ";
    }
private:
    const char *m_function;
};

extern cpplog::StdErrLogger slog;

#define Logger_Info()   LOG_INFO(slog)
#define Logger_Warn()   LOG_WARN(slog)
#define Logger_Err()    LOG_ERROR(slog)

#endif // LOGGER_H

