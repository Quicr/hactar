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

bool ui_chip_ready = false;

const size_t audio_sz = 355;
uint8_t tx_buff[TX_BUFF_SIZE] = { 0 };
size_t tx_write_mod = 0;
size_t tx_read_mod = 0;
uint8_t rx_buff[RX_BUFF_SIZE] = { 0 };


bool tx_data_ready = false;
uint32_t num_to_send = 0;

uint32_t num_write = 0;
uint32_t num_recv = 0;
bool synced = true;

QueueHandle_t uart_queue;

static void SerialWriteTask(void* param)
{
    int next = 0;
    while (1)
    {
        if (tx_data_ready)
        {
            uint16_t offset = tx_read_mod * TX_BUFF_SIZE_2;
            uart_write_bytes(UART1, tx_buff + offset, num_to_send);
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
            vTaskDelay(10 / portTICK_PERIOD_MS);
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
                        packet_len = static_cast<uint16_t>(data[idx + 0]) << 8 | data[idx + 1];

                        // Get the type
                        packet_type = data[idx + 2];
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

                                int tx_write_offset = tx_write_mod * TX_BUFF_SIZE_2;

                                // Audio copy to the tx buffer
                                for (int i = 0; i < audio_sz; ++i)
                                {
                                    tx_buff[tx_write_offset + i] = rx_buff[i];
                                }

                                tx_write_mod = !tx_write_mod;
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
                uart_flush_input(UART1); // Flush the input to recover
                xQueueReset(uart_queue);   // Reset the event queue
                break;
            }
            case UART_FRAME_ERR: // Frame error
            {

                printf("UART Frame Error!\n");
                uart_flush_input(UART1); // Flush the input to recover
                xQueueReset(uart_queue);   // Reset the event queue
                break;
            }
            default:
            {

                printf("Unhandled UART event type: %d\n", event.type);
                uart_flush_input(UART1); // Flush the input to recover
                xQueueReset(uart_queue);   // Reset the event queue
                break;
            }

        }
    }
}

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
