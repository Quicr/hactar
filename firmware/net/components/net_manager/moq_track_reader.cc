// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#include "moq_track_reader.hh"
#include "utils.hh"

using namespace moq;

void
TrackReader::ObjectReceived(const quicr::ObjectHeaders& headers, quicr::BytesSpan data)
{
    SPDLOG_INFO("Object Received: {0}", to_hex({data.begin(), data.end()}));
}

void
TrackReader::StatusChanged(quicr::SubscribeTrackHandler::Status status)
{
    switch (status) {
        case Status::kOk: {
            if (auto track_alias = GetTrackAlias(); track_alias.has_value()) {
                SPDLOG_INFO("Reader:Track {0} with alias: {1} is ready to read",
                            Stringify(GetFullTrackName()),
                            track_alias.value());
            }
        } break;
        default:
            break;
    }
}
