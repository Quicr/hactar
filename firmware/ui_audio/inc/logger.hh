#pragma once

#include "stm32.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <bitset>

extern UART_HandleTypeDef huart1;

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
    static void Log(Level level, const T&... args)
    {
        auto ss = std::stringstream();
        ss << "[UI][" << log_level_string(level) << "] ";
        (..., (ss << args << ' '));
        ss << std::endl;

        const auto line = ss.str();

        const auto* line_ptr = reinterpret_cast<const uint8_t*>(line.c_str());
        HAL_UART_Transmit(&huart1, line_ptr, line.size(), HAL_MAX_DELAY);
    }

    static std::string to_hex(const uint8_t* data, size_t size)
    {
        std::stringstream hex(std::ios_base::out);
        hex.flags(std::ios::hex);
        for (size_t i = 0; i < size; ++i)
        {
            hex << std::setw(2) << std::setfill('0') << int(data[i]);
        }
        return hex.str();
    }

    static std::string to_binary(const uint8_t* data, size_t size)
    {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');
        for (size_t i = 0; i < size; ++i)
        {
            ss << std::setw(2) << static_cast<int>(data[i]);
        }
        std::string binaryString = ss.str();

        // Convert hex string to binary string
        std::string result;
        for (size_t i = 0; i < binaryString.length(); i += 2)
        {
            std::string byteString = binaryString.substr(i, 2);
            int byte = std::stoi(byteString, nullptr, 16);
            result += std::bitset<8>(byte).to_string();
        }
        return result;
    }

private:
    static std::string log_level_string(Level level)
    {
        switch (level)
        {
            case Level::Error: return "ERRR";
            case Level::Warn:  return "WARN";
            case Level::Info:  return "INFO";
            case Level::Debug: return "DBUG";
        }

        return "UNKN";
    }
};