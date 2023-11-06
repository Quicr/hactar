#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "Logging.hh"
#include "SerialLogger.hh"
#include "SerialEsp.hh"
#include "SerialManager.hh"
#include "NetManager.hh"

#include "Wifi.hh"

#include <qsession.h>

static const char* TAG = "[Net-Main]";

static hactar_utils::LogManager* logger;

static NetManager* manager = nullptr;
static SerialEsp* ui_uart1 = nullptr;
static SerialManager* ui_layer = nullptr;
static std::shared_ptr<QSession> qsession = nullptr;
static Wifi* wifi = nullptr;

static bool qsession_connected = false;

static void GpioInit();
static void UartInit();
static void Setup();
static void Run();

// TODO move into wifi?
static void WifiWatchdog();



