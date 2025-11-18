#pragma once

#include <stdint-gcc.h>
#include <cstddef>
#include <span>

class GraphicMemory
{
public:
    static constexpr size_t Graphic_Memory_Size = 1024;

    std::span<uint8_t> AllocateMemory(const size_t byte_cnt)
    {
        if (memory_head + byte_cnt >= Graphic_Memory_Size)
        {
            // Try to move to the front
            memory_head = 0;
            if (memory_head + byte_cnt >= memory_tail)
            {
                Error("EmbeddedDiaply::AllocateMemory",
                      "Couldn't allocate anymore memory to drawing");
                return {};
            }
        }

        std::span buff(memory + memory_head, byte_cnt);

        memory_head += byte_cnt;
        parameter_write = 0;
        parameter_end_idx = byte_cnt;

        return buff;
    }

    std::span<uint8_t> RetrieveMemory(const size_t offset, const size_t byte_cnt)
    {
        if (memory_tail + offset + byte_cnt > Graphic_Memory_Size)
        {
            Error("GraphicMemory::RetrieveMemory", "Tried to read past the total memory");
        }

        return std::span<uint8_t>(memory + memory_tail + offset, byte_cnt);
    }

    uint8_t RetrieveMemory(const size_t offset)
    {
        if (memory_tail + offset + 1 > Graphic_Memory_Size)
        {
            Error("GraphicMemory::RetrieveMemory", "Tried to read past the total memory");
        }

        return memory[memory_tail + offset];
    }

    void AdvanceTail(const size_t amt)
    {
        memory_tail += amt;

        if (memory_tail >= memory_head)
        {
            memory_tail = memory_head;
        }

        if (memory_tail >= Graphic_Memory_Size)
        {
            memory_tail = 0;
        }

        memory_tail_read = memory_tail;
    }

    bool TailHasAllocatedMemory()
    {
        return memory[memory_tail] > 0;
    }

    // Maybe this should be in polygon...?
    template <typename T>
    requires std::is_integral_v<T> && std::is_unsigned_v<T>
    void PushParameter(std::span<uint8_t>& buff, T param)
    {
        const size_t end_idx = parameter_write + sizeof(T);
        if (end_idx + sizeof(T) > parameter_end_idx)
        {
            Error("GraphicMemory::PushParamter", "Tried to use more memory than was allocated");
            return;
        }

        for (size_t i = 0; i < sizeof(T); ++i)
        {
            // We are relying on truncation to save a cycle
            buff[parameter_write] = (param >> (8 * i));
            ++parameter_write;
        }
    }

    template <typename T>
    requires std::is_integral_v<T> && std::is_unsigned_v<T> T PullParamter(std::span<uint8_t>& buff,
                                                                           uint16_t& offset)
    {
        offset += sizeof(T);
        return 0;
    }

private:
    uint8_t memory[Graphic_Memory_Size];
    uint16_t memory_head;
    uint16_t memory_tail;
    uint16_t memory_tail_read;
    uint16_t parameter_write;
    uint16_t parameter_end_idx;
    uint16_t parameter_read;
};