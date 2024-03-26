#pragma once

#include "ViewInterface.hh"
#include <string>
#include "SerialPacket.hh"
#include "Message.hh"
#include "QChat.hh"

#include <optional>

#include <mls/state.h>

#define Margin_0 0

#define Padding_0 0
#define Padding_1 1
#define Padding_2 2
#define Padding_3 3

#define Text_Draw_Speed 20

using namespace mls;

struct MLSState;

struct PreJoinedState {
  SignaturePrivateKey identity_priv;
  HPKEPrivateKey init_priv;
  HPKEPrivateKey leaf_priv;
  LeafNode leaf_node;
  KeyPackage key_package;
  bytes key_package_data;

  PreJoinedState(const std::string& username);
  MLSState create();
  MLSState join(const bytes& welcome_data);
};

struct MLSState {
  bool should_commit() const;
  std::tuple<bytes, bytes> add(const bytes& key_package_data);
  void handle(const bytes& commit_data);

  bytes protect(const bytes& plaintext);
  bytes unprotect(const bytes& ciphertext);

  mls::State state;
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
    void SendPacket(const bytes& data);
    void PushMessage(std::string&& msg);

    // Consts
    const std::string name_seperator = ": ";

    // Decrypted messages
    std::vector<std::string> messages;
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
