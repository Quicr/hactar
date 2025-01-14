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

#include "wifi.hh"

#include <memory>

struct DeviceSetupConfig {
    std::string moq_connect_uri {"moq://192.168.10.246:1234"};
    std::string moq_endpoint_id {"hactar-12-suhas"};
};


static SerialEsp* ui_uart1 = nullptr;
static std::unique_ptr<SerialPacketManager> ui_layer = nullptr;
static SerialPacketManager* audio_layer = nullptr;
static void SetupPins();
static void SetupComponents(const DeviceSetupConfig& config);
static void PostSetup();

