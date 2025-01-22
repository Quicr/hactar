#ifndef __PACKET_H__
#define __PACKET_H__

#include <stddef.h>
#include <stdint.h>

#ifndef PACKET_SIZE
#define PACKET_SIZE 511
#endif

#define PACKET_TYPE_TYPE uint8_t
#define PACKET_LENGTH_TYPE uint16_t

constexpr size_t Packet_Type_Size = sizeof(PACKET_TYPE_TYPE);
constexpr size_t Packet_Length_Size = sizeof(PACKET_LENGTH_TYPE);
constexpr size_t Packet_Header_Size = Packet_Length_Size + Packet_Type_Size;
constexpr size_t Packet_Payload_Size = PACKET_SIZE - Packet_Header_Size;

// TODO change data to be a pointer to a memory location.
struct link_packet_t
{
    union
    {
        struct
        {
            uint8_t type;
            uint16_t length;
            uint8_t payload[Packet_Payload_Size];
        } __attribute((packed));
        uint8_t data[PACKET_SIZE] = {0};
    };
    bool is_ready = false;
};

#endif