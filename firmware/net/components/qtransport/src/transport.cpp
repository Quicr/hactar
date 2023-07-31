#include <transport/transport.h>
#include <transport_udp.h>
//#include <transport_picoquic.h>

namespace qtransport {

std::shared_ptr<ITransport>
ITransport::make_client_transport(const TransportRemote& server,
                                  const TransportConfig& tcfg,
                                  TransportDelegate& delegate,
                                  LogHandler& logger)
{

  switch (server.proto) {
    case TransportProtocol::UDP:
      return std::make_shared<UDPTransport>(server, delegate, false, logger);

    case TransportProtocol::QUIC:
      logger.log(LogLevel::error, "Protocol not implemented");
      abort();
      break;

    default:
      logger.log(LogLevel::error, "Protocol not implemented");
      abort();
      break;
  }

  return NULL;
}

std::shared_ptr<ITransport>
ITransport::make_server_transport(const TransportRemote& server,
                                  const TransportConfig& tcfg,
                                  TransportDelegate& delegate,
                                  LogHandler& logger)
{
  switch (server.proto) {
    case TransportProtocol::UDP:
      return std::make_shared<UDPTransport>(server, delegate, true, logger);

    case TransportProtocol::QUIC:
    default:
      logger.log(LogLevel::error, "Protocol not implemented");

      abort();
      break;
  }

  return NULL;
}

} // namespace qtransport
