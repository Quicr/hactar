
#include "chat_view.hh"
#include "user_interface_manager.hh"
#include "team_view.hh"
#include "settings_view.hh"
#include "qchat.hh"
#include <string>

#include "TitleBar.hh"

static const CipherSuite cipher_suite = CipherSuite{
  CipherSuite::ID::P256_AES128GCM_SHA256_P256
};

static const bytes group_id = from_ascii("group_id");

enum struct MlsMessageType: uint8_t
{
    key_package = 1,
    welcome = 2,
    commit = 3,
    message = 4,
};

static bytes frame(MlsMessageType msg_type, const bytes& msg)
{
    const auto msg_type_8 = static_cast<uint8_t>(msg_type);
    const auto type_vec = std::vector<uint8_t>(1, msg_type_8);
    return bytes(type_vec) + msg;
}

static std::pair<MlsMessageType, bytes> unframe(const bytes& framed)
{
    const auto& data = framed.as_vec();
    const auto msg_type = static_cast<MlsMessageType>(data.at(0));
    const auto msg_data = bytes(std::vector<uint8_t>(data.begin() + 1, data.end()));
    return { msg_type, msg_data };
}

PreJoinedState::PreJoinedState(const std::string& username)
    : identity_priv(SignaturePrivateKey::generate(cipher_suite))
    , init_priv(HPKEPrivateKey::generate(cipher_suite))
    , leaf_priv(HPKEPrivateKey::generate(cipher_suite))
{
    auto credential = Credential::basic(from_ascii(username));
    leaf_node = LeafNode{ cipher_suite,
                          leaf_priv.public_key,
                          identity_priv.public_key,
                          credential,
                          Capabilities::create_default(),
                          Lifetime::create_default(),
                          {},
                          identity_priv };

    key_package = KeyPackage{ cipher_suite,
                              init_priv.public_key,
                              leaf_node,
                              {},
                              identity_priv };

    key_package_data = tls::marshal(key_package);
}

MLSState PreJoinedState::create()
{
    return { State{ group_id,
                    cipher_suite,
                    leaf_priv,
                    identity_priv,
                    leaf_node,
                    {} } };
}

MLSState PreJoinedState::join(const bytes& welcome_data)
{
    const auto welcome = tls::get<Welcome>(welcome_data);
    return MLSState{ State{ init_priv,
                            leaf_priv,
                            identity_priv,
                            key_package,
                            welcome,
                            std::nullopt,
                            {} } };
}

bool MLSState::should_commit() const
{
    Logger::Log(Logger::Level::Debug, "[MLS] should commit?", state.index().val);
    return state.index() == LeafIndex{ 0 };
}

std::tuple<bytes, bytes> MLSState::add(const bytes& key_package_data)
{
    const auto fresh_secret = random_bytes(32);
    const auto key_package = tls::get<KeyPackage>(key_package_data);
    const auto add = state.add_proposal(key_package);
    auto [commit, welcome, next_state] =
        state.commit(fresh_secret, CommitOpts{ { add }, true, false, {} }, {});

    state = std::move(next_state);
    return { tls::marshal(commit), tls::marshal(welcome) };
}

void MLSState::handle(const bytes& commit_data)
{
    const auto commit = tls::get<MLSMessage>(commit_data);
    auto maybe_next_state = state.handle(commit);
    state = std::move(maybe_next_state.value());
}

bytes MLSState::protect(const bytes& plaintext)
{
    const auto private_message = state.protect({}, plaintext, 0);
    return tls::marshal(private_message);
}

bytes MLSState::unprotect(const bytes& ciphertext)
{
    const auto private_message = tls::get<MLSMessage>(ciphertext);
    const auto [aad, pt] = state.unprotect(private_message);
    return pt;
}

ChatView::ChatView(UserInterfaceManager& manager,
    Screen& screen,
    Q10Keyboard& keyboard,
    SettingManager& setting_manager,
    SerialPacketManager& serial,
    Network& network,
    AudioChip& audio
):
    ViewInterface(manager, screen, keyboard, setting_manager, serial, network, audio),
    pre_joined_state(PreJoinedState(*setting_manager.Username())),
    last_audio_buffer_used(1)
{
    redraw_messages = true;
    const auto framed = frame(MlsMessageType::key_package,
        pre_joined_state->key_package_data);
    SendPacket(framed);
}

