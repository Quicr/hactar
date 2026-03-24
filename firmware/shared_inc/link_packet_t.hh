#ifndef __PACKET_H__
#define __PACKET_H__

#include "constants.hh"
#include "logger.hh"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

// TODO change data to be a pointer to a memory location.
struct link_packet_t
{
    static constexpr size_t Sync_Word_Size = 4;
    static constexpr size_t Type_Size = sizeof(uint16_t);
    static constexpr size_t Length_Size = sizeof(uint32_t);
    static constexpr size_t Header_Size = Length_Size + Type_Size;
    static constexpr size_t Crypto_Overhead = 33;
    static constexpr size_t Extra_Padding = 447;
    static constexpr size_t Payload_Size =
        constants::Audio_Phonic_Sz + Crypto_Overhead + Extra_Padding;
    static constexpr size_t Packet_Size = Header_Size + Payload_Size + Sync_Word_Size;

    const uint8_t sync_word[Sync_Word_Size] = {0x4C, 0x49, 0x4E, 0x4B};
    uint16_t type = 0;
    uint32_t length = 0;
    std::array<uint8_t, Payload_Size> payload{0};

    bool is_ready = false;

    static constexpr size_t Writeable_Size = Packet_Size - Sync_Word_Size;

    std::span<uint8_t, Writeable_Size> WriteableData() noexcept
    {
        return std::span<uint8_t, Writeable_Size>{reinterpret_cast<uint8_t*>(this) + Sync_Word_Size,
                                                  Writeable_Size};
    }

    std::span<const uint8_t, Packet_Size> ReadableData() const noexcept
    {
        return std::span<const uint8_t, Packet_Size>{reinterpret_cast<const uint8_t*>(this),
                                                     Packet_Size};
    }

    std::span<const uint8_t> PacketData() const noexcept
    {
        return std::span<const uint8_t>{reinterpret_cast<const uint8_t*>(this),
                                        length + Header_Size + Sync_Word_Size};
    }

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
} __attribute__((packed));

static_assert(sizeof(link_packet_t) == link_packet_t::Packet_Size + sizeof(bool));
static_assert(link_packet_t::Packet_Size == 650 /* sync + header + 640 value */);

#endif
