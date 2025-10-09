#pragma once

#include "stm32.h"
#include <string.h>
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

#if LOGGER_ACTIVE_LEVEL >= LOGGING_ERROR
#define UI_LOG_ERROR(format, ...) UI_LOG(Logger::Level::Error, format, __VA_ARGS__)
#else
#define UI_LOG_ERROR(...)
#endif

#if LOGGER_ACTIVE_LEVEL >= LOGGING_WARN
#define UI_LOG_WARN(format, ...) UI_LOG(Logger::Level::Warn, format, __VA_ARGS__)
#else
#define UI_LOG_WARN(...)
#endif

#if LOGGER_ACTIVE_LEVEL >= LOGGING_INFO
#define UI_LOG_INFO(format, ...) UI_LOG(Logger::Level::Info, format, __VA_ARGS__)
#else
#define UI_LOG_INFO(...)
#endif

#if LOGGER_ACTIVE_LEVEL >= LOGGING_DEBUG
#define UI_LOG_DEBUG(format, ...) UI_LOG(Logger::Level::Debug, format, __VA_ARGS__)
#else
#define UI_LOG_DEBUG(...)
#endif

#if LOGGER_ACTIVE_LEVEL >= LOGGING_DEBUG
#define UI_LOG_RAW(format, ...) UI_LOG(Logger::Level::Raw, format, __VA_ARGS__)
#else
#define UI_LOG_RAW(...)
#endif

extern UART_HandleTypeDef huart1;
constexpr int MAX_LOG_LENGTH = 256;
constexpr int Prefix_Len = 8;
class Logger
{
public:
    static inline bool enabled = true;

    enum class Level
    {
        Error,
        Warn,
        Info,
        Debug,
        Raw
    };

#ifdef STM32F405xx
    template <typename... T>
    static void Log(Level level, const char* format, const T&... args)
    {
        if (!enabled)
        {
            return;
        }

        static char log_line[MAX_LOG_LENGTH] = {0};
        const int line_size = std::sprintf(log_line, format, args...);

        // If raw don't add any additional text.
        switch (level)
        {
        case Level::Raw:
        {
            break;
        }
        default:
        {
            HAL_UART_Transmit(&huart1, (const uint8_t*)log_level_string(level), Prefix_Len,
                              HAL_MAX_DELAY);
            break;
        }
        }
        HAL_UART_Transmit(&huart1, (const uint8_t*)log_line, line_size, HAL_MAX_DELAY);
        HAL_UART_Transmit(&huart1, (const uint8_t*)"\n", 1, HAL_MAX_DELAY);
    }
#endif

    static void Enable()
    {
        enabled = true;
    }

    static void Disable()
    {
        enabled = false;
    }

    static void Toggle()
    {
        enabled = !enabled;
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
    static const char* log_level_string(Level level)
    {
        switch (level)
        {
        case Level::Error:
            return "[UI-E] ";
        case Level::Warn:
            return "[UI-W] ";
        case Level::Info:
            return "[UI-I] ";
        case Level::Debug:
            return "[UI-D] ";
        default:
            break;
        }

        return "[UI-U] ";
    }
};