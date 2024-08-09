#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "logger.hh"
#include "SerialEsp.hh"
#include "SerialPacketManager.hh"
#include "NetManager.hh"

#include "Wifi.hh"

#include <qsession.h>

static NetManager* manager = nullptr;
static SerialEsp* ui_uart1 = nullptr;
static SerialPacketManager* ui_layer = nullptr;
static Wifi* wifi = nullptr;
static std::shared_ptr<QSession> qsession = nullptr;
static std::shared_ptr<AsyncQueue<QuicrObject>> inbound_queue;

static bool qsession_connected = false;

static void SetupPins();
static void SetupComponents();