void ChatView::Update(uint32_t current_tick)
{
    if (keyboard.MicPressed())
    {
        SendAudio(current_tick);
    }
}

void ChatView::AnimatedDraw()
{
}

void ChatView::Draw()
{
    ViewInterface::Draw();

    if (redraw_menu)
    {
        if (first_load)
        {
            DrawUsrInputSeperator();
            // DrawTitle();
            DrawTitleBar(manager.ActiveRoom()->friendly_name,
                menu_font, C_WHITE, C_BLACK, screen);
            first_load = false;
        }
    }

    // TODO move into ViewInterface?
    if (usr_input.length() > last_drawn_idx || redraw_input)
    {
        ViewInterface::DrawInputString(usr_input.substr(last_drawn_idx));
        last_drawn_idx = usr_input.length();
    }

    if (manager.HasNewMessages() || redraw_messages)
    {
        // TODO draw arrows and the ability to scroll messages.
        // I have a feeling this is gonna be slow as ever.

        DrawMessages();

        redraw_messages = false;
    }
}

void ChatView::HandleInput()
{
    // Check if this is a command
    if (usr_input[0] == '/')
    {
        ChangeView(usr_input);
        return;
    }

    // TODO switch to using C strings so we don;t need to extend
    // strings for each added character
    // and then we can just memcpy?
    const std::string& username = *setting_manager.Username();
    std::string plaintext = "00:00 ";
    plaintext += username;
    plaintext += ": ";
    plaintext += usr_input;

    // If we have MLS state, encrypt and send the message
    if (mls_state)
    {
        // Encrypt the message
        const auto plaintext_data = from_ascii(plaintext);
        const auto ciphertext = mls_state->protect(plaintext_data);
        const auto framed = frame(MlsMessageType::message, ciphertext);

        Logger::Log(Logger::Level::Info, "Send mls message");

        // Send the message out on the wire
        SendPacket(framed);
    }

    // Keep a copy of the message for display
    PushMessage(std::move(plaintext));
}

void ChatView::SendPacket(const bytes& msg)
{
    // prepare ascii message, encode into Message + Packet
    // XXX(RLB): This should use a null-safe message type.
    qchat::Ascii ascii{
        manager.ActiveRoom()->room_uri,
        std::string { msg.begin(), msg.end() }
    };

    // TODO move into encode...
    // TODO packet should maybe have a static next_packet_id?
    std::unique_ptr<SerialPacket> packet = std::make_unique<SerialPacket>(HAL_GetTick(), 1);
    packet->SetData(SerialPacket::Types::QMessage, 0, 1);
    packet->SetData(serial.NextPacketId(), 1, 2);

    // The packet length is set in the encode function
    // TODO encode probably could just generate a packet instead...
    qchat::Codec::encode(packet, 3, ascii);

    uint64_t new_offset = packet->NumBytes();
    // Expiry time
    packet->SetData(0xFFFFFFFF, new_offset, 4);
    new_offset += 4;

    // Creation time
    packet->SetData(0, new_offset, 4);
    new_offset += 4;

    serial.EnqueuePacket(std::move(packet));
}

void ChatView::PushMessage(std::string&& msg)
{
    messages.push_back(std::move(msg));
    redraw_messages = true;
}

