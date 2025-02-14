#ifndef STATIC_RING_BUFF_H
#define STATIC_RING_BUFF_H

#include <cstdint>

typedef struct
{
    uint8_t* buff;
    uint16_t size;
    uint16_t read_idx;
    uint16_t write_idx;
} StaticRingBuffer;

void StaticRingBuffer_Init(StaticRingBuffer* ring_buff, uint8_t* buff, const uint16_t size)
{
    ring_buff->buff = buff;
    ring_buff->size = size;
    ring_buff->read_idx = 0;
    ring_buff->write_idx = 0;
}

uint8_t StaticRingBuffer_Available(StaticRingBuffer* ring_buff)
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

uint8_t StaticRingBuffer_Read(StaticRingBuffer* ring_buff)
{
    if (ring_buff->read_idx >= ring_buff->size)
    {
        ring_buff->read_idx = 0;
    }

    return ring_buff->buff[ring_buff->read_idx++];
}

uint8_t* StaticRingBuffer_Write(StaticRingBuffer* ring_buff)
{
    if (ring_buff->write_idx >= ring_buff->size)
    {
        ring_buff->write_idx = 0;
    }

    return &ring_buff->buff[ring_buff->write_idx++];
}

void StaticRingBuffer_Write(StaticRingBuffer* ring_buff, const uint8_t v)
{

    if (ring_buff->write_idx >= ring_buff->size)
    {
        ring_buff->write_idx = 0;
    }

    ring_buff->buff[ring_buff->write_idx++] = v;
}

#endif