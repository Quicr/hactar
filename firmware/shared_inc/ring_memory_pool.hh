#pragma once

#include <stdint.h>

class RingMemoryPool
{
public:
    RingMemoryPool(const uint32_t num_blocks, const uint32_t block_size) :
        num_blocks(num_blocks),
        block_size(block_size),
        buff(new uint8_t[num_blocks * block_size]{0}),
        is_busy(new bool[num_blocks]{0}),
        next_block_idx(0),
        blocks_available(num_blocks)
    {
    }

    ~RingMemoryPool()
    {

        delete[] buff;
        delete[] is_busy;
    }

    bool ReserveBlock(uint8_t** out_buff, uint32_t& out_idx)
    {
        if (blocks_available == 0)
        {
            return false;
        }

        // Search for an available block
        uint32_t search_num = 0;

        while (search_num < num_blocks)
        {
            if (next_block_idx >= num_blocks)
            {
                next_block_idx = 0;
            }

            if (!is_busy[next_block_idx])
            {
                break;
            }

            search_num++;
            next_block_idx++;
        }

        if (is_busy[next_block_idx])
        {
            return false;
        }

        *out_buff = buff + next_block_idx;
        is_busy[next_block_idx] = true;
        out_idx = next_block_idx;
        next_block_idx++;
        blocks_available--;

        return true;
    }

    bool ReleaseBlock(const uint32_t idx)
    {
        if (blocks_available == num_blocks)
        {
            return false;
        }

        if (idx >= block_size)
        {
            return false;
        }

        is_busy[idx] = false;
        blocks_available++;

        return true;
    }

    uint32_t BlocksAvailable()
    {
        return blocks_available;
    }

    uint32_t BlockSize()
    {
        return block_size;
    }

private:
    const uint32_t num_blocks;
    const uint32_t block_size;
    uint8_t* buff;
    bool* is_busy;
    uint32_t next_block_idx;
    uint32_t blocks_available;
};