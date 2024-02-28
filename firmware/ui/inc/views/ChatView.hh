#pragma once

#include "ViewInterface.hh"
#include "String.hh"
#include "SerialPacket.hh"
#include "Message.hh"
#include "QChat.hh"

#include <optional>

#define Margin_0 0

#define Padding_0 0
#define Padding_1 1
#define Padding_2 2
#define Padding_3 3

#define Text_Draw_Speed 20

struct MLSState;

struct PreJoinedState {
  PreJoinedState();
  String key_package();
  MLSState join(const String& welcome);
};

struct MLSState {
  static std::pair<String, MLSState> create(const String& key_package);
  String protect(const String& plaintext);
  String unprotect(const String& ciphertext);
};

class ChatView : public ViewInterface
{
public:
    ChatView(UserInterfaceManager& manager,
             Screen& screen,
             Q10Keyboard& keyboard,
             SettingManager& setting_manager);
    ~ChatView() = default;

    void SetActiveRoom(const struct Room &room);

protected:
    void AnimatedDraw();
    void Draw();
    void Update();
    void HandleInput();

private:
    void DrawTitle();
    void DrawUsrInputSeperator();
    void DrawMessages();
    void IngestMessages();
    void SendPacket(const String& data);
    void PushMessage(String&& msg);

    // Consts
    const String name_seperator = ": ";

    // Decrypted messages
    std::vector<String> messages;
    bool redraw_messages = true;

    struct
    {
        uint16_t timestamp_colour = C_CYAN;
        uint16_t name_colour = C_BLUE;
        uint16_t body_colour = C_WHITE;
    } settings;

    std::optional<PreJoinedState> pre_joined_state;
    std::optional<MLSState> mls_state;
};
