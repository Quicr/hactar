#ifndef __PACKET_H__
#define __PACKET_H__

#include <stddef.h>
#include <stdint.h>

#ifndef PACKET_SIZE
#define PACKET_SIZE 512
#endif

#define LENGTH_TYPE uint16_t

constexpr size_t Packet_Length_Size = sizeof(LENGTH_TYPE);
constexpr size_t Packet_Header_Size = Packet_Length_Size;
constexpr size_t Packet_Payload_Size = PACKET_SIZE - Packet_Header_Size;

struct packet_t
{
    union
    {
        struct 
        {
            uint16_t length;
            uint8_t payload[Packet_Payload_Size];
        } __attribute((packed));
        uint8_t data[PACKET_SIZE] = {0};
    };
    bool is_ready = false;
};

#endif