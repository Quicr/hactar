#include "packet_handler.h"

void HandlePackets(uart_stream_t* stream, struct packet_t* packet)
{

    const uint8_t Start_Value = 0xFF;
    const uint16_t Front_Bytes = 6;
    const uint16_t Start_Bytes = Front_Bytes - 1;

    if (stream->pending_bytes == 0)
    {
        return;
    }

    if (packet->len == 0)
    {
        if (stream->pending_bytes < Front_Bytes)
        {
            return;
        }

        if (stream->tx_buffer[stream->tx_read++] != Start_Value)
        {
            return;
        }

        // Byte [1] type
        packet->bytes[0] = stream->tx_buffer[stream->tx_read++];

        // Byte [2,3] id
        packet->bytes[1] = stream->tx_buffer[stream->tx_read++];
        packet->bytes[2] = stream->tx_buffer[stream->tx_read++];

        // Byte [4,5] size
        packet->bytes[3] = stream->tx_buffer[stream->tx_read++];
        packet->bytes[4] = stream->tx_buffer[stream->tx_read++];

        // Minus off the front bytes
        stream->pending_bytes -= Front_Bytes;
    }

    // Otherwise keep getting data until it reaches the max size
    uint16_t len = packet->len;

    while (stream->pending_bytes > 0)
    {
        // Handle the pending bytes until we read the max len

        stream->pending_bytes--;

        // TODO roll over on tx_read?
    }
}

uint32_t GetData(struct packet_t* packet)
{
    // TODO
    return 0;
}