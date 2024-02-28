#pragma once

#include <string>
#include <vector>

#include "SerialPacket.hh"
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart1;
// Quicr based chat protocol
namespace qchat
{

//
// Model
//

constexpr uint8_t Field_Len_Bytes = 4;
constexpr uint8_t Msg_Field_Len = 4;

struct Room; // TODO is this needed?

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
    bool is_default;
    std::string friendly_name;
    std::string publisher_uri;
    std::string room_uri; //quicr namespace as URI
    std::string root_channel_uri; // Owner of this room

    Room(bool is_default=false, std::string friendly_name="", std::string publisher_uri="", std::string room_uri="", std::string root_channel_uri="") :
        is_default(is_default),
        friendly_name(friendly_name),
        publisher_uri(publisher_uri),
        room_uri(room_uri),
        root_channel_uri(root_channel_uri) {}
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

    WatchRoom() : publisher_uri(), room_uri() {}
    WatchRoom(std::string publisher_uri, std::string room_uri) : publisher_uri(publisher_uri), room_uri(room_uri) {}
};

// Express no interest in receiving messages from a given room
struct UnwatchRoom
{
    std::string room_uri;

    UnwatchRoom() : room_uri() {}
    UnwatchRoom(std::string room_uri) : room_uri(room_uri) {}
};

struct Ascii
{
    std::string message_uri; // matches quicr_name for the message sender
    std::string message; // todo: make it vector of bytes

