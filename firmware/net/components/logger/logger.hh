#pragma once

#include "esp_log.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#ifndef NET_LOGGER_ACTIVE_LEVEL
#define NET_LOGGER_ACTIVE_LEVEL NET_LOGGING_DEBUG
#endif

#define NET_LOGGING_OFF 0
#define NET_LOGGING_ERROR 1
#define NET_LOGGING_WARN 2
#define NET_LOGGING_INFO 3
#define NET_LOGGING_DEBUG 4

#define NET_LOG(level, format, ...) Logger::Log(level, format __VA_OPT__(,) __VA_ARGS__)
#if NET_LOGGER_ACTIVE_LEVEL >= NET_LOGGING_ERROR
#define NET_LOG_ERROR(format, ...) NET_LOG(Logger::Level::Error, format, __VA_ARGS__)
#else
#define NET_LOG_ERROR(...)
#endif

#if NET_LOGGER_ACTIVE_LEVEL >= NET_LOGGING_WARN
#define NET_LOG_WARN(format, ...) NET_LOG(Logger::Level::Warn, format, __VA_ARGS__)
#else
#define NET_LOG_WARN(...)
#endif

#if NET_LOGGER_ACTIVE_LEVEL >= NET_LOGGING_INFO
#define NET_LOG_INFO(format, ...) NET_LOG(Logger::Level::Info, format, __VA_ARGS__)
#else
#define NET_LOG_INFO(...)
#endif

#if NET_LOGGER_ACTIVE_LEVEL >= NET_LOGGING_DEBUG
#define NET_LOG_DEBUG(format, ...) NET_LOG(Logger::Level::Debug, format, __VA_ARGS__)
#else
#define NET_LOG_DEBUG(...)
#endif

constexpr int MAX_LOG_LENGTH = 128;

class Logger
{
static constexpr const char* TAG = "[NET]";

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
    try
    {
        char line[MAX_LOG_LENGTH];
        std::sprintf(line, format, args...);

        switch (level)
        {
            case Level::Error:
                ESP_LOGE(TAG, "%s", line);
                break;
            case Level::Warn:
                ESP_LOGW(TAG, "%s", line);
                break;
            case Level::Info:
                ESP_LOGI(TAG, "%s", line);
                break;
            case Level::Debug:
                ESP_LOGD(TAG, "%s", line);
                break;
        }
    }
    catch (const std::exception& e)
    {
        ESP_LOGE(TAG, "Caught exception while logging: %s", e.what());
    }

    static std::string to_hex(const uint8_t* data, size_t size)
    {
        std::ostringstream hex;
        hex.flags(std::ios::hex);

        for (size_t i = 0; i < size; ++i)
        {
            hex << std::setw(2) << std::setfill('0') << int(data[i]);
        }
        return hex.str();
    }
};
