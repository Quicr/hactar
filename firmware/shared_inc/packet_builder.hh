#pragma once

#include "ring_buffer.hh" 
#include "packet_t.hh"


static void BuildPacket(const uint8_t* buff, const uint32_t num_bytes, RingBuffer<packet_t>& packets)
{
    static packet_t* packet = nullptr;
    uint32_t idx = 0;
    uint32_t bytes_read = 0;

    while (idx < num_bytes)
    {
        if (packet == nullptr)
        {
            // Reset the number of bytes read for our next packet
            bytes_read = 0;

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
            // We can copy bytes until we run out of space 
            // or out of buffered bytes
            while (bytes_read < packet->length && idx < num_bytes)
            {
                packet->data[bytes_read] = buff[idx];
                ++bytes_read;
                ++idx;
            }

            if (bytes_read >= packet->length)
            {
                // Done the packet
                packet->is_ready = true;

                // Null out our packet pointer
                packet = nullptr;
            }
        }
    }
}