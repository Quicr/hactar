#pragma once

#include <stdarg.h>
#include <list>
#include <mutex>

namespace hactar_utils
{

enum class LogLevel
{
    None = 0,
    Error,
    Warn,
    Info,
    Debug,
    Verbose
};

// Interface for generic loggers
class LogHandler
{
public:
    virtual void log(const char* tag, const LogLevel level, const char* fmt, va_list args) = 0;
};

// Singleton class for setting up log handlers
// TODO : move away from singleton, pass logger instead
class LogManager
{
public:
    LogManager(LogManager& other) = delete;
    void operator=(const LogManager& other) = delete;
    static LogManager* GetInstance();

    void add_logger(LogHandler* handler);
    void remove_logger(LogHandler* handler);

    void verbose(const char* tag, const char* fmt, ...);
    void debug(const char* tag, const char* fmt, ...);
    void info(const char* tag, const char* fmt, ...);
    void warn(const char* tag, const char* fmt, ...);
    void error(const char* tag, const char* fmt, ...);

protected:
    LogManager()
    {

    }

    ~LogManager()
    {

    }


private:
    void log_message(const char* tag, const LogLevel, const char* fmt, va_list args);
    static std::mutex mutex;
    static LogManager* instance;
    std::list<LogHandler*> handlers;
};

}