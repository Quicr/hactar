#pragma once

#include "link_packet_t.hh"
#include "constants.hh"

#include <cstring>

namespace ui_net_link
{
    enum struct Packet_Type: uint8_t
    {
        PowerOnReady,
        GetAudioLinkPacket,
        GetTextLinkPacket,
        TalkStart,
        TalkStop,
        PlayStart,
        PlayStop,
        MoQStatus,
        MoQChangeNamespace,
        AudioObject,
        AudioMultiObject,
        SSIDRequest,
        WifiConnect,
        WifiStatus
    };

    struct ChangeNamespace
    {

        uint8_t channel_id;
        uint16_t trackname_len;
        char trackname[128]; // Todo get actual max size of a trackname
    };

    struct TalkStart
    {
        uint8_t channel_id;
    };

    struct TalkStop
    {
        uint8_t channel_id;
    };

    struct PlayStart
    {
        uint8_t channel_id;
    };

    struct PlayStop
    {
        uint8_t channel_id;
    };

    struct AudioObject
    {
        uint8_t channel_id;
        uint8_t data[constants::Audio_Phonic_Sz];
    };


    [[maybe_unused]] static void BuildGetLinkPacket(uint8_t* buff)
    {
        buff[0] = (uint8_t)Packet_Type::GetAudioLinkPacket;
        buff[1] = 0;
        buff[2] = 0;
    }

    [[maybe_unused]] static void Serialize(const TalkStart& talk_start, link_packet_t& packet)
    {
        packet.type = (uint8_t)Packet_Type::TalkStart;
        packet.length = 1;

        // Channel id
        packet.payload[0] = talk_start.channel_id;

        packet.is_ready = true;
    }

    [[maybe_unused]] static void Serialize(const TalkStop& talk_stop, link_packet_t& packet)
    {
        packet.type = (uint8_t)Packet_Type::TalkStop;
        packet.length = 1;

        // Channel id
        packet.payload[0] = talk_stop.channel_id;

        packet.is_ready = true;
    }

    [[maybe_unused]] static void Serialize(const PlayStart& play_start, link_packet_t& packet)
    {
        packet.type = (uint8_t)Packet_Type::PlayStart;
        packet.length = 1;

        // Channel id
        packet.payload[0] = play_start.channel_id;

        packet.is_ready = true;
    }

    [[maybe_unused]] static void Serialize(const PlayStop& play_stop, link_packet_t& packet)
    {
        packet.type = (uint8_t)Packet_Type::PlayStop;
        packet.length = 1;

        // Channel id
        packet.payload[0] = play_stop.channel_id;

        packet.is_ready = true;
    }

    [[maybe_unused]] static void Serialize(const AudioObject& talk_frame, link_packet_t& packet)
    {
        const uint16_t num_extra_bytes = 1;

        packet.type = (uint8_t)Packet_Type::AudioObject;
        packet.length = num_extra_bytes + constants::Audio_Phonic_Sz;
        packet.payload[0] = talk_frame.channel_id;

        constexpr uint32_t payload_offset = num_extra_bytes;
        memcpy(packet.payload+payload_offset, talk_frame.data, constants::Audio_Phonic_Sz);

        packet.is_ready = true;
    }

    [[maybe_unused]] static void Serialize(const ChangeNamespace& change_moq_namespace_frame, link_packet_t& packet)
    {
        const uint16_t num_extra_bytes = sizeof(change_moq_namespace_frame.channel_id) + sizeof(change_moq_namespace_frame.trackname_len);
        uint32_t payload_offset = 0;

        packet.type = (uint8_t)Packet_Type::MoQChangeNamespace;
        packet.length = num_extra_bytes + change_moq_namespace_frame.trackname_len;

        memcpy(packet.payload, &change_moq_namespace_frame.channel_id, sizeof(change_moq_namespace_frame.channel_id));
        payload_offset += sizeof(change_moq_namespace_frame.channel_id);

        memcpy(packet.payload+payload_offset, &change_moq_namespace_frame.trackname_len, sizeof(change_moq_namespace_frame.trackname_len));
        payload_offset += sizeof(change_moq_namespace_frame.trackname_len);

        memcpy(packet.payload+payload_offset, change_moq_namespace_frame.trackname, change_moq_namespace_frame.trackname_len);

        packet.is_ready = true;
    }

    [[maybe_unused]] static void Deserialize(const link_packet_t& packet, TalkStart& talk_start)
    {
        talk_start.channel_id = packet.payload[0];
    }

    [[maybe_unused]] static void Deserialize(const link_packet_t& packet, TalkStop& talk_stop)
    {
        talk_stop.channel_id = packet.payload[0];
    }

    [[maybe_unused]] static void Deserialize(const link_packet_t& packet, PlayStart& play_start)
    {
        play_start.channel_id = packet.payload[0];
    }

    [[maybe_unused]] static void Deserialize(const link_packet_t& packet, PlayStop& play_stop)
    {
        play_stop.channel_id = packet.payload[0];
    }

    [[maybe_unused]] static void Deserialize(const link_packet_t& packet, AudioObject& audio_object)
    {
        audio_object.channel_id = packet.payload[0];

        constexpr uint32_t payload_offset = 1;
        memcpy(audio_object.data, packet.payload + payload_offset, constants::Audio_Phonic_Sz);
    }

    [[maybe_unused]] static void Deserialize(const link_packet_t& packet, ChangeNamespace& change_moq_namespace_frame)
    {
        memcpy(&change_moq_namespace_frame.channel_id, packet.payload, sizeof(change_moq_namespace_frame.channel_id));
        uint32_t payload_offset = sizeof(change_moq_namespace_frame.channel_id);

        memcpy(&change_moq_namespace_frame.trackname_len, packet.payload+payload_offset, sizeof(change_moq_namespace_frame.trackname_len));
        payload_offset += sizeof(change_moq_namespace_frame.trackname_len);

        memcpy(change_moq_namespace_frame.trackname, packet.payload + payload_offset, change_moq_namespace_frame.trackname_len);
    }

    // TODO serialize and deserialize for audioobjectS
}