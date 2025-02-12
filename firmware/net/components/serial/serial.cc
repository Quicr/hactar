#include "serial.hh"

#include "esp_log.h"
#include <random>

Serial::Serial(const uart_port_t port, uart_dev_t& uart, const periph_interrput_t intr_source,
    const uart_config_t uart_config, const int tx_pin, const int rx_pin,
    const int rts_pin, const int cts_pin,
    uint8_t& tx_buff, const uint32_t tx_buff_sz,
    uint8_t& rx_buff, const uint32_t rx_buff_sz,
    const uint32_t rx_rings):
    SerialHandler(rx_rings, tx_buff, tx_buff_sz, rx_buff, rx_buff_sz, Transmit, this),
    port(port),
    uart(uart),
    update_cache(0),
    unread_mux(xSemaphoreCreateBinary()),
    unread_cache(xSemaphoreCreateCounting(0xFFFF, 0))
{
    xSemaphoreGive(unread_mux);
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port, tx_pin, rx_pin, rts_pin, cts_pin));

    ESP_ERROR_CHECK(esp_intr_alloc(intr_source, 2, ISRHandler, (void*)this, &isr_handle));

    // Enable interrupt for full fifo, and rx timeout.
    // Enable interrupt for tx transmit complete

    uart_intr_config_t uintr_cfg = {
      .intr_enable_mask = (UART_RXFIFO_FULL_INT_ENA_M | UART_RXFIFO_TOUT_INT_ENA_M | UART_TX_DONE_INT_ENA_M),
      .rx_timeout_thresh = 1,
      .rxfifo_full_thresh = 68,
    };
    ESP_ERROR_CHECK(uart_intr_config(port, &uintr_cfg));
}

Serial::~Serial()
{

}

void Serial::UpdateUnread(const uint16_t update)
{
    if (xSemaphoreTake(unread_mux, 0))
    {
        unread -= (update + update_cache);
        update_cache = 0;

        while (xSemaphoreTake(unread_cache, 0))
        {
            unread += 1;
        }

        xSemaphoreGive(unread_mux);
    }
    else
    {
        update_cache += update;
        printf("Couldn't take semaphore\n");
    }

}

// NOTE TO YE TO WHOM MAY COME TO CHANGE THIS FUNCTION, DO NOT PUT
// LOGGING INTO THIS FUNCTION, IT WILL CAUSE AN AUTOMATIC CRASH AND NOT
// TELL YOU WHY
void Serial::Transmit(void* arg)
{
    // TODO semaphores
    Serial* serial = static_cast<Serial*>(arg);

    // Greater than our fifo has room for clamp it.
    if (serial->num_to_send > (128 - serial->uart.status.txfifo_cnt))
    {
        serial->num_to_send = 128 - serial->uart.status.txfifo_cnt;
    }
    uart_ll_write_txfifo(&serial->uart, serial->tx_buff + serial->tx_read_idx, serial->num_to_send);
}

// NOTE TO YE TO WHOM MAY COME TO CHANGE THIS FUNCTION, DO NOT PUT
// LOGGING INTO THIS FUNCTION, IT WILL CAUSE AN AUTOMATIC CRASH AND NOT
// TELL YOU WHY
void Serial::ISRHandler(void* args)
{
    Serial* serial = (Serial*)args;

    uint32_t intr_status = 0;
    while (1)
    {
        intr_status = uart_ll_get_intsts_mask(&serial->uart);

        if (intr_status == 0)
        {
            break;
        }

        // Parity error intr
        if (intr_status & UART_INTR_PARITY_ERR)
        {
            uart_ll_clr_intsts_mask(&serial->uart, UART_PARITY_ERR_INT_CLR_M);
            abort();
            continue;
        }
        else if (intr_status & UART_INTR_TX_DONE)
        {
            // Advance the read head
            serial->UpdateTx();

            uart_ll_clr_intsts_mask(&serial->uart, UART_TX_DONE_INT_CLR_M);
            continue;
        }
        else if (intr_status & UART_INTR_RXFIFO_TOUT || intr_status & UART_INTR_RXFIFO_FULL)
        {
            Serial::RxHandler(serial);
            uart_ll_clr_intsts_mask(&serial->uart, UART_RXFIFO_FULL_INT_CLR_M | UART_RXFIFO_TOUT_INT_CLR_M);
            continue;
        }
    }
}

void Serial::RxHandler(Serial* serial)
{
    // Loop until we've emptied the buff if a small buffer has been
    // designated then this will cover overflowing.
    if (serial->uart.status.rxfifo_cnt)
    {
        // Note- Reading from uart.fifo.rxfifo_rd_byte automatically
        // decrements uart.status.rxfifo_cnt
        uint32_t num_bytes = serial->uart.status.rxfifo_cnt;
        if (num_bytes + serial->rx_write_idx > serial->rx_buff_sz)
        {
            num_bytes = serial->rx_buff_sz - serial->rx_write_idx;
        }

        // Pull the data out of the register
        for (uint32_t i = 0; i < num_bytes; ++i)
        {
            serial->rx_buff[serial->rx_write_idx++] = serial->uart.fifo.rxfifo_rd_byte;
        }

        if (serial->rx_write_idx >= serial->rx_buff_sz)
        {
            serial->rx_write_idx = 0;
        }

        // Take the semaphore
        BaseType_t higher_priority = pdFALSE;
        if (xSemaphoreTakeFromISR(serial->unread_mux, &higher_priority))
        {
            serial->unread += num_bytes;
            xSemaphoreGiveFromISR(serial->unread_mux, &higher_priority);
        }
        else
        {
            // Couldn't get the semaphore, so lets just put this in a different
            // semaphore that counts the number of cached bytes
            // that we can pull from later
            for (uint32_t i = 0 ;i < num_bytes; ++i)
            {
                xSemaphoreGiveFromISR(serial->unread_cache, NULL);
            }
            portYIELD_FROM_ISR(higher_priority);
        }
    }
}