void ChatView::IngestMlsMessages()
try
{
    // TODO use serial instead of manager?
    const auto raw_messages = manager.TakeMessages();
    for (const auto& msg : raw_messages)
    {
        const auto msg_bytes = from_ascii(msg);
        const auto [msg_type, msg_data] = unframe(msg_bytes);

        Logger::Log(Logger::Level::Debug, "[MLS] Bytes", to_hex(msg_bytes));
        Logger::Log(Logger::Level::Debug, "[MLS] Type", int(msg_type));
        Logger::Log(Logger::Level::Debug, "[MLS] Data", to_hex(msg_data));

        switch (msg_type)
        {
            case MlsMessageType::key_package: {
                // If this is the initial creation, create the group
                if (pre_joined_state)
                {
                    Logger::Log(Logger::Level::Debug, "[MLS] Creating group");
                    auto state = pre_joined_state->create();
                    const auto [commit, welcome] = state.add(msg_data);

                    pre_joined_state = std::nullopt;
                    mls_state = std::move(state);

                    const auto framed_welcome = frame(MlsMessageType::welcome, welcome);
                    SendPacket(framed_welcome);
                    break;
                }

                // Otherwise, we should have MLS state ready
                if (!mls_state)
                {
                    Logger::Log(Logger::Level::Debug, "[MLS] Ignoring KeyPackage; no MLS state");
                    break;
                }

                if (!mls_state->should_commit())
                {
                    // We are not the committer
                    Logger::Log(Logger::Level::Debug, "[MLS] Ignoring KeyPackage; not the committer");
                    break;
                }

                Logger::Log(Logger::Level::Debug, "[MLS] Committing");
                const auto [commit, welcome] = mls_state->add(msg_data);

                const auto framed_welcome = frame(MlsMessageType::welcome, welcome);
                SendPacket(framed_welcome);

                const auto framed_commit = frame(MlsMessageType::commit, commit);
                SendPacket(framed_commit);
                break;
            }

            case MlsMessageType::welcome: {
                if (!pre_joined_state)
                {
                    // Can't join by welcome
                    Logger::Log(Logger::Level::Error, "[MLS] Ignoring Welcome; not pre-joined");
                    break;
                }

                Logger::Log(Logger::Level::Debug, "[MLS] Joining");
                mls_state = pre_joined_state->join(msg_data);
                pre_joined_state = std::nullopt;
                break;
            }

            case MlsMessageType::commit: {
                if (!mls_state)
                {
                    // Can't handle commits before join
                    Logger::Log(Logger::Level::Error, "[MLS] Ignoring Commit; no MLS state");
                    break;
                }

                Logger::Log(Logger::Level::Debug, "[MLS] Processing commit");
                mls_state->handle(msg_data);
                break;
            }

            case MlsMessageType::message: {
                // If we don't have MLS state, we can't decrypt
                if (!mls_state)
                {
                    Logger::Log(Logger::Level::Error, "[MLS] Ignoring message; no MLS state");
                    PushMessage("[decryption failure]");
                    break;
                }

                Logger::Log(Logger::Level::Debug, "[MLS] Decrypting message");
                auto plaintext = mls_state->unprotect(msg_data);
                auto plaintext_str = std::string(to_ascii(plaintext));
                PushMessage(std::move(plaintext_str));
                break;
            }
        }
    }
}
catch (const std::exception& e)
{
    Logger::Log(Logger::Level::Error, "[EXCEPT] Caught exception: ", e.what());
}

void ChatView::IngestPlainMessages()
{
    // TODO
}

