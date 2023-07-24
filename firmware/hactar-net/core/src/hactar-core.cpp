#include "hactar-core.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "serial_logger.h"

#include "test_logger.h"

#include <memory>
#include <iostream>

using namespace hactar_net;


static const char *TAG = "net_core";

TestLogger test_logger;
    
// quicr helpers
class subDelegate : public quicr::SubscriberDelegate
{
public:
  subDelegate(TestLogger& logger)
    : logger(logger)
  {
  }

  void onSubscribeResponse(
    [[maybe_unused]] const quicr::Namespace& quicr_namespace,
    [[maybe_unused]] const quicr::SubscribeResult& result) override
  {

    std::cout << "onSubscriptionResponse: name: " << quicr_namespace.to_hex()
            << " status: " << int(static_cast<uint8_t>(result.status)) 
            << std::endl;

  }

  void onSubscriptionEnded(
    [[maybe_unused]] const quicr::Namespace& quicr_namespace,
    [[maybe_unused]] const quicr::SubscribeResult::SubscribeStatus& reason)
    override
  {
  }

  void onSubscribedObject([[maybe_unused]] const quicr::Name& quicr_name,
                          [[maybe_unused]] uint8_t priority,
                          [[maybe_unused]] uint16_t expiry_age_ms,
                          [[maybe_unused]] bool use_reliable_transport,
                          [[maybe_unused]] quicr::bytes&& data) override
  {
    std::cout << "onSubscribedObject:  Name: " << quicr_name.to_hex()
            << " data sz: " << data.size() << std::endl;
  }

  void onSubscribedObjectFragment(
    [[maybe_unused]] const quicr::Name& quicr_name,
    [[maybe_unused]] uint8_t priority,
    [[maybe_unused]] uint16_t expiry_age_ms,
    [[maybe_unused]] bool use_reliable_transport,
    [[maybe_unused]] const uint64_t& offset,
    [[maybe_unused]] bool is_last_fragment,
    [[maybe_unused]] quicr::bytes&& data) override
  {
  }

private:
  TestLogger& logger;
};

std::shared_ptr<subDelegate> sub_delegate = nullptr;

class pubDelegate : public quicr::PublisherDelegate
{
public:
  void onPublishIntentResponse(
    [[maybe_unused]] const quicr::Namespace& quicr_namespace,
    [[maybe_unused]] const quicr::PublishIntentResult& result) override
  {
  }
};


std::shared_ptr<pubDelegate> pd = nullptr;
auto name = quicr::Name("abcd");

void HactarApp::wifi_monitor() {

    auto state = wifi.get_state();
    switch (state)
    {
        case hactar_utils::Wifi::State::ReadyToConnect:
        case hactar_utils::Wifi::State::Disconnected: {
            logger->info(TAG, "Wifi : Ready to connect/Disconnected\n");
            wifi.connect();
        }
        default:
            break;
    }

}

void HactarApp::run() {
    static bool subscribed = false;
    wifi_monitor();
    auto state = wifi.get_state();
    // quicr setup
    if (state == hactar_utils::Wifi::State::Connected) {
        if(qclient == nullptr) {
            char defaultRelay[] = "192.168.50.143";
            auto relayName = defaultRelay;
            int port = 1234;

            quicr::RelayInfo relay{ .hostname = relayName,
                                    .port = uint16_t(port),
                                    .proto = quicr::RelayInfo::Protocol::UDP };
            qclient = new quicr::QuicRClient{relay, {}, test_logger};  
        } else {
            if(subscribed)
              return;
            logger->info(TAG, "Subscribe \n");
            quicr::Namespace nspace(0xA11CEE00000001010007000000000000_name, 80);
            std::cout << "Subscribe to " << nspace.to_hex()  << std::endl;
            quicr::SubscribeIntent intent = quicr::SubscribeIntent::immediate;
            quicr::bytes empty;
            qclient->subscribe(
            sub_delegate, nspace, intent, "origin_url", false, "auth_token", std::move(empty));
            subscribed = true;

        }
    } 

}

void HactarApp::setup() {

   logger = hactar_utils::LogManager::GetInstance();
   logger->add_logger(new hactar_utils::ESP32SerialLogger());
   logger->info(TAG,  "Hactar Net app_main start");

    esp_event_loop_create_default();
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        logger->warn(TAG, "nvs_flash_init - no free-pages/version issue ");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);

    // Wifi setup
    hactar_utils::Wifi::State wifi_state { hactar_utils::Wifi::State::NotInitialized };
    wifi.SetCredentials("CTO", "FionaTheHippo");
    
    wifi.init();

    // TODO : add error checks

}

extern "C" void app_main(void) {    
    HactarApp app;
    app.setup();
    sub_delegate = std::make_shared<subDelegate>(test_logger);        
    while(true) {
        app.run();
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}