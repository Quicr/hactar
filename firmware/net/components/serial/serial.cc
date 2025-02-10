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
    uart(uart)
{
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port, tx_pin, rx_pin, rts_pin, cts_pin));

    ESP_ERROR_CHECK(esp_intr_alloc(intr_source, NULL, ISRHandler, (void*)this, &isr_handle));

    // Enable interrupt for full fifo, and rx timeout.
    // Enable interrupt for tx transmit complete
    uart_intr_config_t uintr_cfg = {
      .intr_enable_mask = (UART_RXFIFO_FULL_INT_ENA_M | UART_RXFIFO_TOUT_INT_CLR | UART_TX_DONE_INT_ENA_M),
      .rx_timeout_thresh = 1,
      .rxfifo_full_thresh = 60,
    };
    ESP_ERROR_CHECK(uart_intr_config(port, &uintr_cfg));
}

Serial::~Serial()
{

}

// NOTE TO YE TO WHOM MAY COME TO CHANGE THIS FUNCTION, DO NOT PUT
// LOGGING INTO THIS FUNCTION, IT WILL CAUSE AN AUTOMATIC CRASH AND NOT
// TELL YOU WHY
void Serial::Transmit(void* arg)
{
    Serial* self = static_cast<Serial*>(arg);

    // Greater than our fifo has room for clamp it.
    if (self->num_to_send > (128 - self->uart.status.txfifo_cnt))
    {
        self->num_to_send = 128 - self->uart.status.txfifo_cnt;
    }
    uart_ll_write_txfifo(&self->uart, self->tx_buff + self->tx_read_idx, self->num_to_send);
}

// NOTE TO YE TO WHOM MAY COME TO CHANGE THIS FUNCTION, DO NOT PUT
// LOGGING INTO THIS FUNCTION, IT WILL CAUSE AN AUTOMATIC CRASH AND NOT
// TELL YOU WHY
void Serial::ISRHandler(void* args)
{
    Serial* self = (Serial*)args;

    uint32_t intr_status = 0;
    while (1)
    {
        intr_status = uart_ll_get_intsts_mask(&self->uart);

        if (intr_status == 0)
        {
            break;
        }

        // Parity error intr
        if (intr_status & UART_INTR_PARITY_ERR)
        {
            uart_ll_clr_intsts_mask(&self->uart, UART_PARITY_ERR_INT_CLR_M);
            abort();
            continue;
        }
        else if (intr_status & UART_INTR_TX_DONE)
        {
            uart_ll_clr_intsts_mask(&self->uart, UART_TX_DONE_INT_CLR_M);

            // Advance the read head
            self->UpdateTx();
            continue;
        }
        else if (intr_status & UART_INTR_RXFIFO_TOUT)
        {
            uart_ll_clr_intsts_mask(&self->uart, UART_RXFIFO_TOUT_INT_CLR_M);
            Serial::RxHandler(self);
            continue;
        }
        else if (intr_status & UART_INTR_RXFIFO_FULL)
        {
            uart_ll_clr_intsts_mask(&self->uart, UART_RXFIFO_FULL_INT_CLR_M);
            Serial::RxHandler(self);
            continue;
        }
    }
}

void Serial::RxHandler(Serial* self)
{
    // Loop until we've emptied the buff if a small buffer has been
    // designated then this will cover overflowing.
    while (self->uart.status.rxfifo_cnt)
    {
        // Note- Reading from uart.fifo.rxfifo_rd_byte automatically
        // decrements uart.status.rxfifo_cnt
        uint32_t bytes_to_read = self->uart.status.rxfifo_cnt;
        if (bytes_to_read + self->rx_write_idx > self->rx_buff_sz)
        {
            bytes_to_read = self->rx_buff_sz - self->rx_write_idx;
        }

        const uint32_t end = self->rx_write_idx + bytes_to_read;
        for (uint32_t i = self->rx_write_idx; i < end; ++i)
        {
            self->rx_buff[i] = self->uart.fifo.rxfifo_rd_byte;
        }

        self->UpdateRx(bytes_to_read);
    }
}

