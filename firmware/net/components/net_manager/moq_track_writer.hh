// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __MOQ_TRACK_WRITER__
#define __MOQ_TRACK_WRITER__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logger.hh"
#include <quicr/publish_track_handler.h>

namespace moq
{
class TrackWriter : public quicr::PublishTrackHandler
{
public:
    TrackWriter(const quicr::FullTrackName& full_track_name,
                quicr::TrackMode track_mode,
                uint8_t default_priority,
                uint32_t default_ttl);

    virtual ~TrackWriter();

    void Start();

    void Stop();

    void StatusChanged(Status status) override;
    void PushObject(const uint8_t* bytes, const uint32_t len, const uint64_t timestamp);

    struct link_data_obj
    {
        quicr::ObjectHeaders headers = {
            0,
            0,
            0,
            0,
            quicr::ObjectStatus::kAvailable,
            2 /*priority*/,
            3000 /* ttl */,
            std::nullopt,
            std::nullopt,
        };
        std::vector<uint8_t> data;
    };

private:
    static void PublishTask(void* params);

    std::string track_name;
    std::deque<moq::TrackWriter::link_data_obj> moq_objs;
    uint64_t object_id;

    bool is_running;
    std::mutex obj_mux;
    TaskHandle_t task_handle;
    StaticTask_t task_buffer;
    StackType_t* task_stack;
};
} // namespace moq

#endif