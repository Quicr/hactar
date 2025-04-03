#include "moq_session.hh"

#include "logger.hh"

#include "utils.hh"

using namespace moq;


void Session::StatusChanged(Status status)
{
    switch (status) {
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

void StartWriteTrack(const json track_json)
{
    try
    {
        // TODO something with channel name, like transmitting it to ui
        std::string channel_name = track_json.at("channel_name").get<std::string>();

        // TODO
        std::string lang = track_json.at("language").get<std::string>();

        // std::
    }
    catch (const std::exception& ex)
    {
        ESP_LOGE("sub", "Exception in sub %s", ex.what());
    }
}