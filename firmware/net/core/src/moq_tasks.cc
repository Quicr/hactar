#include "net.hh"
#include "moq_tasks.hh"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"

#include "moq_session.hh"
#include "moq_track_writer.hh"
#include "utils.hh"
#include "logger.hh"
#include "ui_net_link.hh"
#include "chunk.hh"
#include <memory>

void MoqPublishTask(void* args)
{
    NET_LOG_INFO("Start publish task");

    std::shared_ptr<moq::TrackWriter> pub_track_handler;
    int64_t next_print = 0;

    while (true)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);

        while (!moq_session
            || moq_session->GetStatus() != moq::Session::Status::kReady)
        {
            // Wait for moq sesssion to be set.
            if (esp_timer_get_time_ms() > next_print)
            {
                NET_LOG_INFO("Publisher task waiting for moq session and wifi");
                next_print = esp_timer_get_time_ms() + 2000;
            }
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }

        pub_track_handler.reset(
            new moq::TrackWriter(
                moq::MakeFullTrackName(base_track_namespace + track_location, pub_track, 1001),
                quicr::TrackMode::kDatagram, 2, 100)
        );
        moq_session->PublishTrack(pub_track_handler);

        NET_LOG_INFO("Started publisher");

        while (moq_session &&
            pub_track_handler->GetStatus() != moq::TrackWriter::Status::kOk &&
            xSemaphoreTake(pub_change_smpr, 0) == pdFALSE)
        {
            if (esp_timer_get_time_ms() > next_print)
            {
                NET_LOG_INFO("Waiting for pub ok!");
                next_print = esp_timer_get_time_ms() + 2000;
            }
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }

        NET_LOG_INFO("Publishing!");

        pub_ready = true;

        while (moq_session
            && moq_session->GetStatus() == moq::Session::Status::kReady
            && xSemaphoreTake(pub_change_smpr, 0) == pdFALSE)
        {
            vTaskDelay(2 / portTICK_PERIOD_MS);

            if (pub_track_handler && pub_track_handler->GetStatus() != moq::TrackWriter::Status::kOk)
            {
                continue;
            }

            if (moq_objects.size() == 0)
            {
                continue;
            }

            std::lock_guard<std::mutex> lock(object_mux);
            const link_data_obj& obj = moq_objects.front();
            pub_track_handler->PublishObject(obj.headers, obj.data);
            moq_objects.pop_front();
        }

        // If we break out of here then we should try to unplublish
        if (moq_session)
        {
            moq_session->UnpublishTrack(pub_track_handler);
        }
    }

    vTaskDelete(nullptr);
}

void MoqSubscribeTask(void* arg)
{
    link_packet_t link_packet;
    std::shared_ptr<moq::AudioTrackReader> sub_track_handler;
    int64_t next_print = 0;

    NET_LOG_INFO("Start subscribe task");

    while (true)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);

        while (!moq_session
            || moq_session->GetStatus() != moq::Session::Status::kReady)
        {
            // Wait for moq sesssion to be set.
            if (esp_timer_get_time_ms() > next_print)
            {
                NET_LOG_INFO("Subscriber task waiting for moq session");
                next_print = esp_timer_get_time_ms() + 2000;
            }
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }

        sub_track_handler.reset(
            new moq::AudioTrackReader(
                moq::MakeFullTrackName(base_track_namespace + track_location, sub_track, 2001),
                10)
        );

        moq_session->SubscribeTrack(sub_track_handler);

        NET_LOG_INFO("Started subscriber");

        while (moq_session &&
            moq_session->GetStatus() == moq::Session::Status::kReady &&
            sub_track_handler->GetStatus() != moq::AudioTrackReader::Status::kOk &&
            xSemaphoreTake(sub_change_smpr, 0) == pdFALSE)
        {
            if (esp_timer_get_time_ms() > next_print)
            {
                NET_LOG_INFO("Waiting for sub ok!");
                next_print = esp_timer_get_time_ms() + 2000;
            }
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }

        NET_LOG_INFO("Subscribed");

        while (moq_session &&
            moq_session->GetStatus() == moq::Session::Status::kReady &&
            xSemaphoreTake(sub_change_smpr, 0) == pdFALSE)
        {
            try
            {
                vTaskDelay(2 / portTICK_PERIOD_MS);
                if (sub_track_handler->GetStatus() != moq::AudioTrackReader::Status::kOk)
                {
                    // TODO handling
                    continue;
                }

                sub_track_handler->TryPlay();

                if (xSemaphoreTake(audio_req_smpr, 0))
                {
                    // TODO move?
                    auto data = sub_track_handler->PopFront();
                    if (!data.has_value())
                    {
                        continue;
                    }

                    // Convert to chunk
                    Chunk chunk = Chunk::Deserialize(data);

                    switch (chunk.type)
                    {
                        case Chunk::ContentType::Audio:
                        {
                            if (chunk.is_last)
                            {
                                // is last, dunno what to do with it but may need it.
                                // perhaps send some sort of link packet
                                // informing the ui that we are done?
                            }

                            // TODO if the chunk is bigger than a chunk can handle
                            // then we should spit it and push extra packets
                            // onto a queue that gets handled first.

                            // Hard code for now.

                            link_packet_t link_packet;
                            // link_packet.type = (uint8_t)ui_net_link::Packet_Type::AudioObject;
                            ui_layer.Write((uint8_t)ui_net_link::Packet_Type::AudioObject, false);
                            ui_layer.Write((uint8_t*)&chunk.length, 4, false);
                            ui_layer.Write(chunk.data.data(), chunk.length);
                            break;
                        }
                        case Chunk::ContentType::Json:
                        {
                            NET_LOG_INFO("Json received");
                            break;
                        }
                        default:
                        {
                            NET_LOG_INFO("Chunk without handler received");
                            break;
                        }
                    }
                }
            }
            catch (const std::exception& ex)
            {
                ESP_LOGE("sub", "Exception in sub %s", ex.what());
            }
        }

        if (moq_session)
        {
            moq_session->UnsubscribeTrack(sub_track_handler);
        }
    }

    NET_LOG_INFO("Delete sub task");
    vTaskDelete(NULL);
}
