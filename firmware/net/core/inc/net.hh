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
#include "moq_session.hh"
#include "ui_net_link.hh"

#include <memory>

#define NET_UI_UART_PORT UART_NUM_1
#define NET_UI_UART_DEV UART1
#define NET_UI_UART_TX_PIN 17
#define NET_UI_UART_RX_PIN 18
#define NET_UI_UART_RX_BUFF_SIZE 8192
#define NET_UI_UART_TX_BUFF_SIZE 8192
#define NET_UI_UART_RING_TX_NUM 30
#define NET_UI_UART_RING_RX_NUM 30

extern Wifi wifi;
extern Serial ui_layer;
extern std::shared_ptr<moq::Session> moq_session;
extern std::string base_track_namespace;
extern std::string track_location;
extern std::string sub_track;
extern SemaphoreHandle_t audio_req_smpr;
extern SemaphoreHandle_t sub_change_smpr;

struct DeviceSetupConfig
{
    std::string moq_connect_uri{ "moq://192.168.10.246:1234" };
    std::string moq_endpoint_id{ "hactar-12-suhas" };
};

static void SetupComponents(const DeviceSetupConfig& config);
static bool CreateLinkPacketTask();
static bool CreatePubTask();
static bool CreateSubTask();
