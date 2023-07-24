#include "Logging.hh"

namespace hactar_utils
{


// Static
LogManager* LogManager::instance = nullptr;

LogManager* LogManager::GetInstance()
{
    /**
     * This is a safer way to create an instance. instance = new Singleton is
     * dangeruous in case two instance threads wants to access at the same time
     */
    if (instance == nullptr)
    {
        instance = new LogManager();
    }
    return instance;
}

void LogManager::add_logger(LogHandler* handler)
{
    if (handler)
    {
        handlers.push_back(handler);
    }
}

void LogManager::remove_logger(LogHandler* handler)
{
    if (handler)
    {
        handlers.remove(handler);
    }
}

void LogManager::info(const char* tag, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(tag, LogLevel::Info, fmt, args);
    va_end(args);
}

void LogManager::verbose(const char* tag, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(tag, LogLevel::Verbose, fmt, args);
    va_end(args);
}

void LogManager::debug(const char* tag, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(tag, LogLevel::Debug, fmt, args);
    va_end(args);
}


void LogManager::warn(const char* tag, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(tag, LogLevel::Warn, fmt, args);
    va_end(args);
}

void LogManager::error(const char* tag, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_message(tag, LogLevel::Error, fmt, args);
    va_end(args);
}

///
/// private
///

void LogManager::log_message(const char* tag, const LogLevel level, const char* fmt, va_list args)
{
    for (const auto& handler : handlers)
    {
        handler->log(tag, level, fmt, args);
    }
}

}