// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#include "moq_track_writer.hh"
#include "utils.hh"

using namespace moq;

void
TrackWriter::StatusChanged(TrackWriter::Status status)
{
    switch (status) {
        case Status::kOk: {
            if (auto track_alias = GetTrackAlias(); track_alias.has_value()) {
                SPDLOG_INFO("Publish track alias: {0} is ready to send", track_alias.value());
            }
            break;
        }
        case Status::kNoSubscribers: {
            if (auto track_alias = GetTrackAlias(); track_alias.has_value()) {
                SPDLOG_INFO("Publish track alias: {0} has no subscribers", track_alias.value());
            }
            break;
        }
        case Status::kSubscriptionUpdated: {
            if (auto track_alias = GetTrackAlias(); track_alias.has_value()) {
                SPDLOG_INFO("Publish track alias: {0} has updated subscription", track_alias.value());
            }
            break;
        }
        default:
            if (auto track_alias = GetTrackAlias(); track_alias.has_value()) {
                SPDLOG_INFO("Publish track alias: {0} status {1}", track_alias.value(), static_cast<int>(status));
            }
            break;
    }
}
