// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#include "moq_track_writer.hh"
#include "chunk.hh"
#include "macros.hh"
#include "task_helpers.hh"
#include "utils.hh"

using namespace moq;

extern uint64_t device_id;

TrackWriter::TrackWriter(const quicr::FullTrackName& full_track_name,
                         quicr::TrackMode track_mode,
                         uint8_t default_priority,
                         uint32_t default_ttl) :
    quicr::PublishTrackHandler(full_track_name, track_mode, default_priority, default_ttl),
    track_name(std::string(full_track_name.name_space.begin(), full_track_name.name_space.end())
               + std::string(full_track_name.name.begin(), full_track_name.name.end())),
    moq_objs({0}),
    object_id(0),
    is_running(false),
    task_mutex(),
    obj_mux(),
    task_handle(nullptr),
    task_buffer(),
    task_stack(nullptr)
{
}

TrackWriter::~TrackWriter()
{
    if (!is_running)
    {
        return;
    }

    Stop();
}

void TrackWriter::Start()
{
    if (task_handle)
    {
        NET_LOG_INFO("Moq writer has already started");
        return;
    }

    task_helpers::Start_PSRAM_Task(PublishTask, this, track_name, task_handle, task_buffer,
                                   &task_stack, 16384, 10);
}

void TrackWriter::Stop()
{
    NET_LOG_WARN("Stopping writer");
    is_running = false;

    std::lock_guard<std::mutex> _(task_mutex);

    if (task_handle)
    {
        task_handle = nullptr;
    }

    if (task_stack)
    {
        heap_caps_free(task_stack);
        task_stack = nullptr;
    }

    NET_LOG_WARN("Stopped writer");
}

void TrackWriter::StatusChanged(TrackWriter::Status status)
{
    switch (status)
    {
    case Status::kOk:
    {
        if (auto track_alias = GetTrackAlias(); track_alias.has_value())
        {
            SPDLOG_INFO("Publish track alias: {0} is ready to send", track_alias.value());
        }
        break;
    }
    case Status::kNoSubscribers:
    {
        if (auto track_alias = GetTrackAlias(); track_alias.has_value())
        {
            SPDLOG_INFO("Publish track alias: {0} has no subscribers", track_alias.value());
        }
        break;
    }
    case Status::kSubscriptionUpdated:
    {
        if (auto track_alias = GetTrackAlias(); track_alias.has_value())
        {
            SPDLOG_INFO("Publish track alias: {0} has updated subscription", track_alias.value());
        }
        break;
    }
    default:
        if (auto track_alias = GetTrackAlias(); track_alias.has_value())
        {
            SPDLOG_INFO("Publish track alias: {0} status {1}", track_alias.value(),
                        static_cast<int>(status));
        }
        break;
    }
}

void TrackWriter::PushObject(const uint8_t* bytes, const uint32_t len, const uint64_t timestamp)
{
    auto time_bytes = quicr::AsBytes(timestamp);

    std::lock_guard<std::mutex> _(obj_mux);

    auto& obj = moq_objs.emplace_back();
    obj.headers.group_id = device_id;
    obj.headers.object_id = object_id++;
    obj.headers.payload_length = len;
    obj.headers.extensions = quicr::Extensions{};
    obj.headers.extensions.value()[2].assign(time_bytes.begin(), time_bytes.end());

    obj.data.assign(bytes, bytes + len);
}

const std::string& TrackWriter::GetTrackName() const noexcept
{
    return track_name;
}

void TrackWriter::PublishTask(void* params)
{
    TrackWriter* writer = static_cast<TrackWriter*>(params);
    std::lock_guard<std::mutex> _(writer->task_mutex);
    writer->is_running = true;

    const auto can_publish = [&] {
        const auto status = writer->GetStatus();
        return status == moq::TrackWriter::Status::kOk
            || status == moq::TrackWriter::Status::kSubscriptionUpdated
            || status == moq::TrackWriter::Status::kNewGroupRequested;
    };

    while (writer->is_running)
    {
        uint32_t next_print = 0;
        // TODO add in changing pub
        while (!can_publish() && writer->is_running)
        {
            if (esp_timer_get_time_ms() > next_print)
            {
                NET_LOG_WARN("Publisher on track %s waiting to for pub ok status %d",
                             writer->track_name.c_str(), (int)writer->GetStatus());
                next_print = esp_timer_get_time_ms() + 2000;
            }
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }

        NET_LOG_INFO("Publishing to track %s (status=%d)", writer->track_name.c_str(),
                     static_cast<int>(writer->GetStatus()));

        // TODO changing pub
        while (can_publish() && writer->is_running)
        {
            // TODO use notifies and then drain the entire moq objs
            vTaskDelay(2 / portTICK_PERIOD_MS);

            if (writer->moq_objs.size() == 0)
            {
                continue;
            }

            std::lock_guard<std::mutex> _(writer->obj_mux);
            const link_data_obj& obj = writer->moq_objs.front();
            if (auto pub_status = writer->PublishObject(obj.headers, obj.data);
                pub_status == moq::TrackWriter::PublishObjectStatus::kOk)
            {
                writer->moq_objs.pop_front();
            }
        }
    }

    NET_LOG_ERROR("Publish task for %s has exited", writer->track_name.c_str());

    writer->task_mutex.unlock();
    vTaskDelete(nullptr);
}