#include <cassert>
#include <cstring> // memcpy
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include <arpa/inet.h>
#include <lwip/netdb.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <esp_pthread.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "transport_udp.h"

using namespace qtransport;

static esp_pthread_cfg_t create_config(const char *name, int core_id, int stack, int prio)
{
    auto cfg = esp_pthread_get_default_config();
    cfg.thread_name = name;
    cfg.pin_to_core = core_id;
    cfg.stack_size = stack;
    cfg.prio = prio;
    return cfg;
}

UDPTransport::~UDPTransport()
{
  // TODO: Close all streams and connections

  // Stop threads
  stop = true;

  // Clear threads from the queue
  fd_write_queue.stopWaiting();

  // Close socket fd
  if (fd >= 0)
    ::close(fd);

  logger.log(LogLevel::info, "Closing transport threads");
  for (auto &thread : running_threads) {
    thread.join();
  }
}

UDPTransport::UDPTransport(const TransportRemote& server,
                           TransportDelegate& delegate,
                           bool isServerMode,
                           LogHandler& logger)
  : stop(false)
  , logger(logger)
  , fd(-1)
  , isServerMode(isServerMode)
  , serverInfo(server)
  , delegate(delegate)
{
  std::cout << "UDP Transport Created" << std::endl;
}


TransportStatus
UDPTransport::status() const
{
  return (fd > 0) ? TransportStatus::Ready : TransportStatus::Disconnected;
}

/*
 * UDP doesn't support multiple streams for clients.  This will return the same
 * stream ID used for the context
 */
StreamId
UDPTransport::createStream(
  const qtransport::TransportContextId& context_id,
  [[maybe_unused]] bool use_reliable_transport)
{

  if (remote_contexts.count(context_id) == 0) {
    std::cout << "Invalid context id: " << context_id;
    return 0; // Error
  }
  auto addr = remote_contexts[context_id];
  return remote_addrs[addr.key].sid;
}

TransportContextId
UDPTransport::start()
{

  if (isServerMode) {
    return connect_server();
  } else {
    return connect_client();
  }

  return 0;
}

void
UDPTransport::closeStream(const TransportContextId& context_id,
                          const StreamId streamId)
{

  if (dequeue_data_map[context_id].count(streamId) > 0) {
    dequeue_data_map[context_id].erase(streamId);
  }
}

void
UDPTransport::close(const TransportContextId& context_id)
{

  if (not isServerMode) {
    if (fd >= 0) {
      ::close(fd);
    }
    fd = -1;

  } else {
    addrKey ak;

    addr_to_key(remote_contexts[context_id].addr, ak);

    remote_addrs.erase(ak);
    remote_contexts.erase(context_id);
    dequeue_data_map.erase(context_id);
  }
}

void
UDPTransport::addr_to_key(sockaddr_storage& addr, addrKey& key)
{

  key.port = 0;
  key.ip_lo = 0;
  key.ip_hi = 0;

  switch (addr.ss_family) {
    case AF_INET: {
      sockaddr_in* s = (sockaddr_in*)&addr;

      key.port = s->sin_port;
      key.ip_lo = s->sin_addr.s_addr;
      break;
    }
    default: {
      // IPv6
      sockaddr_in6* s = (sockaddr_in6*)&addr;

      key.port = s->sin6_port;

      key.ip_hi = (uint64_t)&s->sin6_addr;
      key.ip_lo = (uint64_t)&s->sin6_addr + 8;
      break;
    }
  }
}

void
UDPTransport::addr_to_remote(sockaddr_storage& addr, TransportRemote& remote)
{
  char ip[INET6_ADDRSTRLEN];

  remote.proto = TransportProtocol::UDP;

  switch (addr.ss_family) {
    case AF_INET: {
      sockaddr_in* s = (sockaddr_in*)&addr;

      remote.port = s->sin_port;
      inet_ntop(AF_INET, &s->sin_addr, ip, sizeof(ip));
      break;
    }
    default: {
      // IPv6
      sockaddr_in6* s = (sockaddr_in6*)&addr;

      remote.port = s->sin6_port;
      inet_ntop(AF_INET6, &s->sin6_addr, ip, sizeof(ip));
      break;
    }
  }
}

/*
 * Blocking socket writer. This should be called in its own thread
 *
 * Writer will perform the following:
 *  - loop reads data from fd_write_queue and writes it to the socket
 */
void
UDPTransport::fd_writer()
{

  logger.log(LogLevel::info, "Starting transport writer thread");

  while (not stop) {
    auto cd = fd_write_queue.block_pop();

    if (cd) {
      std::cerr << "Write Queue Pop got an item " << std::endl;
      if ((dequeue_data_map.count(cd->contextId) == 0 || dequeue_data_map[cd->contextId].count(cd->streamId) == 0)
          || remote_contexts.count(cd->contextId) == 0) {
        // Drop/ignore connection data since the connection or stream no
        // longer exists
        std::cerr << "fd_writer no data ?" << std::endl; 
        continue;
      }

      auto& r = remote_contexts.at(cd->contextId);
      std::cerr << "Sending  data of size " << cd.value().data.size() << std::endl; 
      int numSent = sendto(fd,
                           cd.value().data.data(),
                           cd.value().data.size(),
                           0 /*flags*/,
                           (struct sockaddr*)&r.addr,
                           sizeof(sockaddr_in));

      if (numSent < 0) {
        std::cerr << "Error sending on UDP socket: " << strerror(errno) << std::endl;
        continue;
      } else if (numSent != (int)cd.value().data.size()) {
        std::cerr << "Num sent mismatch" << std::endl;
        continue;
      }
    }
  }
}

