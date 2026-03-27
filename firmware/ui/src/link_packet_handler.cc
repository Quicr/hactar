#include "link_packet_handler.hh"
#include "audio_chip.hh"
#include "audio_codec.hh"
#include "keyboard_display.hh"
#include "logger.hh"
#include "stack_debug.hh"
#include "ui_mgmt_link.h"
#include "ui_net_link.hh"
#include <cstdio>
#include <span>

static void HandleMedia(link_packet_t* packet, AudioChip& audio)
{
    ui_net_link::AudioObject play_frame;
    ui_net_link::Deserialize(*packet, play_frame);
    AudioCodec::ALawExpand(play_frame.data, constants::Audio_Phonic_Sz, audio.TxBuffer(),
                           constants::Audio_Buffer_Sz, constants::Stereo, true);
}

static void HandleAiResponse(link_packet_t* packet, AudioChip& audio)
{
    auto* response =
        static_cast<ui_net_link::AIResponseChunk*>(static_cast<void*>(packet->payload.data() + 1));

    switch (response->content_type)
    {
    case ui_net_link::ContentType::Audio:
    {
        const uint16_t len = constants::Audio_Phonic_Sz < response->chunk_length
                               ? constants::Audio_Phonic_Sz
                               : response->chunk_length;

        AudioCodec::ALawExpand(response->chunk_data, len, audio.TxBuffer(),
                               constants::Audio_Buffer_Sz, constants::Stereo, true);
        break;
    }
    case ui_net_link::ContentType::Json:
    {
        // This is a text.
        response->chunk_data[response->chunk_length] = 0;
        UI_LOG_INFO("[AI] %s", response->chunk_data);
        break;
    }
    }
}

void HandleNetLinkPackets(Serial& serial, Protector& protector, AudioChip& audio, Screen& screen)
{
    // If there are bytes available read them
    while (true)
    {
        link_packet_t* packet = serial.Read();
        if (!packet)
        {
            return;
        }

        if ((ui_net_link::Packet_Type)packet->type != ui_net_link::Packet_Type::Message)
        {
            UI_LOG_ERROR("Unhandled packet type %d", (int)packet->type);
            continue;
        }

        if (!protector.TryUnprotect(packet))
        {
            UI_LOG_ERROR("Failed to decrypt ptt object");
            continue;
        }

        // Get the second byte of the data which is the message type
        // Since the first byte is channel id
        const auto message_type = static_cast<ui_net_link::MessageType>(packet->payload[1]);

        switch (message_type)
        {
        case ui_net_link::MessageType::Media:
        {
            HandleMedia(packet, audio);
            break;
        }
        case ui_net_link::MessageType::AIRequest:
        {
            // Do nothing.
            break;
        }
        case ui_net_link::MessageType::AIResponse:
        {
            // Json, text, or ai audio
            HandleAiResponse(packet, audio);
            break;
        }
        case ui_net_link::MessageType::Chat:
        {
            // Text/translated text
            HandleChatMessages(screen, packet);
            break;
        }
        default:
        {
            UI_LOG_INFO("Got an unknown message type %d", (int)message_type);
            break;
        }
        }
    }
}

void HandleMgmtLinkPackets(Serial& serial, ConfigStorage& storage)
{
    while (true)
    {
        link_packet_t* packet = serial.Read();
        if (!packet)
        {
            break;
        }

        switch (packet->type)
        {
        case Configuration::Ping:
        {
            // Echo the payload back (pong)
            if (packet->length > 0)
            {
                serial.Write(packet->payload.data(), packet->length);
            }
            else
            {
                serial.ReplyAck();
            }
            break;
        }
        case Configuration::Clear:
        {
            // NOTE, the storage clear WILL brick your hactar for some amount of time until
            // the eeprom fixes itself
            UI_LOG_INFO("OK! Clearing configurations");
            storage.Clear();
            UI_LOG_INFO("OK! Cleared all configurations");
            break;
        }
        case Configuration::Set_Sframe_Key:
        {
            if (packet->length != 16)
            {
                UI_LOG_ERROR("ERR. Sframe key is too short!");
                serial.ReplyNack();
                break;
            }

            if (storage.Save(ConfigStorage::Config_Id::Sframe_Key, packet->payload.data(),
                             packet->length))
            {
                UI_LOG_INFO("OK! Saved SFrame Key configuration");
                serial.ReplyAck();
            }
            else
            {
                UI_LOG_ERROR("ERR. Failed to save SFrame Key configuration");
                serial.ReplyNack();
            }

            break;
        }
        case Configuration::Get_Sframe_Key:
        {
            ConfigStorage::Config config = storage.Load(ConfigStorage::Config_Id::Sframe_Key);
            if (config.loaded && config.len == 16)
            {
                serial.Reply(Configuration::Response_SframeKey,
                             std::span<const uint8_t>(config.buff, config.len));
            }
            else
            {
                serial.ReplyError();
            }
            break;
        }
        case Configuration::Toggle_Logs:
        {
            serial.ReplyAck();
            Logger::Toggle();
            break;
        }
        case Configuration::Disable_Logs:
        {
            serial.ReplyAck();
            Logger::Disable();
            break;
        }
        case Configuration::Enable_Logs:
        {
            serial.ReplyAck();
            Logger::Enable();
            break;
        }
        case Configuration::Get_Stack_Info:
        {
            stack_debug::StackInfo info = stack_debug::GetStackInfo();
            char json[128];
            int len = snprintf(
                json, sizeof(json),
                "{\"stack_base\":%lu,\"stack_top\":%lu,\"stack_size\":%lu,\"stack_used\":%lu}",
                info.stack_base, info.stack_top, info.stack_size, info.stack_used);
            serial.Reply(Configuration::Response_StackInfo,
                         std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(json), len));
            break;
        }
        case Configuration::Repaint_Stack:
        {
            stack_debug::RepaintStack();
            serial.ReplyAck();
            break;
        }
        case Configuration::Get_Loopback:
        {
            // Return current loopback mode (always Off - not implemented)
            uint8_t mode = static_cast<uint8_t>(UiLoopbackMode::Off);
            serial.Reply(Configuration::Response_Loopback, std::span<const uint8_t>(&mode, 1));
            break;
        }
        case Configuration::Set_Loopback:
        {
            if (packet->length < 1)
            {
                serial.ReplyNack();
                break;
            }
            auto mode = static_cast<UiLoopbackMode>(packet->payload[0]);
            if (mode == UiLoopbackMode::Off)
            {
                // Off is the only supported mode (loopback not implemented)
                serial.ReplyAck();
            }
            else
            {
                // Raw, Alaw, Sframe not implemented
                UI_LOG_WARN("Loopback mode %d not supported", static_cast<int>(mode));
                serial.ReplyNack();
            }
            break;
        }
        case Configuration::Get_Logs_Enabled:
        {
            uint8_t enabled = Logger::enabled ? 1 : 0;
            serial.Reply(Configuration::Response_LogsEnabled,
                         std::span<const uint8_t>(&enabled, 1));
            break;
        }
        case Configuration::Set_Logs_Enabled:
        {
            if (packet->length < 1)
            {
                serial.ReplyNack();
                break;
            }
            uint8_t enabled = packet->payload[0];
            if (enabled)
            {
                serial.ReplyAck();
                Logger::Enable();
            }
            else
            {
                serial.ReplyAck();
                Logger::Disable();
            }
            break;
        }
        default:
        {
            UI_LOG_ERROR("ERR. No handler for received packet type");
            break;
        }
        }
    }
}
