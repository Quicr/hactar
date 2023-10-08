#include "qsession.h"
#include "transport/transport.h"


QSession::QSession(quicr::RelayInfo relay_info)
: qlogger(std::make_shared<cantina::Logger>("quicr_logger"))  
  , qclient {relay_info, {}, qlogger} {}




void QSession::subscribe(quicr::Namespace nspace) {}

void QSession::unsubscribe(quicr::Namespace nspace){}

void QSession::publishData(quicr::Namespace& nspace, quicr::bytes&& data) {}

void QSession::handle(const quicr::Name& name, quicr::bytes&& data) {}
