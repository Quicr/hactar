#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "logger.hh"

#include "serial.hh"
#include "wifi.hh"

#include <memory>

struct DeviceSetupConfig {
    std::string moq_connect_uri {"moq://192.168.10.246:1234"};
    std::string moq_endpoint_id {"hactar-12-suhas"};
};


static void SetupComponents(const DeviceSetupConfig& config);

