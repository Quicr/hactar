#pragma once

#include <string>
#include <vector>

#include "Packet.hh"


// Quicr based chat protocol
namespace qchat
{

//
// Model
//

constexpr uint8_t field_len_bits = 32U;
constexpr uint8_t field_len_bytes = field_len_bits / 8;
constexpr uint8_t msg_len_bits = 32U; // 2^32 -1

struct Room;

// Channels are the top level construct is made up of
// one or more rooms (see Room)
// Channels are where the policies are applied,
// advertisement of room status are done.
struct Channel
{
    bool is_default{ false };
    std::string channel_uri; //quicr namespace as URI
    std::string channel_id_hex;  // quicr namespace for the channel
    std::vector<Room> rooms{};
    // TODO: a map may be more useful here
};

// Room represent the main handle for sending and receiving
// user messages. Users are added to the rooms
struct Room
{
    bool is_default{ false };
    std::string friendly_name;
    std::string publisher_uri;
    std::string room_uri; //quicr namespace as URI
    std::string root_channel_uri; // Owner of this room
};

//
// QChat Messages
//

// Message types 0 - 9 are for QChat Message Subtype
enum struct MessageTypes: uint8_t
{
    Watch = 0,
    Unwatch,
    Ascii
};

// Watch messages from a given room
struct WatchRoom
{
    std::string publisher_uri; // quicr namespacee matching the publisher
    std::string room_uri; // matches quicr namespace for the room namespace
};

// Express no interest in receiving messages from a given room
struct UnwatchRoom
{
    std::string room_uri;
};

struct Ascii
{
    std::string message_uri; // matches quicr_name for the message sender
    std::string message; // todo: make it vector of bytes
};



//
// Encode/Decode API
//

struct Codec
{

    static std::vector<uint8_t>
        string_to_bytes(const std::string& str)
    {
        return { str.begin(), str.end() };
    }

    static inline void AppendStringFieldToPacket(const std::string& field,
        const uint32_t len_bits,
        std::unique_ptr<Packet>& packet)
    {
        packet->AppendData(field.length(), len_bits);
        for (size_t i = 0; i < field.length(); ++i)
        {
            packet->AppendData(field[i], 8);
        }
    }

    static inline void SetStringFieldFromPacket(std::string& field,
        size_t& offset,
        const uint32_t len_bits,
        const std::unique_ptr<Packet>& packet)
    {
        size_t field_len = packet->GetData(offset, len_bits);
        offset += len_bits;
        for (size_t i = 0; i < field_len; ++i)
        {
            field.push_back(static_cast<char>(
                packet->GetData(offset, 8)));
            offset += 8;
        }
    }

    static void encode(std::unique_ptr<Packet>& packet, const Room& room)
    {
        // [total_len] = [type][pub_uri_len][[pub_uri][room_uri_len][room_uri]
        const uint32_t Bool_Byte_Len = 1;
        const uint32_t Type_Byte_Len = 1;
        const uint32_t Num_Fields = 5;

        // 1 for type, 5 x field_len_bytes = is_default, friendly_name,
        //  room_uri, publisher_uri, root_channel_uri
        const uint16_t extra_bytes = Type_Byte_Len +
            Num_Fields * field_len_bytes;

        const uint16_t field_bytes = Bool_Byte_Len
            + room.friendly_name.length()
            + room.publisher_uri.length()
            + room.room_uri.length()
            + room.root_channel_uri.length();

        packet->AppendData(extra_bytes + field_bytes, 10);

        // Set the message type, starts at bit 24
        packet->AppendData((unsigned int)Packet::Commands::RoomsGet, 8);

        // Append the is_default field len and value
        packet->AppendData((unsigned int)1, field_len_bits);
        packet->AppendData((unsigned int)room.is_default, 8);

        // Append the friendly name
        AppendStringFieldToPacket(room.friendly_name, field_len_bits, packet);

        // Append the publisher uri
        AppendStringFieldToPacket(room.publisher_uri, field_len_bits, packet);

        // Append the room uri
        AppendStringFieldToPacket(room.room_uri, field_len_bits, packet);

        // Append the root channel uri
        AppendStringFieldToPacket(room.root_channel_uri, field_len_bits, packet);
    }

    static void encode(std::unique_ptr<Packet>& packet, const WatchRoom& msg)
    {
        // [total_len][type][pub_uri_len][[pub_uri][room_uri_len][room_uri]
        const uint16_t extra_bytes = 1 + field_len_bytes + field_len_bytes;
        packet->AppendData(extra_bytes + msg.publisher_uri.length() +
            msg.room_uri.length(), 10);
        // Set the message type, starts at bit 24
        packet->AppendData((unsigned int)MessageTypes::Watch, 8);

        // Append the publisher uri
        AppendStringFieldToPacket(msg.publisher_uri, field_len_bits, packet);

        // Append the room uri
        AppendStringFieldToPacket(msg.room_uri, field_len_bits, packet);
    }

    static void encode(std::unique_ptr<Packet>& packet,
        const uint16_t start_offset,
        const Ascii& msg)
    {
        // [total_len][type][msg_uri_len][[msg_uri][msg_len][msg]

        // +9 for the type byte, 2 len bytes and the 8 bytes of length
        uint16_t extra_bytes = 1 + field_len_bytes + field_len_bytes;
        packet->AppendData(msg.message_uri.length()
            + msg.message.length() + extra_bytes, 10);
        packet->AppendData((unsigned int)MessageTypes::Ascii, 8);

        // Append the message_uri
        AppendStringFieldToPacket(msg.message_uri, field_len_bits, packet);

        // Append the message
        AppendStringFieldToPacket(msg.message, field_len_bits, packet);
    }

    static bool decode(Room& room,
        const std::unique_ptr<Packet>& encoded,
        const size_t current_offset)
    {
        size_t offset = current_offset;

        // Get the is_default field
        size_t field_len = encoded->GetData(offset, field_len_bits);
        offset += field_len_bits;
        room.is_default = static_cast<bool>(encoded->GetData(offset, 8));
        offset += 8;

        // Get the friendly name
        SetStringFieldFromPacket(room.friendly_name, offset,
            field_len_bits, encoded);

        // Get the publisher uri
        SetStringFieldFromPacket(room.publisher_uri, offset,
            field_len_bits, encoded);

        // Get the room uri
        SetStringFieldFromPacket(room.room_uri, offset,
            field_len_bits, encoded);

        // Get the root channel uri
        SetStringFieldFromPacket(room.root_channel_uri, offset,
            field_len_bits, encoded);

        return true;
    }

    static bool decode(WatchRoom& msg,
        const std::unique_ptr<Packet>& encoded,
        const size_t current_offset)
    {
        if (encoded == nullptr)
        {
            return false;
        }
        // type is already determined elswhere
        size_t offset = current_offset;

        // Get the publisher uri
        SetStringFieldFromPacket(msg.publisher_uri, offset,
            field_len_bits, encoded);

        // Get the room uri
        SetStringFieldFromPacket(msg.room_uri, offset,
            field_len_bits, encoded);

        return true;
    }

    static bool decode(Ascii& msg,
        const std::unique_ptr<Packet>& encoded,
        const size_t current_offset)
    {
        size_t offset = current_offset;

        // Get the message uri
        SetStringFieldFromPacket(msg.message_uri, offset,
            field_len_bits, encoded);

        // Get the actual msg
        SetStringFieldFromPacket(msg.message, offset,
            field_len_bits, encoded);

        return true;
    }

};



} //namespace