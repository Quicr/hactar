#include "net.hh"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <iostream>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "peripherals.hh"
#include "serial.hh"

QueueHandle_t uart_queue;

extern "C" void app_main(void)
{
    InitializeGPIO();
    IntitializeLEDs();
    // Not using for now
    // IntitializePWM();


    // Configure the uart
    uart_config_t uart1_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT // UART_SCLK_DEFAULT
    };

    InitializeQueuedUART(uart1_config, UART1, uart_queue,
        RX_BUFF_SIZE, TX_BUFF_SIZE,
        EVENT_QUEUE_SIZE, TX_PIN, RX_PIN,
        RTS_PIN, CTS_PIN, ESP_INTR_FLAG_LOWMED);

    gpio_set_level(NET_LED_R, 1);
    gpio_set_level(NET_LED_G, 1);
    gpio_set_level(NET_LED_B, 1);
    gpio_set_level(NET_DEBUG_1, 0);
    gpio_set_level(NET_DEBUG_2, 0);
    gpio_set_level(NET_DEBUG_3, 0);

    // Ready for normal operations
    gpio_set_level(NET_STAT, 1);

    Serial ui_layer(UART1,  uart_queue, 4096, 4096, 1024, 1024);

    // This is the lazy way of doing it, otherwise we should use a esp_timer.
    uint32_t blink_cnt = 0;
    while (1)
    {


        // manager->Update();

        // auto state = wifi->GetState();
        // if (state == Wifi::State::Connected && !qsession_connected)
        // {
        //     Logger::Log(Logger::Level::Info, "Net app_main Connecting to QSession");
        //     qsession->connect();
        //     qsession_connected = true;
        // }

        vTaskDelay(1600 / portTICK_PERIOD_MS);
    }
}

void SetupComponents()
{


    // wifi = new Wifi(*ui_layer);

    // inbound_queue = std::make_shared<AsyncQueue<QuicrObject>>();
    char default_relay [] = "192.168.50.20";
    auto relay_name = default_relay;
    uint16_t port = 1234;
    // quicr::RelayInfo relay{
    //     .hostname = relay_name,`````````
    //     .port = port,
    //     .proto = quicr::RelayInfo::Protocol::UDP
    // };
    // qsession = std::make_shared<QSession>(relay, inbound_queue);

    // manager = new NetManager(*ui_layer, *wifi, qsession, inbound_queue);

    // Wait for wifi to finish initializing
    // while (!wifi->IsInitialized())
    // {
    //     vTaskDelay(10 / portTICK_PERIOD_MS);
    // }

    Logger::Log(Logger::Level::Info, "Components ready");
}
