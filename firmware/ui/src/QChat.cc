#include "QChat.hh"

namespace qchat
{
uint16_t encode(Packet* packet, const WatchRoom& msg)
{
    // Set the length of the entire message
    packet->AppendData(
        msg.publisher_uri.length() + msg.room_uri.length() + Str_Len_Bytes,
        10);

    packet->AppendData(msg.publisher_uri.length(), Str_Len_Bits);

    for (uint16_t i = 0; i < msg.publisher_uri.length(); ++i)
    {
        packet->AppendData(msg.publisher_uri[i], 8);
    }

    packet->AppendData(msg.room_uri.length(), Str_Len_Bits);

    for (uint16_t i = 0; i < msg.room_uri.length(); ++i)
    {
        packet->AppendData(msg.room_uri[i], 8);
    }
}

void encode(Packet* packet, const uint16_t start_offset, const Ascii& msg)
{
    // Set the length of the entire message
    packet->AppendData(
        msg.message.length() + msg.message_uri.length() + Str_Len_Bytes,
        10);

    packet->AppendData(msg.message.length(), Str_Len_Bits);

    for (uint16_t i = 0; i < msg.message.length(); ++i)
    {
        packet->AppendData(msg.message[i], 8);
    }

    packet->AppendData(msg.message_uri.length(), Str_Len_Bits);

    for (uint16_t i = 0; i < msg.message_uri.length(); ++i)
    {
        packet->AppendData(msg.message_uri[i], 8);
    }
}

void decode(WatchRoom& msg, const Packet& encoded, const uint16_t start_addr)
{
    // Get the publisher_uri len
    uint16_t offset = start_addr;
    uint16_t len = encoded.GetData(offset, Str_Len_Bits);

    offset += Str_Len_Bits;
    for (uint16_t i = 0; i < len; ++i)
    {
        msg.publisher_uri.push_back(static_cast<char>(
            encoded.GetData(offset, 8)));
        offset += 8;
    }

    len = encoded.GetData(offset, Str_Len_Bits);
    offset += Str_Len_Bits;

    for (uint16_t i = 0; i < len; ++i)
    {
        msg.room_uri.push_back(static_cast<char>(
            encoded.GetData(offset, 8)));
        offset += 8;
    }
}

void decode(Ascii& msg, const Packet& encoded, const uint16_t start_addr)
{
    uint16_t offset = start_addr;
    uint16_t len = encoded.GetData(offset, Str_Len_Bits);

    offset += Str_Len_Bits;
    for (uint16_t i = 0; i < len; ++i)
    {
        msg.message.push_back(static_cast<char>(
            encoded.GetData(offset, 8)));
        offset += 8;
    }

    len = encoded.GetData(offset, Str_Len_Bits);
    offset += Str_Len_Bits;

    for (uint16_t i = 0; i < len; ++i)
    {
        msg.message_uri.push_back(static_cast<char>(
            encoded.GetData(offset, 8)));
        offset += 8;
    }
}

} // namespace qchat
