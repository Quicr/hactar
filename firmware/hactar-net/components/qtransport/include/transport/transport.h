#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <vector>

#include "logger.h"

namespace qtransport {

using TransportContextId = uint64_t; ///< Context Id is a 64bit number that is used as a key to maps
using StreamId = uint64_t;           ///< stream Id is a 64bit number that is
                                     ///< used as a key to maps
/**
 * Transport status/state values
 */
enum class TransportStatus : uint8_t
{
  Ready = 0,
  Connecting,
  RemoteRequestClose,
  Disconnected,
  Shutdown
};

/**
 * Transport errors
 */
enum class TransportError : uint8_t
{
  None = 0,
  QueueFull,
  UnknownError,
  PeerDisconnected,
  PeerUnreachable,
  CannotResolveHostname,
  InvalidContextId,
  InvalidStreamId,
  InvalidIpv4Address,
  InvalidIpv6Address
};

/**
 *
 */
enum class TransportProtocol
{
  UDP = 0,
  QUIC
};

/**
 * @brief Remote/Destination endpoint address info.
 *
 * @details Remote destination is either a client or server hostname/ip and port
 */
struct TransportRemote
{
  std::string host_or_ip;  // IPv4/v6 or FQDN (user input)
  uint16_t port;           // Port (user input)
  TransportProtocol proto; // Protocol to use for the transport
};

/**
 * Transport configuration parameters
 */
struct TransportConfig
{
  const char *tls_cert_filename;            /// QUIC TLS certificate to use
  const char *tls_key_filename;             /// QUIC TLS private key to use
  const uint32_t data_queue_size {500};     /// Size of incoming and outgoing data queues
  bool debug {false};                       /// Enable debug logging/processing
};

/**
 * @brief ITransport interface
 *
 * @details A single threaded, async transport interface.
 * 	The transport implementations own the queues
 * 	on which the applications can enqueue the messages
 * 	for transmitting and dequeue for consumption
 *
 * 	Applications using this transport interface
 * 	MUST treat it as thread-unsafe and the same
 * 	is ensured by the transport owing the lock and
 * 	access to the queues.
 *
 * @note Some implementations may cho/ose to
 * 	have enqueue/dequeue being blocking. However
 * 	in such cases applications needs to
 * 	take the burden of non-blocking flows.
 */
class ITransport
{
public:
  /**
   * @brief Async Callback API on the transport
   */
  class TransportDelegate
  {
  public:
    virtual ~TransportDelegate() = default;

    /**
     * @brief Event notification for connection status changes
     *
     * @details Called when the connection changes state/status
     *
     * @param[in] context_id  Transport context Id
     * @param[in] status 			Transport Status value
     */
    virtual void on_connection_status(const TransportContextId& context_id,
                                      const TransportStatus status) = 0;

    /**
     * @brief Report arrival of a new connection
     *
     * @details Called when new connection is received. This is only used in
     * server mode.
     *
     * @param[in] context_id	Transport context identifier mapped to the
     * connection
     * @param[in] remote			Transport information for the
     * connection
     */
    virtual void on_new_connection(const TransportContextId& context_id,
                                   const TransportRemote& remote) = 0;

    /**
     * @brief Report arrival of a new stream
     *
     * @details Called when new connection is received. This is only used in
     * server mode.
     *
     * @param[in] context_id	Transport context identifier mapped to the
     * connection
     * @param[in] streamId		A new stream id created
     */
    virtual void on_new_stream(const TransportContextId& context_id,
                               const StreamId & streamId) = 0;

    /**
     * @brief Event reporting transport has some data over
     * 		the network for the application to consume
     *
     * @details Applications must invoke ITransport::deqeue() to obtain
     * 		the data by passing the transport context id
     *
     * @param[in] context_id 	Transport context identifier mapped to the
     * connection
     * @param[in] streamId	Stream id that the data was
     * received on
     */
    virtual void on_recv_notify(const TransportContextId& context_id,
                                const StreamId & streamId) = 0;
  };

  /* Factory APIs */

  /**
   * @brief Create a new client transport based on the remote (server) host/ip
   *
   * @param[in] server			Transport remote server information
   * @param[in] tcfg                    Transport configuration
   * @param[in] delegate		Implemented callback methods
   * @param[in] logger			Transport log handler
   *
   * @return shared_ptr for the under lining transport.
   */
  static std::shared_ptr<ITransport> make_client_transport(
    const TransportRemote& server,
    const TransportConfig &tcfg,
    TransportDelegate& delegate,
    LogHandler& logger);

  /**
   * @brief Create a new server transport based on the remote (server) ip and
   * port
   *
   * @param[in] server			Transport remote server information
   * @param[in] tcfg                    Transport configuration
   * @param[in] delegate		Implemented callback methods
   * @param[in] logger			Transport log handler
   *
   * @return shared_ptr for the under lining transport.
   */
  static std::shared_ptr<ITransport> make_server_transport(
    const TransportRemote& server,
    const TransportConfig &tcfg,
    TransportDelegate& delegate,
    LogHandler& logger);

public:
  virtual ~ITransport() = default;

  /**
   * @brief Status of the transport
   *
   * @details Return the status of the transport. In server mode, the transport
   * will reflect the status of the listening socket. In client mode it will
   * reflect the status of the server connection.
   */
  virtual TransportStatus status() const = 0;

  /**
   * @brief Setup the transport connection
   *
   * @details In server mode this will create the listening socket and will
   * 		start listening on the socket for new connections. In client
   * mode this will initiate a connection to the remote/server.
   *
   * @return TransportContextId: identifying the connection
   */
  virtual TransportContextId start() = 0;

  /**
   * @brief Create a stream in the context
   *
   * @todo change to generic stream
   *
   * @param[in] context_id
   * Identifying the connection
   * @param[in] use_reliable_transport 	Indicates a reliable stream is
   *                                 		preferred for transporting data
   *
   * @return StreamId identifying the stream via the connection
   */
  virtual StreamId createStream(const TransportContextId& context_id,
                                bool use_reliable_transport) = 0;

  /**
   * @brief Close a transport context
   */
  virtual void close(const TransportContextId& context_id) = 0;

  /**
   * @brief Close/end a stream within context
   */
  virtual void closeStream(const TransportContextId& context_id,
                           StreamId stream_id) = 0;

  /**
   * @brief Enqueue application data within the transport
   *
   * @details Add data to the transport queue. Data enqueued will be transmitted
   * when available.
   *
   * @todo is priority missing?
   *
   * @param[in] context_id	Identifying the connection
   * @param[in] streamId	stream Id to send data on
   * @param[in] bytes				Data to send/write
   *
   * @returns TransportError is returned indicating status of the operation
   */
  virtual TransportError enqueue(const TransportContextId& context_id,
                                 const StreamId & streamId,
                                 std::vector<uint8_t>&& bytes) = 0;

  /**
   * @brief Dequeue application data from transport queue
   *
   * @details Data received by the transport will be queued and made available
   * to the caller using this method.  An empty return will be
   *
   * @param[in] context_id		Identifying the connection
   * @param[in] streamId	        Stream Id to receive data
   * from
   *
   * @returns std::nullopt if there is no data
   */
  virtual std::optional<std::vector<uint8_t>> dequeue(
    const TransportContextId& context_id,
    const StreamId & streamId) = 0;
};

} // namespace qtransport