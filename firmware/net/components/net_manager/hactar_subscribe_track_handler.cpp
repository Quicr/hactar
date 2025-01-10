#include "hactar_subscribe_track_handler.hh"


HactarSubscribeTrackHandler::HactarSubscribeTrackHandler(
    const quicr::FullTrackName& full_track_name):
    SubscribeTrackHandler(full_track_name, 2, quicr::messages::GroupOrder::kAscending, quicr::messages::FilterType::LatestGroup)
{

}

void HactarSubscribeTrackHandler::ObjectReceived(
    const quicr::ObjectHeaders& headers, quicr::BytesSpan data)
{

}

void HactarSubscribeTrackHandler::StatusChanged(Status status)
{

}

// #include <sstream>

// SubDelegate(cantina::LoggerPointer logger_in,
//     std::shared_ptr<AsyncQueue<QuicrObject>> queue_in,
//     std::promise<bool> on_response_in)
//     : logger(std::move(logger_in))
//     , queue(std::move(queue_in))
//     , on_response(std::move(on_response_in))
// {
// }

// void
// onSubscribeResponse(const quicr::Namespace& quicr_namespace,
//     const quicr::SubscribeResult& result)
// {
//     logger->info << "onSubscriptionResponse: ns: " << quicr_namespace
//         << " status: " << static_cast<int>(result.status) << std::flush;

//     if (on_response)
//     {
//         on_response->set_value(result.status ==
//             quicr::SubscribeResult::SubscribeStatus::Ok);
//         on_response.reset();
//     }
// }

// void
// onSubscriptionEnded(
//     const quicr::Namespace& quicr_namespace,
//     const quicr::SubscribeResult::SubscribeStatus& reason)

// {
//     logger->info << "onSubscriptionEnded: ns: " << quicr_namespace
//         << " reason: " << static_cast<int>(reason) << std::flush;
// }

// void
// onSubscribedObject(const quicr::Name& quicr_name,
//     uint8_t /* priority */,
//     uint16_t /* expiry_age_ms */,
//     bool /* use_reliable_transport */,
//     quicr::bytes&& data)
// {
//     logger->info << "recv object: name: " << quicr_name
//         << " data sz: " << data.size();

//     if (!data.empty())
//     {
//         logger->info << " data: " << data.data();
//     }
//     else
//     {
//         logger->info << " (no data)";
//     }

//     logger->info << std::flush;
//     queue->push({ quicr_name, std::move(data) });
// }

// void
// onSubscribedObjectFragment(const quicr::Name& quicr_name,
//     uint8_t /* priority */,
//     uint16_t /* expiry_age_ms */,
//     bool /* use_reliable_transport */,
//     const uint64_t& /* offset */,
//     bool /* is_last_fragment */,
//     quicr::bytes&& /* data */)
// {
//     logger->info << "Ignoring object fragment received for " << quicr_name
//         << std::flush;
// }