#pragma once

#include <string>
#include <vector>

#include "Packet.hh"


// Quicr based chat protocol
namespace qchat {

//
// Model
//

constexpr uint8_t uri_len_bits = 32U;
constexpr uint8_t uri_len_bytes = uri_len_bits/8;
constexpr uint8_t msg_len_bits = 32U; // 2^32 -1 

struct Room;

// Channels are the top level construct is made up of
// one or more rooms (see Room)
// Channels are where the policies are applied,
// advertisement of room status are done.
struct Channel {
    bool is_default {false};
    std::string channel_uri; //quicr namespace as URI
    std::string channel_id_hex;  // quicr namespace for the channel
    std::vector<Room> rooms {};
     // TODO: a map may be more useful here
};

// Room represent the main handle for sending and receiving
// user messages. Users are added to the rooms
struct Room {
    bool is_default {false};
    std::string friendly_name;
    std::string room_uri; //quicr namespace as URI
    std::string publisher_uri;
    std::string root_channel_uri; // Owner of this room
};

//
// QChat Messages
//

// Message types 0 - 9 are for QChat Message Subtype
enum struct MessageTypes: uint8_t {
    Watch = 0,
    Unwatch,
    Ascii
};

// Watch messages from a given room
struct WatchRoom {
  std::string publisher_uri; // quicr namespacee matching the publisher
  std::string room_uri; // matches quicr namespace for the room namespace
};

// Express no interest in receiving messages from a given room
struct UnwatchRoom {
  std::string room_uri;
};

struct Ascii  {
  std::string message_uri; // matches quicr_name for the message sender
  std::string message; // todo: make it vector of bytes
};



//
// Encode/Decode API
//

struct Codec {

static  std::vector<uint8_t>
string_to_bytes(const std::string& str)
{
  return { str.begin(), str.end() };
}

static void 
encode(Packet* packet, const WatchRoom& msg)
{
    // [type][pub_uri_len][[pub_uri][room_uri_len][room_uri]
    packet->AppendData((unsigned int) MessageTypes::Watch, 8);
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

static void encode(Packet* packet, const uint16_t start_offset, const Ascii& msg)
{
    // [type][msg_uri_len][[msg_uri][msg_len][msg]
    packet->AppendData((unsigned int) MessageTypes::Ascii, 8);
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

static bool decode(WatchRoom& msg, const Packet* encoded, size_t& current_offset)
{
    if (encoded == nullptr) {
      return false;
    }
    // type is already determined elswhere
    size_t offset = current_offset;
    
    // Get the publisher_uri len
    size_t uri_len = encoded->GetData(offset, uri_len_bits);
    offset += uri_len_bits;
    for (size_t i = 0; i < uri_len; ++i)
    {
        msg.publisher_uri.push_back(static_cast<char>(
            encoded->GetData(offset, 8)));
        offset += 8;
    }

    // room_uri
    uri_len = encoded->GetData(offset, uri_len_bits);
    offset += uri_len_bits;
    for (uint16_t i = 0; i < uri_len; ++i)
    {
        msg.room_uri.push_back(static_cast<char>(
            encoded->GetData(offset, 8)));
        offset += 8;
    }

    current_offset = offset;
    return true;
}

static bool decode(Ascii& msg, const Packet* encoded, size_t& current_offset)
{
    // type
    size_t offset = current_offset;
    
    // message uri
    size_t uri_len = encoded->GetData(offset, uri_len_bits);
    offset += uri_len_bits;
    for (uint16_t i = 0; i < uri_len; ++i)
    {
        msg.message_uri.push_back(static_cast<char>(
            encoded->GetData(offset, 8)));
        offset += 8;
    }

    // msg
    size_t msg_len = encoded->GetData(offset, msg_len_bits);
    offset += msg_len_bits;

    for (uint16_t i = 0; i < msg_len; ++i)
    {
        msg.message.push_back(static_cast<char>(
            encoded->GetData(offset, 8)));
        offset += 8;
    }
    
    current_offset = offset;
    return true;
}

};



} //namespace