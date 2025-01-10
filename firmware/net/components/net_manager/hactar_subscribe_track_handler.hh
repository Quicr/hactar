#pragma once

#include "async_queue.h"

#include <quicr/client.h>

#include <future>

class HactarSubscribeTrackHandler: public quicr::SubscribeTrackHandler
{
public:
  HactarSubscribeTrackHandler(const quicr::FullTrackName& full_track_name);

  void ObjectReceived(const quicr::ObjectHeaders& headers, quicr::BytesSpan data) override;

  void StatusChanged(Status status) override;
};