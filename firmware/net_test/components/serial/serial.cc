#include "serial.hh"
#include "packet_builder.hh"

#include "esp_log.h"
#include <random>

Serial::Serial(const uart_port_t port, uart_dev_t& uart, const periph_interrput_t intr_source,
    const uart_config_t uart_config, const int tx_pin, const int rx_pin,
    const int rts_pin, const int cts_pin,
    const uint32_t rx_isr_buff_sz, const uint32_t tx_isr_buff_sz,
    const uint32_t tx_rings, const uint32_t rx_rings):
    port(port),
    uart(uart),
    rx_isr_buff_sz(rx_isr_buff_sz),
    rx_isr_buff(new uint8_t[rx_isr_buff_sz]{ 0 }),
    tx_isr_buff_sz(tx_isr_buff_sz),
    tx_isr_buff(new uint8_t[tx_isr_buff_sz]{ 0 }),
    tx_packets(tx_rings),
    rx_packets(rx_rings),
    rx_isr_buff_write(0),
    rx_isr_buff_read(0),
    tx_isr_buff_write(0),
    tx_isr_buff_read(0),
    tx_unwritten(0),
    tx_free(true),
    num_to_write(0)
{
    // Start the tasks
    // xTaskCreate(ReadTask, "serial_read_task", rx_task_sz, this, 1, NULL);
    // xTaskCreate(WriteTask, "serial_write_task", tx_task_sz, this, 1, NULL);


    // TODO add pins
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port, tx_pin, rx_pin, rts_pin, cts_pin));

    ESP_ERROR_CHECK(esp_intr_alloc(intr_source, NULL, ISRHandler, (void*)this, &isr_handle));

    // Enable interrupt for full fifo, and rx timeout.
    // Enable interrupt for tx transmit complete
    uart_intr_config_t uintr_cfg = {
      .intr_enable_mask = (UART_RXFIFO_FULL_INT_ENA_M | UART_RXFIFO_TOUT_INT_CLR | UART_TX_DONE_INT_ENA_M),
      .rx_timeout_thresh = 1,
      .txfifo_empty_intr_thresh = 0,
      .rxfifo_full_thresh = 68,
    };
    ESP_ERROR_CHECK(uart_intr_config(port, &uintr_cfg));
}

Serial::~Serial()
{
    delete [] rx_isr_buff;
    delete [] tx_isr_buff;
}

link_packet_t* Serial::Read()
{
    if (!rx_packets.Peek().is_ready)
    {
        return nullptr;
    }

    link_packet_t* packet = &rx_packets.Read();
    packet->is_ready = false;
    return packet;
}


link_packet_t* Serial::Write()
{
    return &tx_packets.Write();
}

void Serial::TestWrite()
{
    // Write





    for (int i = 0 ; i < tx_isr_buff_sz; ++i)
    {
        tx_isr_buff[tx_isr_buff_write] = (uint8_t)(rand() % 255);
        tx_isr_buff_write++;
        tx_unwritten++;
        if (tx_isr_buff_write >= tx_isr_buff_sz)
        {
            tx_isr_buff_write = 0;
        }
    }
    Transmit(this);
}

#if 0
void Serial::WriteTask(void* param)
{

}
#else

// Loopback version
void Serial::WriteTask(void* param)
{

}
#endif
void Serial::ReadTask(void* param)
{

}

// NOTE TO YE TO WHOM MAY COME TO CHANGE THIS FUNCTION, DO NOT PUT 
// LOGGING INTO THIS FUNCTION, IT WILL CAUSE AN AUTOMATIC CRASH AND NOT 
// TELL YOU WHY
bool Serial::Transmit(Serial* self)
{
    if (!self->tx_free)
    {
        return false;
    }
    self->tx_free = false;

    // Get the number of bytes that are in the read buff
    self->num_to_write = self->tx_unwritten;

    if (self->num_to_write == 0)
    {
        return false;
    }

    if (self->num_to_write > self->tx_isr_buff_sz - self->tx_isr_buff_read) 
    {
        self->num_to_write = self->tx_isr_buff_sz - self->tx_isr_buff_read;
    }

    // Greater than our fifo has room for clamp it.
    if (self->num_to_write > (128 - self->uart.status.txfifo_cnt))
    {
        self->num_to_write = 128 - self->uart.status.txfifo_cnt;
    }
    uart_ll_write_txfifo(&self->uart, self->tx_isr_buff + self->tx_isr_buff_read, self->num_to_write);
    return true;
}

// NOTE TO YE TO WHOM MAY COME TO CHANGE THIS FUNCTION, DO NOT PUT 
// LOGGING INTO THIS FUNCTION, IT WILL CAUSE AN AUTOMATIC CRASH AND NOT 
// TELL YOU WHY
void Serial::ISRHandler(void* args)
{
    Serial* self = (Serial*)args;

    // Parity error intr
    if (self->uart.int_st.parity_err_int_st)
    {
        uart_clear_intr_status(self->port, UART_PARITY_ERR_INT_CLR);
        abort();
    }
    else if (self->uart.int_st.tx_done_int_st)
    {
        // Transmit done intr
        uart_clear_intr_status(self->port, UART_TX_DONE_INT_CLR);

        self->tx_isr_buff_read += self->num_to_write;
        self->tx_unwritten -= self->num_to_write;

        if (self->tx_isr_buff_read >= self->tx_isr_buff_sz)
        {
            self->tx_isr_buff_read = 0;
        }
        self->tx_free = true;

        // Try to transmit more we have more to transmit
        Serial::Transmit(self);
    }
    else if (self->uart.status.rxfifo_cnt)
    {
        ++self->num_rx_intr;
        // Loop until we've emptied the buff if a small buffer has been
        // designated then this will cover overflowing.
        while (self->uart.status.rxfifo_cnt)
        {
            while (self->uart.status.rxfifo_cnt && self->rx_isr_buff_write < self->rx_isr_buff_sz)
            {
                self->num_rx_recv++;
                self->rx_isr_buff[self->rx_isr_buff_write++] = self->uart.fifo.rxfifo_rd_byte;
            }

            if (self->rx_isr_buff_write >= self->rx_isr_buff_sz)
            {
                self->rx_isr_buff_write = 0;
            }
        }
        uart_clear_intr_status(self->port, UART_RXFIFO_FULL_INT_CLR);
        uart_clear_intr_status(self->port, UART_RXFIFO_TOUT_INT_CLR);
    }
}