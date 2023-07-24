/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "Wifi.hh"
#include "Logging.hh"
#include "SerialLogger.hh"
#include "SerialEsp.hh"
#include "SerialManager.hh"
#include "NetManager.hh"

#include "NetPins.hh"



static const char* TAG = "net-main";


// Defines

// #define BUFFER_SIZE (128)

// static QueueHandle_t uart1_queue;
static hactar_utils::LogManager* logger;
static hactar_utils::Wifi wifi;

static NetManager* manager;
static SerialEsp* ui_uart1;
static SerialManager* ui_layer;

// Forward declare functions
void Setup();
void Run();

// static void HandleOutGoingSerial()
// {
//     ui_layer->Rx(0);
// }

// static void IRAM_ATTR gpio_isr_handler(void* arg)
// {
//     uint32_t gpio_num = (uint32_t) arg;
//     xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
// }

// static void gpio_task_example(void* arg)
// {
//     uint32_t io_num;
//     for(;;) {
//         if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
//             printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level((gpio_num_t)io_num));
//         }
//     }
// }


extern "C" void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    // vTaskDelay(10000 / portTICK_PERIOD_MS);

    // Configure the uart
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_MAX,
        .source_clk = UART_SCLK_DEFAULT // UART_SCLK_DEFAULT
    };

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = NET_STAT_SEL;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(NET_STAT_Pin, 0);

    // LED init
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = LEDS_OUTPUT_SEL;
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);

    // Debug init
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = NET_DBG_SEL;
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);

    // TODO put somewhere else?
    ui_uart1 = new SerialEsp(UART1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, uart_config, 32);
    ui_layer = new SerialManager(ui_uart1);
    manager = new NetManager(ui_layer);

    gpio_set_level(LED_R_Pin, 1);
    gpio_set_level(LED_G_Pin, 1);
    gpio_set_level(LED_B_Pin, 1);
    gpio_set_level(NET_DBG5_Pin, 0);
    gpio_set_level(NET_DBG6_Pin, 0);

    int next = 0;
    gpio_set_level(LED_R_Pin, 0);

    // Ready for normal operations
    gpio_set_level(NET_STAT_Pin, 1);

    int32_t count = 0;
    while (1)
    {
        gpio_set_level(LED_R_Pin, next);
        next = next ? 0 : 1;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void Setup()
{
    logger = hactar_utils::LogManager::GetInstance();
    logger->add_logger(new hactar_utils::ESP32SerialLogger());
    logger->info(TAG, "Net app_main start");

    esp_event_loop_create_default();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        logger->warn(TAG, "net - nvs_flash_init - no free-pages/version issue");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);

    // Wifi setup
    hactar_utils::Wifi::State wifi_state = { hactar_utils::Wifi::State::NotInitialized };

    wifi.init();
    // TODO wifi messages from the serial manager
}

void Run()
{
    static bool subscribed = false;
    // wifi_monitor();
}
