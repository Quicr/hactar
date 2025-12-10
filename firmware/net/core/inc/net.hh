#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "logger.hh"
#include "moq_session.hh"
#include "nvs_flash.h"
#include "uart.hh"
#include "ui_net_link.hh"
#include "wifi.hh"
#include <nlohmann/json.hpp>
#include <memory>

#define NET_UI_UART_PORT UART_NUM_1
#define NET_UI_UART_DEV UART1
#define NET_UI_UART_TX_PIN 17
#define NET_UI_UART_RX_PIN 18
#define NET_UI_UART_RX_BUFF_SIZE 8192
#define NET_UI_UART_TX_BUFF_SIZE 8192
#define NET_UI_UART_RING_TX_NUM 30
#define NET_UI_UART_RING_RX_NUM 30

#define NET_MGMT_UART_RX_BUFF_SIZE 256
#define NET_MGMT_UART_TX_BUFF_SIZE 256
#define NET_MGMT_UART_RING_TX_NUM 3
#define NET_MGMT_UART_RING_RX_NUM 3
#define NET_MGMT_UART_TX_PIN GPIO_NUM_43
#define NET_MGMT_UART_RX_PIN GPIO_NUM_44

extern Wifi wifi;
extern Uart ui_layer;
extern std::shared_ptr<moq::Session> moq_session;
extern SemaphoreHandle_t audio_req_smpr;

using json = nlohmann::json;

bool CreateUILinkPacketTask();
bool CreateMgmtLinkPacketTask();
void StopMoqSession(std::shared_ptr<moq::Session>& session,
                    std::vector<std::shared_ptr<moq::TrackReader>>& readers,
                    std::vector<std::shared_ptr<moq::TrackWriter>>& writers);
void RestartMoqSession(std::shared_ptr<moq::Session>& session,
                       quicr::ClientConfig& config,
                       std::vector<std::shared_ptr<moq::TrackReader>>& readers,
                       std::vector<std::shared_ptr<moq::TrackWriter>>& writers);
std::shared_ptr<moq::TrackReader> CreateReadTrack(const json& subscription, Uart& serial);
std::shared_ptr<moq::TrackWriter> CreateWriteTrack(const json& subscription);
