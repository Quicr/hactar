#include "QChat.hh"

namespace qchat
{
uint16_t encode(Packet* packet, const WatchRoom& msg)
{
    // [type][pub_uri_len][[pub_uri][room_uri_len][room_uri]
    packet->AppendData(MessageTypes::Watch, 8);
    packet->AppendData(msg.publisher_uri.length(), uri_len_bits);
    for (size_t i = 0; i < msg.publisher_uri.length(); ++i)
    {
        packet->AppendData(msg.publisher_uri[i], 8);
    }

    packet->AppendData(msg.room_uri.length(), uri_len_bits);
    for (uint16_t i = 0; i < msg.room_uri.length(); ++i)
    {
        packet->AppendData(msg.room_uri[i], 8);
    }
}

void encode(Packet* packet, const uint16_t start_offset, const Ascii& msg)
{
    // [type][msg_uri_len][[msg_uri][msg_len][msg]
    packet->AppendData(MessageTypes::Ascii, 8);
    packet->AppendData(msg.message_uri.length(), uri_len_bits);
    for (size_t i = 0; i < msg.message_uri.length(); ++i)
    {
        packet->AppendData(msg.message_uri[i], 8);
    }
    packet->AppendData(msg.message.length(), msg_len_bits);
    for (size_t i = 0; i < msg.message.length(); ++i)
    {
        packet->AppendData(msg.message[i], 8);
    }
}

bool decode(WatchRoom& msg, const Packet& encoded, size_t& current_offset)
{
    // type
    size_t offset = current_offset;
    MessageTypes msg_type = static_cast<MessageTypes>(encoded.GetData(offset, 8));
    if (msg_type != MessageTypes::Watch) {
        return false;
    }

    // Get the publisher_uri len
    size_t offset += 8;
    size_t uri_len = encoded.GetData(offset, uri_len_bits);
    offset += uri_len_bits;
    for (size_t i = 0; i < uri_len; ++i)
    {
        msg.publisher_uri.push_back(static_cast<char>(
            encoded.GetData(offset, 8)));
        offset += 8;
    }

    // room_uri
    uri_len = encoded.GetData(offset, uri_len_bits);
    offset += uri_len_bits;
    for (uint16_t i = 0; i < uri_len; ++i)
    {
        msg.room_uri.push_back(static_cast<char>(
            encoded.GetData(offset, 8)));
        offset += 8;
    }

    current_offset = offset;
    return true;
}

bool decode(Ascii& msg, const Packet& encoded, size_t& current_offset)
{
    // type
    size_t offset = current_offset;
    MessageTypes msg_type = static_cast<MessageTypes>(encoded.GetData(offset, 8));
    if (msg_type != MessageTypes::Ascii) {
        return false;
    }

    // message uri
    offset += 8;
    size_t uri_len = encoded.GetData(offset, uri_len_bits);
    offset += uri_len_bits;
    for (uint16_t i = 0; i < uri_len; ++i)
    {
        msg.message_uri.push_back(static_cast<char>(
            encoded.GetData(offset, 8)));
        offset += 8;
    }

    // msg
    size_t msg_len = encoded.GetData(offset, msg_len_bits);
    offset += msg_len_bits;

    for (uint16_t i = 0; i < msg_len; ++i)
    {
        msg.message.push_back(static_cast<char>(
            encoded.GetData(offset, 8)));
        offset += 8;
    }
    
    current_offset = offset;
    return true;
}

} // namespace qchat
