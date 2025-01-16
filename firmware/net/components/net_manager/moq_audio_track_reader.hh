// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __MOQ_AUDIO_TRACK_READER__
#define __MOQ_AUDIO_TRACK_READER__

#include <deque>

#include <quicr/client.h>

namespace moq
{
    class AudioTrackReader : public quicr::SubscribeTrackHandler
    {
    public:
        AudioTrackReader(const quicr::FullTrackName &full_track_name, size_t min_depth = 10, size_t max_depth = std::numeric_limits<size_t>::max());

        virtual ~AudioTrackReader() = default;

        void ObjectReceived(const quicr::ObjectHeaders &headers, quicr::BytesSpan data) override;

        void StatusChanged(Status status) override;

        void Pause() { _playing = false;}
        void Play() { _playing = _buffer.size() >= _min_depth; }

        bool IsPlaying() const noexcept { return _playing; }

        quicr::BytesSpan Front() noexcept;
        void Pop() noexcept { _buffer.pop(); }
        std::optional<std::vector<uint8_t>> PopFront() noexcept;

    private:
        std::queue<std::vector<uint8_t>> _buffer;
        bool _playing = false;
        size_t _min_depth = 50;
        size_t _max_depth = std::numeric_limits<size_t>::max();
    };
}

#endif
