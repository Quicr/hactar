#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// default
json default_channel_json = json::parse(R"(
{
    "endpoint_info": {
        "id": "c637ae2a-b3d7-4574-bac7-8c7b5ee44f3e",
        "owner": "alice@acme.com",
        "lang-preference": "es-US"
    },
    "publications": [
        {
            "channel_name": "gardening",
            "language": "es-US",
            "tracknamespace": ["moq://moq.ptt.arpa/v1", "org/acme", "store/1235", "channel/gardening", "ptt"],
            "trackname": "pcm_sp_16khz_mono_i16",
            "codec":"pcm",
            "samplerate":16000,
            "channelConfig":"1",
            "bitrate": 32000
        },
        {
            "channel_name": "voice_ai",
            "language" : "es-US",
            "tracknamespace": ["moq://moq.ptt.arpa/v1", "org/acme", "store/1235", "ai/audio"],
            "trackname": "pcm_sp_16khz_mono_i16",
            "codec": "pcm",
            "samplerate":16000,
            "channelConfig":"1",
            "bitrate": 32000
        }
    ],
    "subscriptions": [
        {
            "channel_name": "gardening",
            "language" : "es-US",
            "tracknamespace": ["moq://moq.ptt.arpa/v1", "org/acme", "store/1234", "channel/gardening", "ptt"],
            "trackname": "pcm_sp_16khz_mono_i16",
            "codec": "pcm",
            "samplerate":16000,
            "channelConfig":"1",
            "bitrate": 32000
        },
        {
            "channel_name": "self",
            "language" : "es-US",
            "tracknamespace": ["moq://moq.ptt.arpa/v1", "org/acme", "store/1234", "ai/audio"],
            "trackname": "<alice-device-id>",
            "codec": "pcm",
            "samplerate":16000,
            "channelConfig":"1",
            "bitrate": 32000
        },
        {
            "channel_name": "self",

            "tracknamespace": ["moq://moq.ptt.arpa/v1", "org/acme", "store/1234", "ai/text"],
            "trackname": "<alice-device-id>",
            "codec": "ai_cmd_response:json"
        }
    ]
}
)");