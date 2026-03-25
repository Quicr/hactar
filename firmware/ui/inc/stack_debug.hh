#pragma once

#include <cstdint>

// Linker-defined symbols
extern "C" uint32_t _estack; // Top of stack (end of RAM)
extern "C" uint32_t _ebss;   // End of BSS (approximate stack bottom)

namespace stack_debug
{

// Paint pattern used to detect stack high water mark
constexpr uint32_t Paint_Pattern = 0xC0C0C0C0;

struct StackInfo
{
    uint32_t stack_base; // Bottom of stack region
    uint32_t stack_top;  // Top of stack (highest address)
    uint32_t stack_size; // Total stack size
    uint32_t stack_used; // High water mark (deepest usage)
};

// Get current stack pointer
inline uint32_t GetSP()
{
    uint32_t sp;
    __asm volatile("mov %0, sp" : "=r"(sp));
    return sp;
}

// Paint unused stack memory with pattern for high water mark detection
inline void RepaintStack()
{
    uint32_t* ptr = &_ebss;
    uint32_t sp = GetSP();

    // Paint from end of BSS up to current SP (leaving some margin)
    while ((uint32_t)ptr < (sp - 64))
    {
        *ptr++ = Paint_Pattern;
    }
}

// Find high water mark by scanning for first non-painted word
inline uint32_t GetHighWaterMark()
{
    uint32_t* ptr = &_ebss;

    while (*ptr == Paint_Pattern && (uint32_t)ptr < (uint32_t)&_estack)
    {
        ptr++;
    }

    return (uint32_t)ptr;
}

// Get stack usage information
inline StackInfo GetStackInfo()
{
    StackInfo info;
    info.stack_base = (uint32_t)&_ebss;
    info.stack_top = (uint32_t)&_estack;
    info.stack_size = info.stack_top - info.stack_base;

    uint32_t high_water = GetHighWaterMark();
    info.stack_used = info.stack_top - high_water;

    return info;
}

} // namespace stack_debug
