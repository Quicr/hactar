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

#include "macros.hh"

void MoqSubscribeTask(void* arg)
{
    link_packet_t link_packet;
    std::shared_ptr<moq::TrackReader> sub_track_handler;
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
            new moq::TrackReader(
                moq::MakeFullTrackName({"moq://moq.ptt.arpa/v1", "org/acme", "store/1234", "channel/gardening", "ptt"}, "pcm_en_8khz_mono_i16")
            )
        );

        moq_session->SubscribeTrack(sub_track_handler);

        NET_LOG_INFO("Started subscriber");

        while (moq_session &&
            moq_session->GetStatus() == moq::Session::Status::kReady &&
            sub_track_handler->GetStatus() != moq::TrackReader::Status::kOk &&
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
                if (sub_track_handler->GetStatus() != moq::TrackReader::Status::kOk)
                {
                    // TODO handling
                    continue;
                }

                sub_track_handler->AudioPlay();

                if (xSemaphoreTake(audio_req_smpr, 0))
                {
                    auto data = sub_track_handler->AudioPopFront();
                    if (!data.has_value())
                    {
                        continue;
                    }

                    uint32_t offset = 0;
                    // TODO something with last chunk?
                    if (data->at(offset))
                    {
                        // Last chunk
                    }
                    offset += 1;

                    link_packet_t link_packet;
                    link_packet.type = (uint8_t)ui_net_link::Packet_Type::AudioObject;

                    // Get the length of the audio packet
                    uint32_t audio_length = 0;
                    memcpy(&audio_length, data->data() + offset, sizeof(audio_length));
                    offset += sizeof(audio_length);


                    // Channel id
                    // TODO get channel id, for now use 1
                    link_packet.payload[0] = 1;
                    link_packet.length = audio_length + 1;

                    memcpy(link_packet.payload + 1, data->data() + offset, audio_length);

                    // NET_LOG_INFO("packet length %d", link_packet.length);

                    ui_layer.Write(link_packet);
                }
            }
            catch (const std::exception& ex)
            {
                ESP_LOGE("sub", "Exception in sub %s", ex.what());
            }
        }

        if (moq_session)
        {
            NET_LOG_INFO("Unsubscribing");
            moq_session->UnsubscribeTrack(sub_track_handler);
        }
    }

    NET_LOG_INFO("Delete sub task");
    vTaskDelete(NULL);
}
