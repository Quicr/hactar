// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#include "moq_audio_track_reader.hh"
#include "utils.hh"

using namespace moq;

AudioTrackReader::AudioTrackReader(const quicr::FullTrackName &full_track_name, size_t min_depth, size_t max_depth)
    : SubscribeTrackHandler(full_track_name,
                            3,
                            quicr::messages::GroupOrder::kAscending,
                            quicr::messages::FilterType::LatestObject)
    , _min_depth(min_depth)
    , _max_depth(max_depth)
{
}

void
AudioTrackReader::ObjectReceived(const quicr::ObjectHeaders& headers, quicr::BytesSpan data)
{
    _buffer.emplace(data.begin(), data.end());
}

void
AudioTrackReader::StatusChanged(AudioTrackReader::Status status)
{
    switch (status) {
        case Status::kOk: {
            if (auto track_alias = GetTrackAlias(); track_alias.has_value()) {
                SPDLOG_INFO("AudioReaderTrack: {0} with alias: {1} is ready to read",
                            Stringify(GetFullTrackName()),
                            track_alias.value());
            }
        } break;
        default:
            break;
    }
}

quicr::BytesSpan moq::AudioTrackReader::Front() noexcept
{
    if (!_playing || _buffer.size() < _min_depth)
    {
        return quicr::BytesSpan();
    }

    return _buffer.front();
}

std::optional<std::vector<uint8_t>> moq::AudioTrackReader::PopFront() noexcept
{
    if (!_playing || _buffer.size() < _min_depth)
    {
        return std::optional<std::vector<uint8_t>>();
    }

    std::optional<std::vector<uint8_t>> value = std::move(_buffer.front());
    _buffer.pop();
    return value;
}
