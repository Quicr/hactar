#pragma once

#include "packet_t.hh"
#include "audio_codec.hh"
#include "audio_chip.hh"

#include <cstring>

namespace ui_net_link
{
    enum struct Packet_Type: uint8_t
    {
        TalkStart,
        TalkStop,
        PlayStart,
        PlayStop,
        MoQDisconnected,
        MoQConnecting,
        MoQReady,
        AudioObject,
        AudioObjects,
        WifiConnected,
        WifiDisconnected,
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
        uint8_t data[constants::Audio_Buffer_Sz_2];
    };

    void Serialize(const TalkStart& talk_start, packet_t& packet)
    {   
        packet.length = 4;

        // Packet type
        packet.payload[0] = (uint8_t)Packet_Type::TalkStart;

        // Channel id
        packet.payload[1] = talk_start.channel_id;

        packet.is_ready = true;
    }

    static void Serialize(const TalkStop& talk_stop, packet_t& packet)
    {
        packet.length = 4;

        // Packet type
        packet.payload[0] = (uint8_t)Packet_Type::TalkStop;

        // Channel id
        packet.payload[1] = talk_stop.channel_id;

        packet.is_ready = true;
    }

    static void Serialize(const PlayStart& play_start, packet_t& packet)
    {   
        packet.length = 4;

        // Packet type
        packet.payload[0] = (uint8_t)Packet_Type::PlayStart;

        // Channel id
        packet.payload[1] = play_start.channel_id;
        
        packet.is_ready = true;
    }

    static void Serialize(const PlayStop& play_stop, packet_t& packet)
    {
        packet.length = 4;

        // Packet type
        packet.payload[0] = (uint8_t)Packet_Type::PlayStop;

        // Channel id
        packet.payload[1] = play_stop.channel_id;

        packet.is_ready = true;
    }

    static void Serialize(const AudioObject& talk_frame, packet_t& packet)
    {
        const uint16_t num_extra_byes = 2;
        packet.length = Packet_Header_Size + num_extra_byes + constants::Audio_Buffer_Sz_2;
        packet.payload[0] = (uint8_t)Packet_Type::AudioObjects;
        packet.payload[1] = talk_frame.channel_id;

        constexpr uint32_t payload_offset = num_extra_byes; 
        memcpy(packet.payload+payload_offset, talk_frame.data, constants::Audio_Buffer_Sz_2);

        packet.is_ready = true;
    }

    static void Deserialize(const packet_t& packet, TalkStart& talk_start)
    {
        talk_start.channel_id = packet.payload[1];
    }

    static void Deserialize(const packet_t& packet, TalkStop& talk_stop)
    {
        talk_stop.channel_id = packet.payload[1];
    }

    static void Deserialize(const packet_t& packet, PlayStart& play_start)
    {
        play_start.channel_id = packet.payload[1];      
    }

    static void Deserialize(const packet_t& packet, PlayStop& play_stop)
    {
        play_stop.channel_id = packet.payload[1];
    }

    static void Deserialize(const packet_t& packet, AudioObject& audio_object)
    {
        audio_object.channel_id = packet.payload[1];
        
        constexpr uint32_t payload_offset = 2; 
        memcpy(audio_object.data, packet.payload + payload_offset, constants::Audio_Buffer_Sz_2); 
    }

    // TODO serialize and deserialize for audioobjectS
}