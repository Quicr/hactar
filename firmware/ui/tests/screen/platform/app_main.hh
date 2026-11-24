#pragma once

#include <cstdlib>

// TileMap uses the firmware's fatal error hook for invalid construction.
[[noreturn]] inline void Error(const char*, const char*)
{
    std::abort();
}
