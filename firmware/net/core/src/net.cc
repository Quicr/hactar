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
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "net_pins.hh"
#include "serial_esp.hh"
#include "serial_packet_manager.hh"
// #include "net_manager.hh"

// #include "wifi.hh"

// #include "qsession.hh"
// #include "logger.hh"

bool ui_chip_ready = false;

constexpr uint32_t Tx_Buff_Size = 1024;
constexpr uint32_t Tx_Buff_Size_2 = Tx_Buff_Size/2;
constexpr uint32_t Rx_Buff_Size = 1024;

const size_t audio_sz = 355;
uint8_t tx_buff[Tx_Buff_Size] = { 0 };
size_t tx_write_mod = 0;
size_t tx_read_mod = 0;
uint8_t rx_buff[Rx_Buff_Size] = { 0 };

constexpr uint32_t Event_Queue_Size = 20;

bool tx_data_ready = false;
uint32_t num_to_send = 0;

uint32_t num_write = 0;
uint32_t num_recv = 0;
bool synced = true;

QueueHandle_t uart_queue;


// TODO try to do a version without two tasks

static void SerialWriteTask(void* param)
{
    int next = 0;
    while (1)
    {
        if (tx_data_ready)
        {
            uint16_t offset = tx_read_mod * Tx_Buff_Size_2;
            uart_write_bytes(UART1, tx_buff+offset, num_to_send);
            tx_data_ready = false;

            ++num_write;

            ESP_LOGW("uart tx", "transmit bytes: %lu, recv %lu sent %lu", num_to_send, num_recv, num_write);
            if (num_write != num_recv)
            {
                synced = false;
                while (true)
                {
                    ESP_LOGE("UART", "Failed to stay synced recv %lu sent %lu", num_recv, num_write);
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                }
            }
            uart_wait_tx_done(UART1, portMAX_DELAY);
            tx_read_mod = !tx_read_mod;
        }
        else
        {
            vTaskDelay(2 / portTICK_PERIOD_MS);
        }
    }
}

static void SerialReadTask(void* param)
{
    uart_event_t event;
    uint8_t data[1024];

    uint32_t total_recv = 0;
    uint32_t events_recv = 0;

    bool partial_packet = false;
    uint16_t packet_len = 0;
    uint8_t packet_type = 0;
    uint16_t bytes_recv = 0;

    uint16_t rx_buff_write_idx = 0;
    uint16_t rx_buff_read_idx = 0;

    size_t bytes_buffered = 0;
    int num_bytes = 0;
    int num_bytes_read = 0;

    while (1)
    {
        // Wait for an event in the queue
        if (!xQueueReceive(uart_queue, (void*)&event, 10 / portTICK_PERIOD_MS))
        {
            continue;
        }

        while (!synced)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        switch (event.type)
        {
            case UART_DATA:
            {
                // By default the max length is 120-121 (timing stuff) as the
                // default fifo buffer size is 128 and an interrupt is sent at
                // 120
                uart_get_buffered_data_len(UART1, &bytes_buffered);
                if (bytes_buffered <= 0)
                {
                    continue;
                }

                num_bytes = uart_read_bytes(UART1, data, bytes_buffered, portMAX_DELAY);
                ++events_recv;


                total_recv += num_bytes;
                // ESP_LOGI("uart rx", "packet status %d, received %lu, events %lu ", (int)partial_packet, total_recv, events_recv);

                uint32_t idx = 0;

                while (num_bytes > 0)
                {
                    if (!partial_packet)
                    {
                        // Start the packet
                        partial_packet = true;

                        // Get the length
                        packet_len = static_cast<uint16_t>(data[idx+0]) << 8 | data[idx+1];

                        // Get the type
                        packet_type = data[idx+2];
                        // idx = 2;

                        // ESP_LOGI("uart rx", "packet len: %u type: %d", packet_len, (int)packet_type);
                    }

                    // Just copy the bytes
                    while (bytes_recv < packet_len && num_bytes > 0)
                    {
                        rx_buff[rx_buff_write_idx++] = data[idx++];
                        ++bytes_recv;
                        --num_bytes;
                    }

                    // HACK This is all hacky
                    if (bytes_recv >= packet_len)
                    {
                        bytes_recv = 0;
                        // Done the packet
                        partial_packet = false;

                        // Determine what to do with it
                        switch (packet_type)
                        {
                            case 1:
                            {
                                // ready message ignore for now
                                break;
                            }
                            case 2:
                            {

                                int tx_read_offset = tx_read_mod * Tx_Buff_Size_2;

                                // Audio copy to the tx buffer
                                for (int i = 0; i < audio_sz; ++i)
                                {
                                    tx_buff[tx_read_offset+i] = rx_buff[i];
                                }

                                tx_read_mod = !tx_read_mod;
                                num_to_send = audio_sz;
                                tx_data_ready = true;
                                ++num_recv;
                                rx_buff_write_idx = 0;

                                break;
                            }
                            default:
                            {
                                num_bytes--;
                            }
                        }
                    }
                }
                break;
            }
            case UART_FIFO_OVF: // FIFO overflow
            {

                printf("UART FIFO Overflow!\n");
                uart_flush_input(UART1); // Flush the input to recover
                xQueueReset(uart_queue);   // Reset the event queue
                break;
            }
            case UART_BUFFER_FULL: // RX buffer full
            {

                printf("UART Buffer Full!\n");
                uart_flush_input(UART1); // Flush the input to recover
                xQueueReset(uart_queue);   // Reset the event queue
                break;
            }
            case UART_PARITY_ERR: // Parity error
            {

                printf("UART Parity Error!\n");
                break;
            }
            case UART_FRAME_ERR: // Frame error
            {

                printf("UART Frame Error!\n");
                break;
            }
            default:
            {

                printf("Unhandled UART event type: %d\n", event.type);
                break;
            }

        }
    }
}

