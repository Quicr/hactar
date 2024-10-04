#pragma once

#include <quicr/client.h>

class HactarPublishTrackHandler: public quicr::PublishTrackHandler
{
public:
  HactarPublishTrackHandler(const quicr::FullTrackName& full_track_name,
    quicr::TrackMode track_mode,
    uint8_t default_priority,
    uint32_t default_ttl);

  void StatusChanged(Status status) override;
};

// #pragma once
// // #include <cantina/logger.h>
// #include <quicr/client.h>

// #include <future>

// class PubDelegate : public quicr::PublisherDelegate
// {
// public:
//   PubDelegate(cantina::LoggerPointer logger_in,
//               std::promise<bool> on_response_in);

//   void onPublishIntentResponse(
//     const quicr::Namespace& quicr_namespace,
//     const quicr::PublishIntentResult& result) override;

// private:
//   // cantina::LoggerPointer logger;
//   std::optional<std::promise<bool>> on_response;
// };