#ifndef __PACKET_H__
#define __PACKET_H__

#include <stddef.h>
#include <stdint.h>

#ifndef PACKET_SIZE
#define PACKET_SIZE 511
#endif

#define PACKET_TYPE_TYPE uint8_t
#define PACKET_LENGTH_TYPE uint16_t


// TODO change data to be a pointer to a memory location.
struct link_packet_t
{
    static constexpr size_t Type_Size = sizeof(PACKET_TYPE_TYPE);
    static constexpr size_t Length_Size = sizeof(PACKET_LENGTH_TYPE);
    static constexpr size_t Header_Size = Length_Size + Type_Size;
    static constexpr size_t Payload_Size = PACKET_SIZE - Header_Size;
    union
    {
        struct
        {
            uint8_t type;
            uint16_t length;
            uint8_t payload[Payload_Size];
        } __attribute((packed));
        uint8_t data[PACKET_SIZE] = {0};
    };
    bool is_ready = false;
};

#endif