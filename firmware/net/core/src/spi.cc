// spi_slave.cpp
#include "spi.hh"
#include "driver/spi_slave.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

static const char* TAG = "SPISlave";

SPISlave::SPISlave(spi_host_device_t host, int mosi, int miso, int sclk, int cs, int dma_chan) :
    host_(host),
    mosi_(mosi),
    miso_(miso),
    sclk_(sclk),
    cs_(cs),
    dma_chan_(dma_chan),
    buf_size_(0),
    rx_buf_(nullptr),
    tx_buf_(nullptr),
    task_handle_(nullptr),
    running_(false)
{
}

SPISlave::~SPISlave()
{
    stop();
}

void SPISlave::start(size_t buf_size)
{
    buf_size_ = buf_size;

    // Allocate DMA-capable buffers
    rx_buf_ = (uint8_t*)heap_caps_malloc(buf_size_, MALLOC_CAP_DMA);
    tx_buf_ = (uint8_t*)heap_caps_malloc(buf_size_, MALLOC_CAP_DMA);
    memset(tx_buf_, 0x00, buf_size_); // TX always zero

    // Configure bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = mosi_,
        .miso_io_num = miso_,
        .sclk_io_num = sclk_,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (int)buf_size_,
    };

    // Configure slave interface
    spi_slave_interface_config_t slave_cfg = {
        .spics_io_num = cs_,
        .flags = 0,
        .queue_size = 4,
        .mode = 0, // CPOL=0, CPHA=0 — change as needed
        .post_setup_cb = nullptr,
        .post_trans_cb = nullptr,
    };

    ESP_ERROR_CHECK(spi_slave_initialize(host_, &bus_cfg, &slave_cfg, dma_chan_));

    running_ = true;
    xTaskCreate(task, "spi_slave_task", 4096, this, 5, &task_handle_);
}

void SPISlave::stop()
{
    running_ = false;
    if (task_handle_)
    {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
    spi_slave_free(host_);
    if (rx_buf_)
    {
        heap_caps_free(rx_buf_);
        rx_buf_ = nullptr;
    }
    if (tx_buf_)
    {
        heap_caps_free(tx_buf_);
        tx_buf_ = nullptr;
    }
}

void SPISlave::task(void* arg)
{
    static_cast<SPISlave*>(arg)->run();
}

void SPISlave::run()
{
    spi_slave_transaction_t trans = {};

    while (running_)
    {
        memset(rx_buf_, 0, buf_size_);

        trans.length = buf_size_ * 8; // length in bits
        trans.tx_buffer = tx_buf_;    // all zeros
        trans.rx_buffer = rx_buf_;

        esp_err_t ret = spi_slave_transmit(host_, &trans, portMAX_DELAY);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "SPI transmit error: %s", esp_err_to_name(ret));
            continue;
        }

        size_t received_bytes = trans.trans_len / 8;
        ESP_LOGI(TAG, "Received %zu bytes:", received_bytes);
        ESP_LOG_BUFFER_HEX(TAG, rx_buf_, received_bytes);
    }

    vTaskDelete(nullptr);
}
