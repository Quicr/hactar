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
                                        quicr::messages::FilterType::LatestObject) {}

        virtual ~TrackReader() = default;

        //
        // overrides from SubscribeTrackHandler
        //
        void ObjectReceived(const quicr::ObjectHeaders &headers, quicr::BytesSpan data) override;

        void StatusChanged(Status status) override;

    private:
    };

}

#endif