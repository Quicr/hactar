#ifndef __PACKET_H__
#define __PACKET_H__

#include "constants.hh"
#include "logger.hh"
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
    static constexpr size_t Crypto_Overhead = 33;
    static constexpr size_t Extra_Padding = 107;
    static constexpr size_t Payload_Size =
        constants::Audio_Phonic_Sz + Crypto_Overhead + Extra_Padding;
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

    // DO NOTE- This function is not intended to be safe
    void Get(void* data, const uint32_t num_bytes, const uint32_t payload_offset)
    {
        if (payload_offset + num_bytes > length)
        {
            return;
        }

        uint8_t* storage = (uint8_t*)data;
        for (uint32_t i = 0; i < num_bytes; ++i)
        {
            storage[i] |= payload[payload_offset + i];
        }
    }

    void Dump()
    {
        Logger::Log(Logger::Level::Info, "Dumping packet");
        Logger::Log(Logger::Level::Raw, "type %d", type);
        Logger::Log(Logger::Level::Raw, "len %d", length);
        for (int i = 0; i < length; i++)
        {
            Logger::Log(Logger::Level::Raw, "%d", (int)payload[i]);
        }
    }
};

#endif