#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

class Logger
{
public:
    template <typename... T>
    static void Log(const T &...args)
    {
        auto str = std::stringstream();
        str << "[Net] ";
        auto line = space_separated_line(std::move(str), args...);

        // Uncomment for UART logging
        line += std::string("\n");
        printf(line.c_str());
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

private:
    static std::string space_separated_line(std::stringstream &&str)
    {
        return str.str();
    }

    template <typename T, typename... U>
    static std::string space_separated_line(std::stringstream &&str, const T &first, const U &...rest)
    {
        str << first << " ";
        return space_separated_line(std::move(str), rest...);
    }
};
