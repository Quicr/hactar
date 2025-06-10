#include "moq_session.hh"
#include "logger.hh"
#include "task_helpers.hh"
#include "utils.hh"

using namespace moq;

extern uint64_t device_id;

Session::Session(const quicr::ClientConfig& cfg) :
    Client(cfg)
{
    StartTasks();
}

void Session::StartTasks() noexcept
{
    task_helpers::Start_PSRAM_Task(PublishTrackTask, this, "Publish Tracks task",
                                   writers_task_handle, writers_task_buffer, &writers_task_stack,
                                   8192, 10);
    task_helpers::Start_PSRAM_Task(SubscribeTrackTask, this, "Subscribe Tracks task",
                                   readers_task_handle, readers_task_buffer, &readers_task_stack,
                                   8192, 10);
}

void Session::StatusChanged(Status status)
{
    switch (status)
    {
    case Status::kReady:
        Logger::Log(Logger::Level::Info, "MOQ Connection ready");
        break;
    case Status::kConnecting:
        break;
    case Status::kPendingSeverSetup:
        Logger::Log(Logger::Level::Info, "MOQ Connection connected and now pending server setup");
        break;
    default:
        Logger::Log(Logger::Level::Error, "MOQ Connection failed: %i", static_cast<int>(status));
        break;
    }
}

std::shared_ptr<TrackWriter> Session::Writer(const size_t id) noexcept
{
    if (id > writers.size())
    {
        NET_LOG_ERROR("ERROR, writer with id %ld does not exist", id);
        return nullptr;
    }

    return writers[id];
}

void Session::StartReadTrack(const json& subscription, Serial& serial)
{
    try
    {

        // TODO something with channel name, like transmitting it to ui
        // std::string channel_name = subscription.at("channel_name").get<std::string>();

        // TODO transmit to the ui chip probably
        // std::string lang = subscription.at("language").get<std::string>();

        std::vector<std::string> track_namespace =
            subscription.at("tracknamespace").get<std::vector<std::string>>();
        std::string trackname = subscription.at("trackname").get<std::string>();

        if (trackname == "")
        {
            trackname = std::to_string(device_id);
        }

        std::string codec = subscription.at("codec").get<std::string>();
        ESP_LOGE("sub", "%s", codec.c_str());

        std::shared_ptr<moq::TrackReader> reader = std::make_shared<moq::TrackReader>(
            moq::MakeFullTrackName(track_namespace, trackname), serial, codec);

        std::lock_guard<std::mutex> _(readers_mux);
        readers.push_back(reader);
    }
    catch (const std::exception& ex)
    {
        ESP_LOGE("sub", "Exception in sub %s", ex.what());
    }
}

void Session::StartWriteTrack(const json& publication)
{
    try
    {
        // TODO something with channel name, like transmitting it to ui
        // std::string channel_name = publication.at("channel_name").get<std::string>();

        // TODO transmit to the ui chip probably
        // std::string lang = publication.at("language").get<std::string>();

        std::vector<std::string> track_namespace =
            publication.at("tracknamespace").get<std::vector<std::string>>();
        std::string trackname = publication.at("trackname").get<std::string>();

        // TODO something with this?
        // std::string codec = publication.at("codec").get<std::string>();

        // uint64_t sample_rate = publication.at("sample_rate").get<uint64_t>();
        // std::string channel_config = publication.at("channelConfig").get<std::string>();

        std::shared_ptr<moq::TrackWriter> writer =
            std::make_shared<moq::TrackWriter>(moq::MakeFullTrackName(track_namespace, trackname),
                                               quicr::TrackMode::kDatagram, 2, 100);

        std::lock_guard<std::mutex> _(writers_mux);
        writers.push_back(writer);
    }
    catch (const std::exception& ex)
    {
        ESP_LOGE("pub", "Exception in pub %s", ex.what());
    }
}

void Session::PublishTrackTask(void* params)
{
    // TODO use a mutex to try and subscribe/publish
    // as I could remove them from the list or update them.
    Session* session = static_cast<Session*>(params);

    while (true)
    {
        while (session->GetStatus() != moq::Session::Status::kReady)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        std::lock_guard<std::mutex> _(session->writers_mux);

        for (int i = 0; i < session->writers.size(); ++i)
        {
            auto& writer = session->writers[i];

            // Writer is publishing
            if (writer->GetStatus() == moq::TrackWriter::Status::kOk)
            {
                continue;
            }

            // TODO switch on all statuses
            session->PublishTrack(writer);

            while (writer->GetStatus() != moq::TrackWriter::Status::kOk)
            {
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(nullptr);
}

void Session::SubscribeTrackTask(void* params)
{
    // TODO use a mutex to try and subscribe/publish
    // as I could remove them from the list or update them.
    Session* session = static_cast<Session*>(params);

    while (true)
    {
        while (session->GetStatus() != moq::Session::Status::kReady)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        std::lock_guard<std::mutex> _(session->readers_mux);

        for (int i = 0; i < session->readers.size(); ++i)
        {
            auto& reader = session->readers[i];

            // Writer is publishing
            if (reader->GetStatus() == moq::TrackReader::Status::kOk)
            {
                continue;
            }

            // TODO switch on all statuses
            session->SubscribeTrack(reader);

            while (reader->GetStatus() != moq::TrackReader::Status::kOk)
            {
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(nullptr);
}
