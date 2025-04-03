// #pragma once

// #include <nlohmann/json.hpp>

// using json = nlohmann::json;

// // default
// json default_channel_json = json::parse(R"(
// {
//     "endpoint_info": {
//         "id": "c637ae2a-b3d7-4574-bac7-8c7b5ee44f3e",
//         "owner": "alice@acme.com",
//         "lang-preference": "en-US"
//     },
//     "publications": [
//         {
//             "channel_name": "gardening",
//             "language": "en-US",
//             "tracknamespace": ["moq://moq.ptt.arpa/v1", "org/acme", "store/1234", "channel/gardening", "ptt"],
//             "trackname": "pcm_en_8khz_mono_i16",
//             "codec":"pcm",
//             "samplerate":8000,
//             "channelConfig":"1"
//         },
//         {
//             "channel_name": "ai_audio",
//             "language" : "en-US",
//             "tracknamespace": ["moq://moq.ptt.arpa/v1", "org/acme", "store/1234", "ai/audio"],
//             "trackname": "pcm_en_8khz_mono_i16",
//             "codec": "pcm",
//             "samplerate":8000,
//             "channelConfig":"1"
//         }
//     ],
//     "subscriptions": [
//         {
//             "channel_name": "gardening",
//             "language" : "en-US",
//             "tracknamespace": ["moq://moq.ptt.arpa/v1", "org/acme", "store/1234", "channel/gardening", "ptt"],
//             "trackname": "pcm_en_8khz_mono_i16",
//             "codec": "pcm",
//             "samplerate":8000,
//             "channelConfig":"1"
//         },
//         {
//             "channel_name": "self_ai_audio",
//             "language" : "en-US",
//             "tracknamespace": ["moq://moq.ptt.arpa/v1", "org/acme", "store/1234", "ai/audio"],
//             "trackname": "",
//             "codec": "pcm",
//             "samplerate":8000,
//             "channelConfig":"1"
//         },
//         {
//             "channel_name": "self_ai_text",
//             "tracknamespace": ["moq://moq.ptt.arpa/v1", "org/acme", "store/1234", "ai/text"],
//             "trackname": "",
//             "codec": "ai_cmd_response:json"
//         }
//     ]
// }
// )");