void ChatView::DrawMessages()
{
    IngestMlsMessages();

    int32_t msg_idx = messages.size() - 1;

    // If there are no messages just return
    if (msg_idx < 0)
    {
        return;
    }

    const uint16_t y_window_start = (menu_font.height + Padding_3);
    const uint16_t y_window_end = screen.ViewHeight() - (usr_font.height + usr_font.height / 2);
    const uint16_t y_window_size = y_window_end - y_window_start;

    const uint16_t x_window_start = Padding_2;
    const uint16_t x_window_end = screen.ViewWidth() - Padding_2;
    const uint16_t x_window_size = x_window_end - x_window_start;

    uint16_t total_used_y_space = 0;
    uint16_t curr_used_y_space = 0;
    uint16_t prev_used_y_space = 0;

    uint16_t x_pos = x_window_start;
    uint16_t curr_used_x_space = x_pos;
    uint16_t next_used_x_space = 0;

    // Get the msg size of the most recent message in the list
    curr_used_x_space = x_window_start + messages[msg_idx].length()
        * usr_font.width;

    // Clip to screen width
    if (curr_used_x_space > screen.ViewWidth())
        curr_used_x_space = screen.ViewWidth();

    uint16_t y_pos;
    do
    {
        // The x_pos always starts a little offset
        x_pos = x_window_start;

        // Get the next message
        auto& msg = messages[msg_idx];

        // Swap the current used space and previous
        prev_used_y_space = curr_used_y_space;

        // Vertical space used to write the message
        curr_used_y_space = usr_font.height * (1 +
            (curr_used_x_space / x_window_size));

        // Sum the total spaced used
        total_used_y_space += curr_used_y_space;

        // Y position is calculated in reverse, so the total space we have
        // minus how much used so far
        y_pos = y_window_end - total_used_y_space;

        // Only calculate the next x space used on not last msgs
        if (msg_idx > 0)
        {
            // Calculate the amount of space based on the number of characters
            // used
            next_used_x_space = x_pos + (messages[msg_idx - 1].length()) * usr_font.width;

            if (next_used_x_space > screen.ViewWidth())
                next_used_x_space = screen.ViewWidth();
        }

        // Check to see if we need to clear characters
        // Clear the line.

        // TODO only clear the amount of space from the last line as needed
        // We can calculate that by taking the amount of x length and
        // modulus divide by the total window x space
        if (next_used_x_space > curr_used_x_space ||
            curr_used_y_space > usr_font.height)
        {
            uint16_t clipped_clear_box_y = y_pos;
            if (clipped_clear_box_y < y_window_start)
                clipped_clear_box_y = y_window_start;

            uint16_t clear_rec_x_start = curr_used_x_space;
            if (curr_used_y_space > usr_font.height)
                clear_rec_x_start = x_window_start;

            // We have a special case for the first box clear
            if (msg_idx == static_cast<int32_t>(messages.size()) - 1)
            {
                // Clear from the expected y position
                screen.FillRectangleAsync(clear_rec_x_start,
                    next_used_x_space,
                    clipped_clear_box_y,
                    y_pos + curr_used_y_space, bg);
            }
            else
            {
                screen.FillRectangleAsync(clear_rec_x_start,
                    next_used_x_space,
                    clipped_clear_box_y + (curr_used_y_space - prev_used_y_space),
                    y_pos + curr_used_y_space, bg);
            }
        }

        // Swap the curr x space with the next x space;
        curr_used_x_space = next_used_x_space;

        // Draw the body
        screen.DrawStringBoxAsync(x_pos, x_window_end,
            y_pos, y_window_end, msg, usr_font,
            settings.body_colour, bg, false);

        // Decrement the idx
        msg_idx--;
    } while (total_used_y_space < y_window_size && msg_idx >= 0);
}

void ChatView::DrawTitle()
{
    // Clear the title space
    screen.FillRectangleAsync(Margin_0, Margin_0, screen.ViewWidth(), menu_font.height + Padding_3, bg);

    // Draw horizontal seperator
    screen.FillRectangleAsync(Margin_0, menu_font.height + Padding_2, screen.ViewWidth(), menu_font.height + Padding_3, fg);

    // Draw team chat name
    screen.DrawStringAsync(Margin_0, Padding_2, "@CTO Team - Chat", menu_font, fg, bg, false);
}

void ChatView::DrawUsrInputSeperator()
{
    // Draw typing seperator
    screen.FillRectangleAsync(Margin_0,
        screen.ViewWidth(),
        screen.ViewHeight() - usr_font.height - Padding_3,
        screen.ViewHeight() - usr_font.height - Padding_2, fg);
}

void ChatView::SendAudio(uint32_t current_tick)
{
    // if (!mls_state)
    // {
    //     return;
    // }
    // if (audio.IsHalfComplete())
    // {
    //     // Get audio at the start
    //     const uint16_t* raw_buff = audio.GetRxBuffer(0);

    //     auto packet = std::move(qchat::Codec::encode(
    //         serial.NextPacketId(),
    //         manager.ActiveRoom()->room_uri,
    //         raw_buff,
    //         audio.AudioBufferSize_2(),
    //         current_tick
    //     ));

    //     serial.EnqueuePacket(std::move(packet));

    //     last_audio_buffer_used = 0;
    // }
    // if (audio.IsComplete())
    // {
    //     // Get the raw audio half way through
    //     const uint16_t* raw_buff = audio.GetRxBuffer(audio.AudioBufferSize_2());

    //     // Send wave
    //     auto packet = std::move(qchat::Codec::encode(
    //         serial.NextPacketId(),
    //         manager.ActiveRoom()->room_uri,
    //         raw_buff,
    //         audio.AudioBufferSize_2(),
    //         current_tick
    //     ));

    //     serial.EnqueuePacket(std::move(packet));

    //     last_audio_buffer_used = 1;
    // }
}