#include<future>
#include "esp_pthread.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <condition_variable>
#include <future>
#include <map>
#include <mutex>
#include <memory>
#include <thread>

#include "qsession.h"
#include "transport/transport.h"

static const uint16_t default_ttl_ms = 1000;


static esp_pthread_cfg_t create_config(const char *name, int core_id, int stack, int prio)
{
    auto cfg = esp_pthread_get_default_config();
    cfg.thread_name = name;
    cfg.pin_to_core = core_id;
    cfg.stack_size = stack;
    cfg.prio = prio;
    return cfg;
}

QSession::QSession(quicr::RelayInfo relay)
: inbound_objects(std::make_shared<AsyncQueue<QuicrObject>>()) {
  logger = std::make_shared<cantina::Logger>("qsession_logger");
  logger->Log("Connecting to " + relay.hostname + ":" +
              std::to_string(relay.port));
  qtransport::TransportConfig tcfg{ .tls_cert_filename = NULL,
                                    .tls_key_filename = NULL };
  client = std::make_unique<quicr::Client>(relay, tcfg, logger);

}


bool 
QSession::connect() {
   // Connect to the quicr relay
  if (!client->connect()) {
    return false;
  }

  auto cfg = create_config("QuicRMessageHandler", 1, 12 * 1024, 5);
  auto esp_err = esp_pthread_set_cfg(&cfg);
  if(esp_err != ESP_OK) {
    std::stringstream s_log;
    s_log << "esp_pthread_set_cfg failed " << esp_err_to_name(esp_err);
    logger->Log(s_log.str());
    return false;
  }

  // Start up a thread to handle incoming messages
  handler_thread = std::thread([&]() {
    while (!stop) {
      auto maybe_obj = inbound_objects->pop(inbound_object_timeout);
      if (!maybe_obj) {
        continue;
      }

      //const auto _ = std::lock();
      auto& obj = maybe_obj.value();
      handle(std::move(obj));
    }

    logger->Log("Handler thread stopping");
  });

  return true;
}

bool 
QSession::subscribe(quicr::Namespace ns) {
  if (sub_delegates.count(ns)) {
    return true;
  }


  auto promise = std::promise<bool>();
  auto future = promise.get_future();
  const auto delegate =
    std::make_shared<SubDelegate>(logger, inbound_objects, std::move(promise));

  logger->Log("Subscribe to " + std::string(ns));
  quicr::bytes empty{};
  client->subscribe(delegate,
                    ns,
                    quicr::SubscribeIntent::immediate,
                    "bogus_origin_url",
                    false,
                    "bogus_auth_token",
                    std::move(empty));

  const auto success = future.get();
  if (success) {
    sub_delegates.insert_or_assign(ns, delegate);
  }

  return success;

}

bool
QSession::publish_intent(quicr::Namespace ns)
{
  logger->Log("Publish Intent for namespace: " + std::string(ns));
  auto promise = std::promise<bool>();
  auto future = promise.get_future();
  const auto delegate =
    std::make_shared<PubDelegate>(logger, std::move(promise));

  client->publishIntent(delegate, ns, {}, {}, {});
  return future.get();
}

void QSession::unsubscribe(quicr::Namespace nspace){}

void
QSession::publish(const quicr::Name& name, quicr::bytes&& data)
{
  logger->Log("Publish, name=" + std::string(name) +
              " size=" + std::to_string(data.size()));
  client->publishNamedObject(name, 0, default_ttl_ms, false, std::move(data));
}


void
QSession::handle(QuicrObject&& obj)
{
  // tie to the right delegate for further processing
}
