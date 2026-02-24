#pragma once

#include "driver/spi_slave.h"
#include <cstdint>
#include <cstring>

class SPISlave
{
public:
    SPISlave(spi_host_device_t host,
             int mosi,
             int miso,
             int sclk,
             int cs,
             int dma_chan = SPI_DMA_CH_AUTO);
    ~SPISlave();
    void start(size_t buf_size = 64);
    void stop();

private:
    spi_host_device_t host_;
    int mosi_, miso_, sclk_, cs_;
    int dma_chan_;
    size_t buf_size_;
    uint8_t* rx_buf_;
    uint8_t* tx_buf_;
    TaskHandle_t task_handle_;
    bool running_;

    static void task(void* arg);
    void run();
};