/*
 * Blocking socket FD reader. This should be called in its own thread.
 *
 * Reader will perform the following:
 *  - Receive data from socket
 *  - Lookup addr in map to find context and name info
 *  - If context doesn't exist, then it's a new connection and the delegate will
 * be called after creating new context
 *  - Create connData and send to queue
 *  - Call on_recv_notify() delegate to notify of new data available. This is
 * not called again if there is still pending data to be dequeued for the same
 * StreamId
 */
void
UDPTransport::fd_reader()
{
  logger.log(LogLevel::info, "Starting transport reader thread");
#if 0
  char rx_buffer[1024];
  
  while (!stop) {
    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    socklen_t socklen = sizeof(source_addr);
    int len = recvfrom(fd, 
                        rx_buffer, 
                        sizeof(rx_buffer) - 1, 
                        0, 
                        (struct sockaddr *)&source_addr, &socklen);

    // Error occurred during receiving
    if (len < 0 && !stop) {
        std::cout <<  "recvfrom failed " << errno <<  std::endl;
        continue;
    } else {
        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
        std::cout << "Received " << len << " bytes" << std::endl;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
#endif  

  const int dataSize = 1024; // TODO Add config var to set this value.  Sizes
                              // larger than actual MTU require IP frags
  struct sockaddr_storage remoteAddr;
  memset(&remoteAddr, 0, sizeof(remoteAddr));
  socklen_t remoteAddrLen = sizeof(remoteAddr);

  uint8_t data[dataSize];

  while (not stop) {
    int rLen = recvfrom(fd,
                        data,
                        dataSize,
                        0 /*flags*/,
                        (struct sockaddr*)&remoteAddr,
                        &remoteAddrLen);

    if (rLen < 0) {
      if ((errno == EAGAIN) || (stop)) {
        // timeout on read or stop issued
        continue;

      } else {
        std::cerr << "Error reading from UDP socket: " << strerror(errno) << std::endl;
        continue;
      }
    }

    if (rLen == 0) {
      continue;
    }

    std::vector<uint8_t> buffer (data, data + rLen);

    connData cd;
    cd.data = buffer;
    cd.streamId = 0;

    addrKey ra_key;
    addr_to_key(remoteAddr, ra_key);

    if (remote_addrs.count(ra_key) == 0) {
        continue;
    } else {
      auto sctx = remote_addrs[ra_key];
      cd.contextId = sctx.tcid;
      cd.streamId = sctx.sid;
    }

    // Add data to caller queue for processing
    auto& dq = dequeue_data_map[cd.contextId][cd.streamId];

    // TODO: Notify caller that packets are being dropped on queue full
    dq.push(cd);
    if (dq.size() < 2) {
      // Notify the caller that there is data to process
      delegate.on_recv_notify(cd.contextId, cd.streamId);
    }
  }   
}

TransportError
UDPTransport::enqueue(const TransportContextId& context_id,
                      const StreamId& streamId,
                      std::vector<uint8_t>&& bytes)
{
  if (bytes.empty()) {
    return TransportError::None;
  }

#if 0
  struct sockaddr_in dest_addr;
  dest_addr.sin_addr.s_addr = inet_addr("192.168.50.143");
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(1234);
  int addr_family = AF_INET;
  int ip_protocol = IPPROTO_IP;
  std::cout << "sending to fd " << fd << std::endl;

  // send a message
  int err = sendto(fd, 
                  bytes.data(), 
                  bytes.size(), 
                  0, 
                  (struct sockaddr *)&dest_addr, 
                  sizeof(dest_addr));
  if (err < 0) {
      std::cout << "Error occurred during sending: errno " << errno << std::endl;
      return TransportError::None;
  }
  
  std::cout << "Sending done " << std::endl;
#endif  

  if (remote_contexts.count(context_id) == 0) {
    // Invalid context id
    std::cerr << "enqueue: invalid context id" << std::endl;
    return TransportError::InvalidContextId;
  }

  if (dequeue_data_map[context_id].count(streamId) == 0) {
    // Invalid stream Id
    std::cerr << "enqueue: invalid stream id" << std::endl;
    return TransportError::InvalidStreamId;
  }

  connData cd;
  cd.data = bytes;
  cd.contextId = context_id;
  cd.streamId = streamId;

  if (not fd_write_queue.push(cd)) {
    std::cerr << "enqueue: queue full" << std::endl;
    return TransportError::QueueFull;
  }

  return TransportError::None;
}

std::optional<std::vector<uint8_t>>
UDPTransport::dequeue(const TransportContextId& context_id,
                      const StreamId& streamId)
{

  if (remote_contexts.count(context_id) == 0) {
    std::cerr << "dequeue: invalid context id: " << context_id << std::endl;
    return std::nullopt;
  }

  if (dequeue_data_map[context_id].count(streamId) == 0) {
    std::cerr << "dequeue: invalid stream id: " << streamId << std::endl;
    return std::nullopt;
  }

  auto& dq = dequeue_data_map[context_id][streamId];

  if (dq.size() <= 0) {
    return std::nullopt;
  }

  return dq.pop().value().data;
}

TransportContextId
UDPTransport::connect_client()
{
  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd == -1) {
    abort();
  }

  // Set timeout
  struct timeval timeout;
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
  setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
  
  struct sockaddr_in srvAddr;
  srvAddr.sin_family = AF_INET;
  srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  srvAddr.sin_port = 0;
  auto err = bind(fd, (struct sockaddr*)&srvAddr, sizeof(srvAddr));
  if (err) {
    std::cout << "client_connect: Unable to bind to socket: " << strerror(errno) << std::endl;
    abort();
  }

  std::string sPort = std::to_string(htons(serverInfo.port));
  struct addrinfo hints = {}, *address_list = NULL;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  err = getaddrinfo(
    serverInfo.host_or_ip.c_str(), sPort.c_str(), &hints, &address_list);
  if (err) {
    std::cerr << "client_connect: Unable to resolve remote ip address: "
          << strerror(errno) << std::endl;
    abort();
  }

  struct addrinfo *item = nullptr, *found_addr = nullptr;
  for (item = address_list; item != nullptr; item = item->ai_next) {
    if (item->ai_family == AF_INET && item->ai_socktype == SOCK_DGRAM &&
        item->ai_protocol == IPPROTO_UDP) {
      found_addr = item;
      break;
    }
  }

  if (found_addr == nullptr) {
    logger.log(LogLevel::fatal, "client_connect: No IP address found");
    abort();
  }

  struct sockaddr_in* ipv4 = (struct sockaddr_in*)&serverAddr.addr;
  memcpy(ipv4, found_addr->ai_addr, found_addr->ai_addrlen);
  ipv4->sin_port = htons(serverInfo.port);
  serverAddr.addr_len = sizeof(sockaddr_in);

  freeaddrinfo(address_list);

  addrKey sa_key;
  addr_to_key(serverAddr.addr, sa_key);

  serverAddr.key = sa_key;

  ++last_context_id;
  ++last_stream_id;

  remote_contexts[last_context_id] = serverAddr;
  remote_addrs[sa_key] = { last_context_id, last_stream_id};

  // Create dequeue
  dequeue_data_map[last_context_id][last_stream_id].setLimit(5000);

  // Notify caller that the connection is now ready
  delegate.on_connection_status(last_context_id, TransportStatus::Ready);

  auto cfg = create_config("FDReader", 1, 12 * 1024, 5);
  esp_pthread_set_cfg(&cfg);
  running_threads.emplace_back(&UDPTransport::fd_reader, this);

  cfg = create_config("FDWriter", 1, 12 * 1024, 5);
  esp_pthread_set_cfg(&cfg);
  running_threads.emplace_back(&UDPTransport::fd_writer, this);
  return last_context_id;
}

TransportContextId
UDPTransport::connect_server()
{
  std::stringstream s_log;

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0) {
    s_log << "connect_server: Unable to create socket: " << strerror(errno);
    logger.log(LogLevel::fatal, s_log.str());
    abort();
  }

  // set for re-use
  int one = 1;
  int err =
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(one));
  if (err != 0) {
    s_log << "connect_server: setsockopt error: " << strerror(errno);
    logger.log(LogLevel::fatal, s_log.str());
    abort();
  }

  // TODO: Add config for this value
  size_t snd_rcv_max = 2000000;

  err =
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &snd_rcv_max, sizeof(snd_rcv_max));
  if (err != 0) {
    s_log << "client_connect: Unable to set send buffer size: "
          << strerror(errno);
    logger.log(LogLevel::fatal, s_log.str());
    abort();
  }

  err =
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &snd_rcv_max, sizeof(snd_rcv_max));
  if (err != 0) {
    s_log << "client_connect: Unable to set receive buffer size: "
          << strerror(errno);
    logger.log(LogLevel::fatal, s_log.str());
    abort();
  }

  struct sockaddr_in srv_addr;
  memset((char*)&srv_addr, 0, sizeof(srv_addr));
  srv_addr.sin_port = htons(serverInfo.port);
  srv_addr.sin_family = AF_INET;
  srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  err = bind(fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
  if (err < 0) {
    s_log << "connect_server: unable to bind to socket: " << strerror(errno);
    logger.log(LogLevel::fatal, s_log.str());
    abort();
  }

  s_log.str(std::string());
  s_log << "connect_server: port: " << serverInfo.port << " fd: " << fd;
  logger.log(LogLevel::info, s_log.str());

  running_threads.emplace_back(&UDPTransport::fd_reader, this);
  running_threads.emplace_back(&UDPTransport::fd_writer, this);

  return last_context_id;
}
