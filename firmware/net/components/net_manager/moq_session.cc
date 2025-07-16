#include "moq_session.hh"
#include "logger.hh"
#include "task_helpers.hh"
#include "utils.hh"

using namespace moq;

extern uint64_t device_id;

Session::Session(const quicr::ClientConfig& cfg,
                 std::vector<std::shared_ptr<TrackReader>>& readers,
                 std::vector<std::shared_ptr<TrackWriter>>& writers) :
    Client(cfg),
    readers_mux(),
    writers_mux(),
    readers(readers),
    readers_task_handle(nullptr),
    readers_task_buffer({0}),
    readers_task_stack(nullptr),
    writers(writers),
    writers_task_handle(nullptr),
    writers_task_buffer({0}),
    writers_task_stack(nullptr)
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

        {
            std::lock_guard<std::mutex> _(session->writers_mux);
            NET_LOG_INFO("num writers %d", session->writers.size());

            for (int i = 0; i < session->writers.size(); ++i)
            {
                auto& writer = session->writers[i];

                if (!writer)
                {
                    continue;
                }

                // Writer is publishing
                if (writer->GetStatus() == moq::TrackWriter::Status::kOk)
                {
                    continue;
                }

                session->PublishTrack(writer);

                while (writer->GetStatus() != moq::TrackWriter::Status::kOk)
                {
                    NET_LOG_INFO("writer %s status %d", writer->GetTrackName().c_str(),
                                 writer->GetStatus());
                    vTaskDelay(300 / portTICK_PERIOD_MS);
                }
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

    // TODO notifier that awakes the thread when a new sub or pub is available
    Session* session = static_cast<Session*>(params);

    while (true)
    {
        while (session->GetStatus() != moq::Session::Status::kReady)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        {
            std::lock_guard<std::mutex> _(session->readers_mux);
            NET_LOG_INFO("num readers %d", session->readers.size());

            for (int i = 0; i < session->readers.size(); ++i)
            {
                auto& reader = session->readers[i];

                if (!reader)
                {
                    continue;
                }

                // Reader is subscribed
                if (reader->GetStatus() == moq::TrackReader::Status::kOk)
                {
                    continue;
                }

                session->SubscribeTrack(reader);

                while (reader->GetStatus() != moq::TrackReader::Status::kOk)
                {
                    NET_LOG_ERROR("Reader %s status %d", reader->GetTrackName().c_str(),
                                  (int)reader->GetStatus());
                    vTaskDelay(300 / portTICK_PERIOD_MS);
                }
            }
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(nullptr);
}
