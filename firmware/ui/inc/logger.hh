#pragma once

#include "stm32.h"
#include <iomanip>
#include <sstream>
#include <string>

#ifndef UI_LOGGER_ACTIVE_LEVEL
#define UI_LOGGER_ACTIVE_LEVEL UI_LOGGING_DEBUG
#endif

#define UI_LOGGING_OFF 0
#define UI_LOGGING_ERROR 1
#define UI_LOGGING_WARN 2
#define UI_LOGGING_INFO 3
#define UI_LOGGING_DEBUG 4

#define UI_LOG(level, format, ...) Logger::Log(level, format __VA_OPT__(, ) __VA_ARGS__)
#if UI_LOGGER_ACTIVE_LEVEL >= UI_LOGGING_ERROR
#define UI_LOG_ERROR(format, ...) UI_LOG(Logger::Level::Error, format, __VA_ARGS__)
#else
#define UI_LOG_ERROR(...)
#endif

#if UI_LOGGER_ACTIVE_LEVEL >= UI_LOGGING_WARN
#define UI_LOG_WARN(format, ...) UI_LOG(Logger::Level::Warn, format, __VA_ARGS__)
#else
#define UI_LOG_WARN(...)
#endif

#if UI_LOGGER_ACTIVE_LEVEL >= UI_LOGGING_INFO
#define UI_LOG_INFO(format, ...) UI_LOG(Logger::Level::Info, format, __VA_ARGS__)
#else
#define UI_LOG_INFO(...)
#endif

#if UI_LOGGER_ACTIVE_LEVEL >= UI_LOGGING_DEBUG
#define UI_LOG_DEBUG(format, ...) UI_LOG(Logger::Level::Debug, format, __VA_ARGS__)
#else
#define UI_LOG_DEBUG(...)
#endif

extern UART_HandleTypeDef huart1;
constexpr int MAX_LOG_LENGTH = 128;

class Logger
{
public:
    enum class Level
    {
        Error,
        Warn,
        Info,
        Debug,
    };

    template <typename... T>
    static void Log(Level level, const char* format, const T&... args)
    {
        static char log_line[MAX_LOG_LENGTH] = {0};
        std::sprintf(log_line, format, args...);

        char line[MAX_LOG_LENGTH + 8];
        const auto line_size =
            std::sprintf(line, "[UI-%s] %s\n", log_level_string(level).c_str(), log_line);

        const auto* line_ptr = reinterpret_cast<const uint8_t*>(line);
        HAL_UART_Transmit(&huart1, line_ptr, line_size, HAL_MAX_DELAY);
    }

    static std::string to_hex(const uint8_t* data, size_t size)
    {
        std::ostringstream hex(std::ios_base::out);
        hex.flags(std::ios::hex);
        for (size_t i = 0; i < size; ++i)
        {
            hex << std::setw(2) << std::setfill('0') << int(data[i]);
        }
        return hex.str();
    }

private:
    static std::string log_level_string(Level level)
    {
        switch (level)
        {
        case Level::Error:
            return "E";
        case Level::Warn:
            return "W";
        case Level::Info:
            return "I";
        case Level::Debug:
            return "D";
        }

        return "UNKN";
    }
};