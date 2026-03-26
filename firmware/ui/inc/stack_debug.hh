#pragma once

#include <cstdint>
#include <unistd.h>

extern "C" uint32_t _estack;

namespace stack_debug
{

constexpr uint32_t Paint_Pattern = 0xC0C0C0C0;

struct StackInfo
{
    uint32_t stack_base;
    uint32_t stack_top;
    uint32_t stack_size;
    uint32_t stack_used;
};

inline uint32_t GetSP()
{
    uint32_t sp;
    __asm volatile("mov %0, sp" : "=r"(sp));
    return sp;
}

inline uint32_t GetHeapEnd()
{
    return (uint32_t)sbrk(0);
}

inline void RepaintStack()
{
    __disable_irq();

    uint32_t heap_end = GetHeapEnd();
    uint32_t* ptr = (uint32_t*)heap_end;
    uint32_t sp = GetSP();
    uint32_t limit = (sp > heap_end + 128) ? (sp - 64) : heap_end;

    while ((uint32_t)ptr < limit)
    {
        *ptr++ = Paint_Pattern;
    }

    __enable_irq();
}

inline uint32_t GetHighWaterMark()
{
    uint32_t heap_end = GetHeapEnd();
    uint32_t* ptr = (uint32_t*)heap_end;

    while (*ptr == Paint_Pattern && (uint32_t)ptr < (uint32_t)&_estack)
    {
        ptr++;
    }

    return (uint32_t)ptr;
}

inline StackInfo GetStackInfo()
{
    StackInfo info;
    info.stack_base = GetHeapEnd();
    info.stack_top = (uint32_t)&_estack;
    info.stack_size = info.stack_top - info.stack_base;

    uint32_t high_water = GetHighWaterMark();
    info.stack_used = info.stack_top - high_water;

    return info;
}

} // namespace stack_debug
