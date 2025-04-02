// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#include "moq_track_reader.hh"
#include "utils.hh"
#include "moq_session.hh"

using namespace moq;

extern uint64_t device_id;
extern bool loopback;

void
TrackReader::ObjectReceived(const quicr::ObjectHeaders& headers, quicr::BytesSpan data)
{
    // SPDLOG_INFO("group id {} device id {}", headers.group_id, device_id);
    if (!loopback && headers.group_id == device_id)
    {
        return;
    }

    // SPDLOG_INFO("Object Received ({}): {}", headers.object_id, to_hex({ data.begin(), data.end() }));

    // Parse the data
    size_t offset = 1;
    switch ((moq::MessageType)data[0])
    {
        case moq::MessageType::Media:
        {
            // Since the content type for media is ALWAYS audio then
            // we assume it is audio and skip the first byte which is the
            // chunk type.
            // Let the task that sends out the audio data handle
            // the rest of the parsing
            // Skip the first byte
            _audio_buffer.emplace(data.begin() + offset, data.end());
            break;
        }
        case moq::MessageType::AIResponse:
        {
            uint32_t request_id = 0;
            std::memcpy(&request_id, data.data() + offset, sizeof(request_id));
            offset += sizeof(request_id);

            switch ((moq::Content_Type)data[offset])
            {
                case moq::Content_Type::Audio:
                {
                    offset += 1;
                    _audio_buffer.emplace(data.begin() + offset, data.end());
                    break;
                }
                case moq::Content_Type::Json:
                {
                    // TODO
                    break;
                }
                default:
                {
                    break;
                }

            }
            break;
        }
        default:
        {

            break;
        }
    }
}

void
TrackReader::StatusChanged(TrackReader::Status status)
{
    switch (status)
    {
        case Status::kOk:
        {
            if (auto track_alias = GetTrackAlias(); track_alias.has_value())
            {
                SPDLOG_INFO("Reader:Track {0} with alias: {1} is ready to read",
                    Stringify(GetFullTrackName()),
                    track_alias.value());
            }
        } break;
        default:
            break;
    }
}


void moq::TrackReader::AudioPlay()
{
    _audio_playing = _audio_playing || (!_audio_buffer.empty() && _audio_buffer.size() >= _audio_min_depth);
}

void moq::TrackReader::AudioPause()
{
    _audio_playing = false;
}

bool moq::TrackReader::AudioPlaying() const noexcept
{
    return _audio_playing;
}

quicr::BytesSpan moq::TrackReader::AudioFront() noexcept
{
    if (!_audio_playing || _audio_buffer.empty())
    {
        return quicr::BytesSpan();
    }

    return _audio_buffer.front();
}

void moq::TrackReader::AudioPop() noexcept
{
    _audio_buffer.pop();
}

std::optional<std::vector<uint8_t>> moq::TrackReader::AudioPopFront() noexcept
{
    if (!_audio_playing || _audio_buffer.empty())
    {
        return std::optional<std::vector<uint8_t>>();
    }

    std::optional<std::vector<uint8_t>> value = std::move(_audio_buffer.front());
    _audio_buffer.pop();
    return value;
}


size_t moq::TrackReader::AudioNumAvailable() noexcept
{
    return _audio_buffer.size();
}