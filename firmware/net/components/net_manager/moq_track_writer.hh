// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __MOQ_TRACK_WRITER__
#define __MOQ_TRACK_WRITER__

#include <quicr/publish_track_handler.h>

namespace moq {

    class TrackWriter : public quicr::PublishTrackHandler
    {
    public:
        TrackWriter(const quicr::FullTrackName& full_track_name,
                            quicr::TrackMode track_mode,
                            uint8_t default_priority,
                            uint32_t default_ttl)
        : quicr::PublishTrackHandler(full_track_name, track_mode, default_priority, default_ttl)
        {
        }

        virtual ~TrackWriter() = default;

        void StatusChanged(Status status) override;
    };

}

#endif