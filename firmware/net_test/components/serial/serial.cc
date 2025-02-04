#include "serial.hh"
#include "packet_builder.hh"

#include "esp_log.h"
#include <random>

Serial::Serial(const uart_port_t port, uart_dev_t& uart, const periph_interrput_t intr_source,
    const uart_config_t uart_config, const int tx_pin, const int rx_pin,
    const int rts_pin, const int cts_pin,
    const uint32_t rx_buff_sz, const uint32_t tx_buff_sz,
    const uint32_t tx_rings, const uint32_t rx_rings):
    port(port),
    uart(uart),
    rx_buff_sz(rx_buff_sz),
    rx_buff(new uint8_t[rx_buff_sz]{ 0 }),
    tx_buff_sz(tx_buff_sz),
    tx_buff(new uint8_t[tx_buff_sz]{ 0 }),
    tx_packets(tx_rings),
    rx_packets(rx_rings),
    rx_buff_write(0),
    rx_buff_read(0),
    unread(0),
    tx_buff_write(0),
    tx_buff_read(0),
    untransmitted(0),
    num_transmitting(0),
    tx_free(true)
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
    delete [] rx_buff;
    delete [] tx_buff;
}

link_packet_t* Serial::Read()
{
    while (unread > 0)
    {
        uint16_t bytes_to_read = unread;

        if (rx_buff_read + bytes_to_read > rx_buff_sz)
        {
            bytes_to_read = rx_buff_sz - rx_buff_read;
        }

        // Builds packet and sets is_ready to true when a packet is done.
        BuildPacket(rx_buff + rx_buff_read, bytes_to_read, rx_packets);
        unread -= bytes_to_read;
        rx_buff_read += bytes_to_read;

        if (rx_buff_read >= rx_buff_sz)
        {
            rx_buff_read = 0;
        }

    }

    if (!rx_packets.Peek().is_ready)
    {
        return nullptr;
    }

    link_packet_t* packet = &rx_packets.Read();
    packet->is_ready = false;
    return packet;
}

void Serial::Write(const link_packet_t* packet)
{
    uint16_t total_bytes = packet->length + link_packet_t::Header_Size;
    Write(packet->data, total_bytes);
}

void Serial::Write(const uint8_t* data, const size_t size)
{
    uint16_t total_bytes = size;
    // Logger::Log(Logger::Level::Info, "packet len", packet->length);
    size_t data_offset = 0;

    if (tx_buff_write + total_bytes > tx_buff_sz)
    {
        const uint16_t diff = tx_buff_sz - tx_buff_write;
        memcpy(tx_buff + tx_buff_write, data, diff);

        // Reset the buff write head since we are at the end.
        tx_buff_write = 0;

        // Minus off the amount of bytes copied
        total_bytes -= diff;

        // Set the offset for the packet data
        data_offset = diff;
    }

    // Copy the data into the tx_buff
    memcpy(tx_buff + tx_buff_write, data + data_offset, total_bytes);

    tx_buff_write += total_bytes;

    // Update the number of bytes to write by how many total will be
    // sent by this packet. Needs to be at AFTER the data is copied
    untransmitted += size;

    if (!tx_free)
    {
        return;
    }

    Serial::Transmit(this);
}

// NOTE TO YE TO WHOM MAY COME TO CHANGE THIS FUNCTION, DO NOT PUT
// LOGGING INTO THIS FUNCTION, IT WILL CAUSE AN AUTOMATIC CRASH AND NOT
// TELL YOU WHY
void Serial::Transmit(Serial* self)
{
    // Get the number of bytes that are in the read buff
    self->num_transmitting = self->untransmitted;

    if (self->num_transmitting == 0)
    {
        self->tx_free = true;
        return;
    }
    self->tx_free = false;

    // Ensure that we don't access illegal memory > tx_buff_sz
    if (self->num_transmitting > self->tx_buff_sz - self->tx_buff_read)
    {
        self->num_transmitting = self->tx_buff_sz - self->tx_buff_read;
    }

    // Greater than our fifo has room for clamp it.
    if (self->num_transmitting > (128 - self->uart.status.txfifo_cnt))
    {
        self->num_transmitting = 128 - self->uart.status.txfifo_cnt;
    }
    uart_ll_write_txfifo(&self->uart, self->tx_buff + self->tx_buff_read, self->num_transmitting);
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

            // Advance the read head
            self->tx_buff_read += self->num_transmitting;
            self->untransmitted -= self->num_transmitting;

            if (self->tx_buff_read >= self->tx_buff_sz)
            {
                self->tx_buff_read = 0;
            }

            uart_ll_clr_intsts_mask(&self->uart, UART_TX_DONE_INT_CLR_M);
            Serial::Transmit(self);
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
        if (bytes_to_read + self->rx_buff_write > self->rx_buff_sz)
        {
            bytes_to_read = self->rx_buff_sz - self->rx_buff_write;
        }
        self->unread += bytes_to_read;

        while (bytes_to_read)
        {
            self->rx_buff[self->rx_buff_write++] = self->uart.fifo.rxfifo_rd_byte;
            --bytes_to_read;
        }

        if (self->rx_buff_write >= self->rx_buff_sz)
        {
            self->rx_buff_write = 0;
        }
    }
}

