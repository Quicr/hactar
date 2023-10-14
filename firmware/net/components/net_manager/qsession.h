#pragma once

#include <memory>
#include <map>

#include <quicr/quicr_client.h>
#include <quicr/quicr_client_delegate.h>
#include <quicr/name.h>
#include <cantina/logger.h>

#include "sub_delegate.h"
#include "pub_delegate.h"

// Rough notes
// watch a room --> publish_intent with publisher_uri for the room and subscribe for room_uri


class QSession {

public:
  QSession(quicr::RelayInfo relay_info);
  ~QSession() = default;
  bool connect();
  bool publish_intent(quicr::Namespace ns);
  bool subscribe(quicr::Namespace ns);
  void unsubscribe(quicr::Namespace ns);
  void publish(const quicr::Name& name, quicr::bytes&& data);
  void handle(QuicrObject&& obj);


private:
  std::optional<std::thread> handler_thread;
  std::atomic_bool stop = false;
  static constexpr auto inbound_object_timeout = std::chrono::milliseconds(100);
  std::shared_ptr<AsyncQueue<QuicrObject>> inbound_objects;
  cantina::LoggerPointer logger;
  std::unique_ptr<quicr::Client> client;
  std::map<quicr::Namespace, std::shared_ptr<SubDelegate>> sub_delegates{};
};
