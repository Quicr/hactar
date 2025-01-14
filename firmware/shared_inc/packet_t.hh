#ifndef __PACKET_H__
#define __PACKET_H__

#include <stddef.h>
#include <stdint.h>

#ifndef PACKET_SIZE
#define PACKET_SIZE 512
#endif

#define LENGTH_TYPE uint16_t
#define SERIAL_HEADER_SIZE sizeof(LENGTH_TYPE)
#define PAYLOAD_SIZE PACKET_SIZE - SERIAL_HEADER_SIZE

typedef struct
{
    union
    {
        struct 
        {
            uint16_t length;
            uint8_t payload[PAYLOAD_SIZE];
        };
        uint8_t data[PACKET_SIZE] = {0};
    };
    bool is_ready = false;
} packet_t;

#endif