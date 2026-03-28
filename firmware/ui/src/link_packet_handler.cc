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
        response->chunk_data[response->chunk_length] = 0;
        UI_LOG_INFO("[AI] %s", response->chunk_data);
        break;
    }
    }
}

void HandleNetLinkPackets(Serial& serial, Protector& protector, AudioChip& audio, Screen& screen)
{
    while (true)
    {
        link_packet_t* packet = serial.Read();
        if (!packet)
        {
            return;
        }

        if (packet->type != static_cast<uint16_t>(ui_net_link::NetToUi::AudioFrame))
        {
            UI_LOG_ERROR("Unhandled packet type %d", (int)packet->type);
            continue;
        }

        if (!protector.TryUnprotect(packet))
        {
            UI_LOG_ERROR("Failed to decrypt ptt object");
            continue;
        }

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
            break;
        }
        case ui_net_link::MessageType::AIResponse:
        {
            HandleAiResponse(packet, audio);
            break;
        }
        case ui_net_link::MessageType::Chat:
        {
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

        switch (static_cast<UiMgmtCmd>(packet->type))
        {
        case Ui_Cmd_Ping:
        {
            if (packet->length > 0)
            {
                serial.Reply(Ui_Resp_Pong,
                             std::span<const uint8_t>(packet->payload.data(), packet->length));
            }
            else
            {
                serial.Reply(Ui_Resp_Pong, std::span<const uint8_t>{});
            }
            break;
        }
        case Ui_Cmd_ClearStorage:
        {
            UI_LOG_INFO("OK! Clearing configurations");
            storage.Clear();
            UI_LOG_INFO("OK! Cleared all configurations");
            serial.Reply(Ui_Resp_Ack, std::span<const uint8_t>{});
            break;
        }
        case Ui_Cmd_SetSframeKey:
        {
            if (packet->length != 16)
            {
                UI_LOG_ERROR("ERR. Sframe key is too short!");
                serial.Reply(Ui_Resp_Error, std::span<const uint8_t>{});
                break;
            }

            if (storage.Save(ConfigStorage::Config_Id::Sframe_Key, packet->payload.data(),
                             packet->length))
            {
                UI_LOG_INFO("OK! Saved SFrame Key configuration");
                serial.Reply(Ui_Resp_Ack, std::span<const uint8_t>{});
            }
            else
            {
                UI_LOG_ERROR("ERR. Failed to save SFrame Key configuration");
                serial.Reply(Ui_Resp_Error, std::span<const uint8_t>{});
            }
            break;
        }
        case Ui_Cmd_GetSframeKey:
        {
            ConfigStorage::Config config = storage.Load(ConfigStorage::Config_Id::Sframe_Key);
            if (config.loaded && config.len == 16)
            {
                serial.Reply(Ui_Resp_SframeKey, std::span<const uint8_t>(config.buff, config.len));
            }
            else
            {
                serial.Reply(Ui_Resp_Error, std::span<const uint8_t>{});
            }
            break;
        }
        case Ui_Cmd_GetStackInfo:
        {
            stack_debug::StackInfo info = stack_debug::GetStackInfo();
            char json[128];
            int len = snprintf(
                json, sizeof(json),
                "{\"stack_base\":%lu,\"stack_top\":%lu,\"stack_size\":%lu,\"stack_used\":%lu}",
                info.stack_base, info.stack_top, info.stack_size, info.stack_used);
            serial.Reply(Ui_Resp_StackInfo,
                         std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(json), len));
            break;
        }
        case Ui_Cmd_RepaintStack:
        {
            stack_debug::RepaintStack();
            serial.Reply(Ui_Resp_Ack, std::span<const uint8_t>{});
            break;
        }
        case Ui_Cmd_GetLoopback:
        {
            uint8_t mode = static_cast<uint8_t>(UiLoopbackMode::Off);
            serial.Reply(Ui_Resp_Loopback, std::span<const uint8_t>(&mode, 1));
            break;
        }
        case Ui_Cmd_SetLoopback:
        {
            if (packet->length < 1)
            {
                serial.Reply(Ui_Resp_Error, std::span<const uint8_t>{});
                break;
            }
            auto mode = static_cast<UiLoopbackMode>(packet->payload[0]);
            if (mode == UiLoopbackMode::Off)
            {
                serial.Reply(Ui_Resp_Ack, std::span<const uint8_t>{});
            }
            else
            {
                UI_LOG_WARN("Loopback mode %d not supported", static_cast<int>(mode));
                serial.Reply(Ui_Resp_Error, std::span<const uint8_t>{});
            }
            break;
        }
        case Ui_Cmd_GetLogsEnabled:
        {
            uint8_t enabled = Logger::enabled ? 1 : 0;
            serial.Reply(Ui_Resp_LogsEnabled, std::span<const uint8_t>(&enabled, 1));
            break;
        }
        case Ui_Cmd_SetLogsEnabled:
        {
            if (packet->length < 1)
            {
                serial.Reply(Ui_Resp_Error, std::span<const uint8_t>{});
                break;
            }
            uint8_t enabled = packet->payload[0];
            if (enabled)
            {
                Logger::Enable();
            }
            else
            {
                Logger::Disable();
            }
            serial.Reply(Ui_Resp_Ack, std::span<const uint8_t>{});
            break;
        }
        default:
        {
            UI_LOG_ERROR("ERR. No handler for packet type 0x%04x", packet->type);
            break;
        }
        }
    }
}
