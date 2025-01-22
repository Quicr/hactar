#pragma once

#include "esp_log.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

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
    static void Log(Level level, const T&... args)
    {
        auto ss = std::ostringstream();
        (..., (ss << args << ' '));

        const auto line = ss.str();

        switch (level)
        {
            case Level::Error:
                ESP_LOGE(TAG, "%s", line.c_str());
                break;
            case Level::Warn:
                ESP_LOGW(TAG, "%s", line.c_str());
                break;
            case Level::Info:
                ESP_LOGI(TAG, "%s", line.c_str());
                break;
            case Level::Debug:
                ESP_LOGD(TAG, "%s", line.c_str());
                break;
        }
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
};
