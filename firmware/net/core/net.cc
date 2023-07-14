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

#include "NetPins.h"

static const char* TAG = "net-main";


// Defines
#define LEDS_OUTPUT_SEL     1 << LED_B_Pin | 1 << LED_G_Pin | 1 << LED_R_Pin

#define UART1 UART_NUM_1
#define PATTERN_CHR_NUM (3) // Number of consecutive bytes that define a pattern

// #define BUFFER_SIZE (128)

// static QueueHandle_t uart1_queue;
static SerialEsp* ui_uart1;
static SerialManager* ui_layer;

// static void HandleIncomingSerial()
// {
//     ui_layer->Tx(0);
// }

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
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_MAX,
        .source_clk = UART_SCLK_DEFAULT // UART_SCLK_DEFAULT
    };

    // const int uart_buffer_size = (1024 * 2);
    // QueueHandle_t uart_queue;

    // ESP_ERROR_CHECK(uart_param_config(UART1, &uart_config));
    // ESP_ERROR_CHECK(uart_set_pin(UART1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    // ESP_ERROR_CHECK(uart_driver_install(UART1, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));





    ui_uart1 = new SerialEsp(UART1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, uart_config, 32);
    ui_layer = new SerialManager(ui_uart1);

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = LEDS_OUTPUT_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    //disable pull-up mode
    io_conf.pull_up_en = (gpio_pullup_t)0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    gpio_set_level(LED_R_Pin, 1);
    gpio_set_level(LED_G_Pin, 1);
    gpio_set_level(LED_B_Pin, 1);
    int next = 0;
    gpio_set_level(LED_R_Pin, 0);
    const char buff[] = "Net: Message\n\r";


        Packet* connected_packet = new Packet(xTaskGetTickCount());
        connected_packet->SetData(Packet::Types::Command, 0, 6);
        connected_packet->SetData(1, 6, 8);
        connected_packet->SetData(2, 14, 10);
        connected_packet->SetData(Packet::Commands::WifiStatus, 24, 8);
        connected_packet->SetData(1, 32, 8);

        ui_layer->EnqueuePacket(connected_packet);
    while (1)
    {
        gpio_set_level(LED_R_Pin, next);
        // printf("Net: Message - led %d\n\r", next);
        next = next ? 0 : 1;




        Packet* connected_packet = new Packet(xTaskGetTickCount());
        connected_packet->SetData(Packet::Types::Command, 0, 6);
        connected_packet->SetData(1, 6, 8);
        connected_packet->SetData(2, 14, 10);
        connected_packet->SetData(Packet::Commands::WifiStatus, 24, 8);
        connected_packet->SetData(1, 32, 8);

        ui_layer->EnqueuePacket(connected_packet);
        connected_packet = nullptr;
        ui_layer->Rx(xTaskGetTickCount());
        Vector<Packet *>& packets = ui_layer->GetRxPackets();
        // printf("Net: Num packets = %d\n\r", packets.size());


        while (packets.size() > 0)
        {
            printf("Net: Num packets = %d\n\r", packets.size());
            packets.erase(0);
        }

        // printf("Before tx");
        ui_layer->Tx(xTaskGetTickCount());


        // Handle receiving and transmitting
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