extern "C" void app_main(void)
{
    SetupPins();
    SetupComponents();

    gpio_set_level(NET_LED_R, 1);
    gpio_set_level(NET_LED_G, 1);
    gpio_set_level(NET_LED_B, 1);
    gpio_set_level(NET_DEBUG_1, 0);
    gpio_set_level(NET_DEBUG_2, 0);
    gpio_set_level(NET_DEBUG_3, 0);

    int next = 0;
    gpio_set_level(NET_LED_R, 0);

    // Ready for normal operations
    gpio_set_level(NET_STAT, 1);

    // This is the lazy way of doing it, otherwise we should use a esp_timer.
    uint32_t blink_cnt = 0;
    while (1)
    {
        gpio_set_level(NET_LED_R, next);
        next = next ? 0 : 1;

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

static void SetupPins()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = NET_STAT_MASK;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(NET_STAT, 0);

    // LED init
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = LED_MASK;
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);

    // Debug init
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = NET_DEBUG_MASK;
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);

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

    // uart_intr_config_t uart1_intr = {
    //     .intr_enable_mask
    // }

    // Setup serial interface for ui
    esp_err_t res;
    res = uart_driver_install(UART1, Rx_Buff_Size, Tx_Buff_Size, Event_Queue_Size, &uart_queue, ESP_INTR_FLAG_LOWMED);
    printf("install res=%d\n", res);

    res = uart_set_pin(UART1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    printf("uart set pin res=%d\n", res);

    res = uart_param_config(UART1, &uart1_config);
    printf("install res=%d\n", res);

    ui_uart1 = new SerialEsp(UART1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, uart1_config, 4096 * 2);

    Logger::Log(Logger::Level::Info, "Pin setup complete");
}

void SetupComponents()
{
    // UART to the ui
    ui_layer = new SerialPacketManager(ui_uart1);

    xTaskCreate(SerialReadTask, "serial_read_task", 4096, NULL, 1, NULL);


    // Once ui is ready then we can start the serial write task
    xTaskCreate(SerialWriteTask, "serial_write_task", 4096, NULL, 1, NULL);
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
