#ifndef NET_HH
#define NET_HH

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "logger.hh"
// #include "serial_esp.hh"
// #include "serial_packet_manager.hh"
// #include "net_manager.hh"

// #include "wifi.hh"

// #include "qsession.hh"

// static NetManager* manager = nullptr;
// static SerialEsp* ui_uart1 = nullptr;
// static SerialPacketManager* ui_layer = nullptr;
// static Wifi* wifi = nullptr;
// static std::shared_ptr<QSession> qsession = nullptr;
// static std::shared_ptr<AsyncQueue<QuicrObject>> inbound_queue;

static bool qsession_connected = false;

#endif