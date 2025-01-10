#include "hactar_publish_track_handler.hh"

HactarPublishTrackHandler::HactarPublishTrackHandler(const quicr::FullTrackName& full_track_name,
  quicr::TrackMode track_mode,
  uint8_t default_priority,
  uint32_t default_ttl)
  : quicr::PublishTrackHandler(full_track_name, track_mode, default_priority, default_ttl)
{
}

void HactarPublishTrackHandler::StatusChanged(Status status)
{
  switch (status)
  {
    case Status::kOk: {
      if (auto track_alias = GetTrackAlias(); track_alias.has_value())
      {
        SPDLOG_INFO("Track alias: {0} is ready to read", track_alias.value());
      }
    } break;
    default:
      break;
  }
}

// #include "pub_delegate.hh"
// #include <sstream>

// PubDelegate::PubDelegate(cantina::LoggerPointer logger_in,
//                          std::promise<bool> on_response_in)
//   : logger(std::move(logger_in))
//   , on_response(std::move(on_response_in))
// {
// }

// void
// PubDelegate::onPublishIntentResponse(const quicr::Namespace& quicr_namespace,
//                                      const quicr::PublishIntentResult& result)
// {
//   if (on_response) {
//     on_response->set_value(result.status == quicr::messages::Response::Ok);
//     on_response.reset();
//   }
// }