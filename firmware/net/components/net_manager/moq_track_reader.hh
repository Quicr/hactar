// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __MOQ_TRACK_READER__
#define __MOQ_TRACK_READER__

#include <deque>

#include <quicr/client.h>

namespace moq {

    class TrackReader : public quicr::SubscribeTrackHandler {
    public:
        TrackReader(const quicr::FullTrackName &full_track_name)
                : SubscribeTrackHandler(full_track_name,
                                        3,
                                        quicr::messages::GroupOrder::kAscending,
                                        quicr::messages::FilterType::kLatestObject) {}

        virtual ~TrackReader() = default;

        //
        // overrides from SubscribeTrackHandler
        //
        void ObjectReceived(const quicr::ObjectHeaders &headers, quicr::BytesSpan data) override;

        void StatusChanged(Status status) override;

        void HandleMessageChunk(const quicr::ObjectHeaders& headers, quicr::BytesSpan data);

        void AudioPlay();
        void AudioPause();

        bool AudioPlaying() const noexcept;

        quicr::BytesSpan AudioFront() noexcept;
        void AudioPop() noexcept;
        std::optional<std::vector<uint8_t>> AudioPopFront() noexcept;
        size_t AudioNumAvailable() noexcept;

    private:
        // Audio variables
        // TODO move into an audio object?
        std::queue<std::vector<uint8_t>> _audio_buffer;
        bool _audio_playing = false;
        size_t _audio_min_depth = 50;
        size_t _audio_max_depth = std::numeric_limits<size_t>::max();
        //

        uint32_t ai_request_id = 0;
        uint64_t num_print = 0;
        uint64_t num_recv = 0;
    };

}

#endif