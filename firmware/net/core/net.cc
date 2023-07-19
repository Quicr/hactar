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

#include "SerialEsp.hh"
#include "SerialManager.hh"
#include "NetManager.hh"

#include "NetPins.h"

static const char* TAG = "net-main";


// Defines

// #define BUFFER_SIZE (128)

// static QueueHandle_t uart1_queue;
static NetManager* manager;
static SerialEsp* ui_uart1;
static SerialManager* ui_layer;

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

    // const int uart_buffer_size = (1024 * 2);
    // QueueHandle_t uart_queue;

    // ESP_ERROR_CHECK(uart_param_config(UART1, &uart_config));
    // ESP_ERROR_CHECK(uart_set_pin(UART1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    // ESP_ERROR_CHECK(uart_driver_install(UART1, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));

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
    const char buff[] = "Net: Message\n\r";


        // Packet* connected_packet = new Packet(xTaskGetTickCount());
        // connected_packet->SetData(Packet::Types::Command, 0, 6);
        // connected_packet->SetData(1, 6, 8);
        // connected_packet->SetData(2, 14, 10);
        // connected_packet->SetData(Packet::Commands::WifiStatus, 24, 8);
        // connected_packet->SetData(1, 32, 8);

        // ui_layer->EnqueuePacket(connected_packet);

    // Ready for normal operations
    gpio_set_level(NET_STAT_Pin, 1);

    int32_t count = 0;
    while (1)
    {
        gpio_set_level(LED_R_Pin, next);
        next = next ? 0 : 1;

        // Packet* connected_packet = new Packet(xTaskGetTickCount());
        // connected_packet->SetData(Packet::Types::Command, 0, 6);
        // connected_packet->SetData(ui_layer->NextPacketId(), 6, 8);
        // connected_packet->SetData(2, 14, 10);
        // connected_packet->SetData(Packet::Commands::WifiStatus, 24, 8);
        // connected_packet->SetData(1, 32, 8);
        // ui_layer->EnqueuePacket(connected_packet);
        // connected_packet = nullptr;

        // Packet* message_packet = new Packet(xTaskGetTickCount());
        // message_packet->SetData(Packet::Types::Message, 0, 6);
        // message_packet->SetData(ui_layer->NextPacketId(), 6, 8);
        // message_packet->SetData(14, 14, 10);

        // for (uint16_t i = 0; i < 15; ++i)
        // {
        //     message_packet->SetData(msg[i], 24 + i * 8, 8);
        // }

        // ui_layer->EnqueuePacket(message_packet);
        // message_packet = nullptr;


        // Handle receiving and transmitting
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
