#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "logger.hh"
#include "serial_esp.hh"
#include "serial_packet_manager.hh"
#include "moq_session.hh"

#include "wifi.hh"

struct DeviceSetupConfig {
    std::string moq_connect_uri {"moqt://relay.us-west-2.quicr.ctgpoc.com:33437"};
    std::string moq_endpoint_id {"hactar-12-suhas"};
};


static SerialEsp* ui_uart1 = nullptr;
static SerialPacketManager* ui_layer = nullptr;
static SerialPacketManager* audio_layer = nullptr;
static Wifi* wifi = nullptr;
static std::shared_ptr<moqt::Session> moq_session = nullptr;

static void SetupPins();
static void SetupComponents(const DeviceSetupConfig&);
static void PostSetup();

