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

// #include "SerialEsp.hh"

static const char* TAG = "net-main";

// Defines
#define LED_B_IO            GPIO_NUM_5
#define LED_G_IO            GPIO_NUM_6
#define LED_R_IO            GPIO_NUM_7
#define LEDS_OUTPUT_SEL     1 << LED_B_IO | 1 << LED_G_IO | 1 << LED_R_IO

#define UART0 UART_NUM_0
#define UART1 UART_NUM_1
#define PATTERN_CHR_NUM (3) // Number of consecutive bytes that define a pattern

// #define BUFFER_SIZE (128)

// static QueueHandle_t uart1_queue;
static SerialEsp* ui_uart0;
static SerialEsp* ui_uart1;

static void uart1_event_task(void *pvParameters)
{
    // while (true)
    // {
    //     ui_uart->RxEvent(pvParameters);

    // }


    return;
    // uart_event_t event;
    // size_t num_buffered;
    // uint8_t* dtmp = new uint8_t[BUFFER_SIZE];

    // while (true)
    // {
    //     if (xQueueReceive(uart1_queue, (void*)&event, portMAX_DELAY))
    //     {
    //         // Zero all bytes in dtmp
    //         bzero(dtmp, BUFFER_SIZE);

    //         ESP_LOGI(TAG, "uart[%d] event:", UART1);
    //         switch (event.type)
    //         {
    //             case UART_DATA:
    //             {
    //                 ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
    //                 uart_read_bytes(UART1, dtmp, event.size, portMAX_DELAY);


    //                 break;
    //             }
    //             default:
    //             {
    //                 break;
    //             }

    //         }
    //     }
    // }

    // delete dtmp;
}

static void HandleIncomingSerial()
{
    // ui_layer->Tx(0);
}

static void HandleOutGoingSerial()
{
    // ui_layer->Rx(0);
}

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

    uart_driver_delete(UART0);

    // Configure the uart
    // TODO move into serialesp?
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_MAX,
        .source_clk = UART_SCLK_APB // UART_SCLK_DEFAULT
    };

    ui_uart0 = new SerialEsp(UART0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, uart_config, 32);
    ui_uart1 = new SerialEsp(UART1, GPIO_NUM_11, GPIO_NUM_10, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, uart_config, 32);

    // uart_driver_install(UART1, BUFFER_SIZE * 2, BUFFER_SIZE * 2, 20, &uart1_queue, 0);
    // uart_param_config(UART1, &uart_config);

    // uart_set_pin(UART1, GPIO_NUM_10, GPIO_NUM_11, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // xTaskCreate(uart1_event_task, "ui_uart_event_task", 2048, NULL, 12, NULL);
    // Not using patterns
    // uart_enable_pattern_det_baud_intr(UART1, '+', PATTERN_CHR_NUM, 9, 0, 0);
    // uart_pattern_queue_reset(UART1, 20);




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

    // //start gpio task
    // xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());

    int cnt = 0;
    while (1)
    {
        cnt++;
        // printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        // gpio_set_level((gpio_num_t)LED_R_IO, cnt % 2);
        gpio_set_level((gpio_num_t)LED_G_IO, cnt % 2);
        // gpio_set_level((gpio_num_t)LED_B_IO, cnt % 2);
        // gpio_set_level((gpio_num_t)GPIO_OUTPUT_IO_1, cnt % 2);
    }
}
