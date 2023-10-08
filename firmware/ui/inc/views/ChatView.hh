#pragma once

#include "ViewInterface.hh"
#include "String.hh"
#include "Packet.hh"
#include "Message.hh"

#define Margin_0 0

#define Padding_0 0
#define Padding_1 1
#define Padding_2 2
#define Padding_3 3

#define Text_Draw_Speed 20

// TODO : Move this out of view and move into model

struct Room {
    bool is_default {false};
    String room_uri; //quicr namespace as URI
    String room_id_hex;  // quicr namespace for the room
    String root_channel_id_hex; // Owner of this room
};

struct Channel {
    bool is_default {false};
    String channel_uri; //quicr namespace as URI
    String channel_id_hex;  // quicr namespace for the channel
    Vector<Room> rooms {};
     // TODO: a map may be more useful here
};

class ChatView : public ViewInterface
{
public:
    ChatView(UserInterfaceManager& manager,
             Screen& screen,
             Q10Keyboard& keyboard,
             SettingManager& setting_manager);
    ~ChatView();

    void SetActiveRoom(const Room &room);

protected:
    void AnimatedDraw();
    void Draw();
    void Update();
    void HandleInput();
private:
    void DrawTitle();
    void DrawUsrInputSeperator();
    void DrawMessages();

    // Consts
    const String name_seperator = ": ";

    // We need to keep this saved somewhere outside of the chat view?
    // Perhaps in the user interface manager.
    Vector<Message> messages;
    bool redraw_messages = true;

    struct
    {
        uint16_t timestamp_colour = C_CYAN;
        uint16_t name_colour = C_BLUE;
        uint16_t body_colour = C_WHITE;
    } settings;

    Room active_room;
};