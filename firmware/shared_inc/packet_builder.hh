#pragma once

#include "ring_buffer.hh"
#include "link_packet_t.hh"
#include "logger.hh"

static int BuildPacket(const uint8_t* buff, const uint32_t num_bytes, RingBuffer<link_packet_t>& packets)
{
    static link_packet_t* packet = nullptr;
    static uint32_t bytes_read = 0;
    uint32_t idx = 0;
    uint32_t num_packets_built = 0;

    // Logger::Log(Logger::Level::Info, "Num bytes to read", num_bytes);

    while (idx < num_bytes)
    {
        if (packet == nullptr)
        {
            // Logger::Log(Logger::Level::Info, "Next packet");

            packet = &packets.Write();
            packet->is_ready = false;
        }

        // We can copy bytes until we run out of space
        // or out of buffered bytes
        // Little endian format!
        while (bytes_read < packet->length + Packet_Header_Size
            && idx < num_bytes
            && bytes_read < PACKET_SIZE)
        {
            packet->data[bytes_read++] = buff[idx++];
        }

        if (bytes_read >= packet->length + Packet_Header_Size
            || bytes_read >= PACKET_SIZE)
        {
            // Logger::Log(Logger::Level::Info, "packet ready type:", (int)packet->type);
            // Done the packet
            packet->is_ready = true;

            // Reset the number of bytes read for our next packet
            bytes_read = 0;

            // Null out our packet pointer
            packet = nullptr;

            ++num_packets_built;
        }
    }
    // Logger::Log(Logger::Level::Info, "break out");

    return num_packets_built;
}
