#include "net_test.hh"

#include "sdkconfig.h"

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

#include "peripherals.hh"
#include "serial.hh"
#include "wifi.hh"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "wifi_creds.hh"

#include "esp_event.h"

#include "ui_net_link.hh"

#define TEST_SERVER_IP "192.168.50.20"
#define TEST_SERVER_PORT 12345

#define NET_UI_UART_PORT UART_NUM_1
#define NET_UI_UART_DEV UART1
#define NET_UI_UART_TX_PIN 17
#define NET_UI_UART_RX_PIN 18
#define NET_UI_UART_RX_BUFF_SIZE 1024
#define NET_UI_UART_TX_BUFF_SIZE 1024
#define NET_UI_UART_RING_TX_NUM 30
#define NET_UI_UART_RING_RX_NUM 30

Serial* ui_link;


extern "C" void app_main(void)
{
    InitializeGPIO();
    IntitializeLEDs();
    // Not using for now
    // IntitializePWM();

    printf("Internal SRAM available: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("PSRAM available: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    // Configure the uart
    uart_config_t net_ui_uart_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ui_link = new Serial(NET_UI_UART_PORT, NET_UI_UART_DEV, ETS_UART1_INTR_SOURCE,
        net_ui_uart_config,
        NET_UI_UART_TX_PIN, NET_UI_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
        NET_UI_UART_RX_BUFF_SIZE, NET_UI_UART_TX_BUFF_SIZE,
        NET_UI_UART_RING_TX_NUM, NET_UI_UART_RING_RX_NUM);

    // Wifi wifi;
    // wifi.Connect(SSID, SSID_PWD);

    // while (!wifi.IsConnected())
    // {
    //     ESP_LOGW("net.cc", "REMOVE ME - Waiting to connect to wifi");
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    // TODO we need to wait until the tcp client is also connected before we say we can take
    // packets
    // TCPClientBegin();

    gpio_set_level(NET_LED_R, 1);
    gpio_set_level(NET_LED_G, 1);
    gpio_set_level(NET_LED_B, 1);
    gpio_set_level(NET_DEBUG_1, 0);
    gpio_set_level(NET_DEBUG_2, 0);
    gpio_set_level(NET_DEBUG_3, 0);

    // Ready for normal operations
    gpio_set_level(NET_STAT, 1);


    Logger::Log(Logger::Level::Info, "Start!");
    link_packet_t* recv = nullptr;
    while (1)
    {
        vTaskDelay(5 / portTICK_PERIOD_MS);
        while ((recv = ui_link->Read()))
        {
            switch ((ui_net_link::Packet_Type)recv->type)
            {
                case ui_net_link::Packet_Type::AudioObject:
                {
                    ui_link->Write(recv);
                    break;
                }
                default:
                    break;
            }

        }
    }
}

void SetupComponents()
{
}
