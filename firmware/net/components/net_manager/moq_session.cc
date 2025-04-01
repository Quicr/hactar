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


void Session::StartAudioReadTrack(const std::string& track_name, const size_t min_depth, const size_t max_depthH)
{
    // Create a audio track reader

    // auto sub_track_handler = std::make_shared<AudioTrackReader>(
    //     new moq::AudioTrackReader(
    //         moq::MakeFullTrackName(base_track_namespace + track_location, sub_track, 2001),
    //         10)
    // );

    // sub_track_handler->GetFullTrackName().name_space

}