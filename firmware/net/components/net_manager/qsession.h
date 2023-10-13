#pragma once

#include <memory>
#include <map>

#include <quicr/quicr_client.h>
#include <quicr/quicr_client_delegate.h>
#include <quicr/name.h>
#include <cantina/logger.h>


// Rough notes
// watch a room --> publish_intent with publisher_uri for the room and subscribe for room_uri


class QSession {

public:
   QSession(quicr::RelayInfo relay_info);
   ~QSession() = default;
  void subscribe(quicr::Namespace nspace);
  void unsubscribe(quicr::Namespace nspace);
  void publishData(quicr::Namespace& nspace, quicr::bytes&& data);
  void handle(const quicr::Name& name, quicr::bytes&& data);


private:
  cantina::LoggerPointer qlogger;
  quicr::QuicRClient qclient;
  
};