    Ascii() : message_uri(), message() {}
    Ascii(std::string message_uri, std::string message) : message_uri(message_uri), message(message) {}
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
        const uint32_t len_bytes,
        std::unique_ptr<SerialPacket>& packet)
    {
        packet->SetData(field.length(), len_bytes);
        for (uint32_t i = 0; i < field.length(); ++i)
        {
            packet->SetData(field[i], 1);
        }
    }

    static inline void SetStringFieldFromPacket(std::string& field,
        uint32_t& offset,
        const uint32_t len_bytes,
        const std::unique_ptr<SerialPacket>& packet)
    {
        uint32_t field_len = packet->GetData<uint32_t>(offset, len_bytes);
        offset += len_bytes;
        for (uint32_t i = 0; i < field_len; ++i)
        {
            field.push_back(packet->GetData<char>(offset, 1));
            offset += 1;
        }
    }

    static void encode(std::unique_ptr<SerialPacket>& packet, const Room& room)
    {
        // [total_len] = [type][pub_uri_len][[pub_uri][room_uri_len][room_uri]
        const uint32_t Bool_Byte_Len = 1;
        const uint32_t Type_Byte_Len = 1;
        const uint32_t Num_Fields = 5;

        // 1 for type, 5 x field_len_bytes = is_default, friendly_name,
        //  room_uri, publisher_uri, root_channel_uri
        const uint16_t extra_bytes = Type_Byte_Len +
            Num_Fields * Field_Len_Bytes;

        const uint16_t field_bytes = Bool_Byte_Len
            + room.friendly_name.length()
            + room.publisher_uri.length()
            + room.room_uri.length()
            + room.root_channel_uri.length();

        // Placed into bytes [3,4]
        packet->SetData(extra_bytes + field_bytes, 2);

        // Set the message type, starts at byte 5
        packet->SetData(SerialPacket::Commands::RoomsGet, 1);

        // Append the is_default field len and value
        packet->SetData(1, Field_Len_Bytes);
        packet->SetData(room.is_default, 1);

        // Append the friendly name
        AppendStringFieldToPacket(room.friendly_name, Field_Len_Bytes, packet);

        // Append the publisher uri
        AppendStringFieldToPacket(room.publisher_uri, Field_Len_Bytes, packet);

        // Append the room uri
        AppendStringFieldToPacket(room.room_uri, Field_Len_Bytes, packet);

        // Append the root channel uri
        AppendStringFieldToPacket(room.root_channel_uri, Field_Len_Bytes, packet);
    }

    static void encode(std::unique_ptr<SerialPacket>& packet, const WatchRoom& msg)
    {
        // [total_len][type][pub_uri_len][[pub_uri][room_uri_len][room_uri]
        const uint16_t extra_bytes = 1 + Field_Len_Bytes + Field_Len_Bytes;
        packet->SetData(extra_bytes + msg.publisher_uri.length() +
            msg.room_uri.length(), 2);
        // Set the message type, starts at bit 24
        packet->SetData((unsigned char)MessageTypes::Watch, 1);

        // Append the publisher uri
        AppendStringFieldToPacket(msg.publisher_uri, Field_Len_Bytes, packet);

        // Append the room uri
        AppendStringFieldToPacket(msg.room_uri, Field_Len_Bytes, packet);
    }

    static void encode(std::unique_ptr<SerialPacket>& packet,
        const uint16_t start_offset,
        const Ascii& msg)
    {
        uint16_t offset = start_offset;
        // [total_len][type][msg_uri_len][[msg_uri][msg_len][msg]

        // +9 for the type byte, 2 len bytes and the 8 bytes of length
        uint16_t extra_bytes = 1 + Field_Len_Bytes + Field_Len_Bytes;
        uint16_t data_len = msg.message_uri.length() + msg.message.length();

        const auto &str_size = ">>>>>>> " + std::to_string(data_len + extra_bytes) + "\n";
        HAL_UART_Transmit(&huart1, reinterpret_cast<const uint8_t *>(str_size.c_str()), str_size.size(), HAL_MAX_DELAY);

        packet->SetData(data_len + extra_bytes, offset, 2);
        offset += 2;

        packet->SetData((unsigned char)MessageTypes::Ascii, offset, 1);
        offset += 1;

        // Append the message_uri
        AppendStringFieldToPacket(msg.message_uri, Field_Len_Bytes, packet);

        // Append the message
        AppendStringFieldToPacket(msg.message, Field_Len_Bytes, packet);
    }

    static void encode(std::unique_ptr<SerialPacket>& packet,
        const UnwatchRoom& unwatch)
    {
        // Packet is in this order
        // [total_len][type][room_uri_len][room_uri]

        uint16_t extra_bytes = 1 + Field_Len_Bytes;
        uint16_t data_len = unwatch.room_uri.length();
        packet->SetData(extra_bytes + data_len, 2);
        packet->SetData((unsigned int)MessageTypes::Unwatch, 1);

        // Append the room uri
        AppendStringFieldToPacket(unwatch.room_uri, Field_Len_Bytes, packet);
    }

    static bool decode(std::unique_ptr<Room>& room,
        const std::unique_ptr<SerialPacket>& encoded,
        const uint32_t current_offset)
    {
        uint32_t offset = current_offset;

        // Get the is_default field length
        uint32_t is_default_len = encoded->GetData<uint32_t>(offset, Field_Len_Bytes);
        offset += Field_Len_Bytes;

        // Get the room is default field
        room->is_default = encoded->GetData<bool>(offset, is_default_len);
        offset += is_default_len;

        // Get the friendly name
        SetStringFieldFromPacket(room->friendly_name, offset,
            Field_Len_Bytes, encoded);

        // Get the publisher uri
        SetStringFieldFromPacket(room->publisher_uri, offset,
            Field_Len_Bytes, encoded);

        // Get the room uri
        SetStringFieldFromPacket(room->room_uri, offset,
            Field_Len_Bytes, encoded);

        // Get the root channel uri
        SetStringFieldFromPacket(room->root_channel_uri, offset,
            Field_Len_Bytes, encoded);

        return true;
    }

    static bool decode(WatchRoom& msg,
        const std::unique_ptr<SerialPacket>& encoded,
        const uint32_t current_offset)
    {
        if (encoded == nullptr)
        {
            return false;
        }
        // type is already determined elswhere
        uint32_t offset = current_offset;

        // Get the publisher uri
        SetStringFieldFromPacket(msg.publisher_uri, offset,
            Field_Len_Bytes, encoded);

        // Get the room uri
        SetStringFieldFromPacket(msg.room_uri, offset,
            Field_Len_Bytes, encoded);

        return true;
    }

    static bool decode(Ascii& msg,
        const std::unique_ptr<SerialPacket>& encoded,
        const uint32_t current_offset)
    {
        uint32_t offset = current_offset;

        // Get the message uri
        SetStringFieldFromPacket(msg.message_uri, offset,
            Field_Len_Bytes, encoded);

        // Get the actual msg
        SetStringFieldFromPacket(msg.message, offset,
            Field_Len_Bytes, encoded);

        return true;
    }

    static bool decode(std::unique_ptr<UnwatchRoom>& unwatch,
        const std::unique_ptr<SerialPacket>& encoded,
        const uint32_t current_offset)
    {
        uint32_t offset = current_offset;

        // Get the room_uri
        SetStringFieldFromPacket(unwatch->room_uri, offset,
            Field_Len_Bytes, encoded);

        return true;
    }
};



} //namespace