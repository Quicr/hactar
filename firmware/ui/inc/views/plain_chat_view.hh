#pragma once

#include "view_interface.hh"
#include <string>
#include "serial_packet.hh"
#include "message.hh"
#include "qchat.hh"

#define Margin_0 0

#define Padding_0 0
#define Padding_1 1
#define Padding_2 2
#define Padding_3 3

#define Text_Draw_Speed 20

class PlainChatView : public ViewInterface
{
public:
    PlainChatView(UserInterfaceManager& manager,
        Screen& screen,
        Q10Keyboard& keyboard,
        SettingManager& setting_manager,
        SerialPacketManager& serial,
        Network& network,
        AudioChip& audio);
    ~PlainChatView() = default;

    void SetActiveRoom(const struct Room &room);

protected:
    void AnimatedDraw();
    void Draw();
    void Update(uint32_t current_tick);
    void HandleInput();

private:
    void DrawTitle();
    void DrawUsrInputSeperator();
    void DrawMessages();
    void IngestPlainMessages();
    void PushMessage(std::string&& msg);
    void SendPacket(std::string& msg);
    void SendAudio(uint32_t current_tick);

    // Consts
    const std::string name_seperator = ": ";
    bool send_once = false;

    // Decrypted messages
    std::vector<std::string> messages;
    bool redraw_messages = true;

    struct
    {
        uint16_t timestamp_colour = C_CYAN;
        uint16_t name_colour = C_BLUE;
        uint16_t body_colour = C_WHITE;
    } settings;

    int8_t last_audio_buffer_used;
};
