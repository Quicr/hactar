#pragma once

#include <string>
#include <vector>


// Quicr based chat protocol
namespace qchat {

//
// Model
//

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

std::string encode(const WatchRoom& msg);
std::string encode(const Ascii& msg);

void decode(WatchRoom& msg, const std::string& encoded);
void decode(Ascii& msg, const std::string& encoded);


} //namespace