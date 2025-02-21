#ifndef STATIC_RING_BUFF_H
#define STATIC_RING_BUFF_H

#include <stdint.h>

typedef struct
{
    uint8_t* buff;
    uint16_t size;
    uint16_t read_idx;
    uint16_t write_idx;
} StaticRingBuffer;

static void StaticRingBuffer_Init(StaticRingBuffer* ring_buff, uint8_t* buff, const uint16_t size)
{
    ring_buff->buff = buff;
    ring_buff->size = size;
    ring_buff->read_idx = 0;
    ring_buff->write_idx = 0;
}

static uint8_t StaticRingBuffer_Available(StaticRingBuffer* ring_buff)
{
    if (ring_buff->write_idx >= ring_buff->read_idx)
    {
        return ring_buff->write_idx - ring_buff->read_idx;
    }
    else
    {
        return ring_buff->size - ring_buff->read_idx + ring_buff->write_idx;
    }
}

static uint8_t StaticRingBuffer_Read(StaticRingBuffer* ring_buff)
{
    if (ring_buff->read_idx >= ring_buff->size)
    {
        ring_buff->read_idx = 0;
    }

    return ring_buff->buff[ring_buff->read_idx++];
}

static uint8_t* StaticRingBuffer_Write(StaticRingBuffer* ring_buff)
{
    if (ring_buff->write_idx >= ring_buff->size)
    {
        ring_buff->write_idx = 0;
    }

    return &ring_buff->buff[ring_buff->write_idx++];
}

static void StaticRingBuffer_Commit(StaticRingBuffer* ring_buff, const uint8_t v)
{
    if (ring_buff->write_idx >= ring_buff->size)
    {
        ring_buff->write_idx = 0;
    }

    ring_buff->buff[ring_buff->write_idx++] = v;
}

static int8_t StaticRingBuffer_WriteBack(StaticRingBuffer* ring)
{
    // Do nothing, the value has already been read
    if (ring->write_idx == ring->read_idx)
    {
        return 0;
    }

    if (ring->write_idx == 0)
    {
        ring->write_idx = ring->size - 1;
    }
    else
    {
        --ring->write_idx;
    }

    return 1;
}

#endif