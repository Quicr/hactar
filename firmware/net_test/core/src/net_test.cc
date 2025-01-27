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
    QueueHandle_t uart_queue;
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

    // TODO move ui layer init to this function
    // ui_link = InitializeQueuedUART(uart1_config, UI_UART2, uart_queue,
    //     RX_BUFF_SIZE, TX_BUFF_SIZE,
    //     EVENT_QUEUE_SIZE, TX_PIN, RX_PIN,
    //     RTS_PIN, CTS_PIN, ESP_INTR_FLAG_LOWMED,
    //     SERIAL_TX_TASK_SZ, SERIAL_RX_TASK_SZ,
    //     SERIAL_RING_TX_NUM, SERIAL_RING_RX_NUM);

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


    // This is the lazy way of doing it, otherwise we should use a esp_timer.
    uint32_t blink_cnt = 0;
    uint32_t last_check = 0;

    Logger::Log(Logger::Level::Info, "Start!");
    link_packet_t* recv = nullptr;
    while (1)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        recv = ui_link->Read();
        if (recv == nullptr)
        {
            continue;
        }

        ui_link->Write(recv);

        // Logger::Log(Logger::Level::Info, "Net Alive");
        // Logger::Log(Logger::Level::Info, "rx intr", 
            // ui_link->num_rx_intr, "num recv", ui_link->num_rx_recv);

        // ESP_LOGI("main", "net alive, rx intr %ld, rx recv %ld, data[0]=%d, data[1]=%d, data[2]=%d, data[3]=%d",
        //     ui_link->num_rx_intr, ui_link->num_rx_recv, ui_link->rx(0), ui_link->rx(1), ui_link->rx(2), ui_link->rx(3));


        // manager->Update();

        // auto state = wifi->GetState();
        // if (state == Wifi::State::Connected && !qsession_connected)
        // {
        //     Logger::Log(Logger::Level::Info, "Net app_main Connecting to QSession");
        //     qsession->connect();
        //     qsession_connected = true;
        // }

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
