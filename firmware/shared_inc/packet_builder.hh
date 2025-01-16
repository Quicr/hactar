#pragma once

#include "ring_buffer.hh" 
#include "packet_t.hh"
#include "logger.hh"


static void BuildPacket(const uint8_t* buff, const uint32_t num_bytes, RingBuffer<packet_t>& packets)
{
    static packet_t* packet = nullptr;
    static uint32_t bytes_read = 0;
    uint32_t idx = 0;

    // Logger::Log(Logger::Level::Info, "Num bytes to read", num_bytes);

    while (idx < num_bytes)
    {
        if (packet == nullptr)
        {
            // Logger::Log(Logger::Level::Info, "Next packet");

            packet = &packets.Write();
            packet->is_ready = false;

            // Get the length
            // Little endian format
            // We use idx, because if we are already partially
            // through the buffered data we want to grab those 
            // next set of bytes.
            while (bytes_read < Packet_Length_Size && idx < num_bytes)
            {
                packet->data[bytes_read++] = buff[idx++];
            }
        }
        else if (bytes_read < Packet_Length_Size)
        {
            // If we get here then packet has been set to something 
            // other than nullptr and only one byte has been 
            // read which is insufficient to compare against the len
            packet->data[bytes_read++] = buff[idx++];
        }
        else
        {
            // Logger::Log(Logger::Level::Info, "Fill packet", packet->length, bytes_read);
            // We can copy bytes until we run out of space 
            // or out of buffered bytes
            while (bytes_read < packet->length + Packet_Length_Size && idx < num_bytes)
            {
                packet->data[bytes_read++] = buff[idx++];
            }

            if (bytes_read >= packet->length + Packet_Length_Size)
            {
                // Logger::Log(Logger::Level::Info, "packet ready");
                // Done the packet
                packet->is_ready = true;

                // Reset the number of bytes read for our next packet
                bytes_read = 0;

                // Null out our packet pointer
                packet = nullptr;
            }
        }
    }
    // Logger::Log(Logger::Level::Info, "break out");
}
