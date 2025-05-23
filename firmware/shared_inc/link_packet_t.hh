#ifndef __PACKET_H__
#define __PACKET_H__

#include "constants.hh"
#include <stddef.h>
#include <stdint.h>

#define PACKET_TYPE_TYPE uint8_t
#define PACKET_LENGTH_TYPE uint32_t
#define PACKET_READY_TYPE uint8_t

// TODO change data to be a pointer to a memory location.
struct link_packet_t
{
    static constexpr size_t Type_Size = sizeof(PACKET_TYPE_TYPE);
    static constexpr size_t Length_Size = sizeof(PACKET_LENGTH_TYPE);
    static constexpr size_t Header_Size = Length_Size + Type_Size;
    static constexpr size_t CryptoOverhead = 33;
    static constexpr size_t ExtraPadding = 32;
    static constexpr size_t Payload_Size =
        constants::Audio_Phonic_Sz + CryptoOverhead + ExtraPadding;
    static constexpr size_t Packet_Size = Header_Size + Payload_Size;
    union
    {
        struct
        {
            PACKET_TYPE_TYPE type;
            PACKET_LENGTH_TYPE length;
            uint8_t payload[Payload_Size];
        } __attribute((packed));
        uint8_t data[Packet_Size] = {0};
    };
    PACKET_READY_TYPE is_ready = false;
};

#endif