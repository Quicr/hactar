#pragma once

#include "app_main.hh"

#include "ring_buffer.hh"
#include "link_packet_t.hh"

#include "serial_handler/serial_handler.hh"

template<size_t rx_buffer_packets>
class Serial : public SerialHandler<rx_buffer_packets>
{
public:

    Serial(UART_HandleTypeDef* uart,
        uint8_t& tx_buff, const uint32_t tx_buff_sz,
        uint8_t& rx_buff, const uint32_t rx_buff_sz)
      : SerialHandler<rx_buffer_packets>(tx_buff, tx_buff_sz, rx_buff, rx_buff_sz, Transmit, this)
      , uart(uart)
    {}


    void StartReceive() {
        HAL_UARTEx_ReceiveToIdle_DMA(uart, this->rx_buff, this->rx_buff_sz);
    }

    void Reset() {
        this->rx_write_idx = 0;
        this->rx_read_idx = 0;

        link_packet_t* packets = this->rx_packets.Buffer();
        for (int i = 0 ; i < this->rx_packets.Size(); ++i)
        {
            packets[i].is_ready = false;
        }

        this->packet = nullptr;
        this->bytes_read = 0;

        StartReceive();
    }

    void Stop() {
        HAL_UART_AbortReceive_IT(uart);
    }

    void ResetRecv() {
        uint16_t err = uart->Instance->SR;
        Stop();
        UNUSED(err);

        Reset();
    }

    const UART_HandleTypeDef* UART() const {
      return uart;
    }

    void RxISR(const uint16_t idx) {
        // NOTE- Do NOT put a breakpoint in here, the callbacks will
        // get blocked will get memory access errors because the
        // callback on your breakpoint will throw off the values and then
        // will cause the value to be at the wrong idx and everything will go
        // ka-boom, crash, plop. Overflow errors and stuff.
        const uint16_t num_recv = idx - this->rx_write_idx;
        // UpdateRx(num_recv);
        this->unread += num_recv;
        this->rx_write_idx += num_recv;
        if (this->rx_write_idx >= this->rx_buff_sz)
        {
            this->rx_write_idx = 0;
        }

        if (this->unread > this->rx_buff_sz)
        {
            // TODO
            // Error("SerialStm rx isr", "Overflowed rx buffer");
        }
    }

    void TxISR() {
        this->UpdateTx();
    }

protected:
    void UpdateUnread(const uint16_t update) override {
        // UI_LOG_INFO("this->unread %d, update %d", (int)this->unread, (int)update);
        __disable_irq();
        this->unread -= update;
        __enable_irq();
        // UI_LOG_INFO("this->unread %d", (int)this->unread);
    }

    static void Transmit(void* arg) {
        Serial* self = static_cast<Serial*>(arg);
        HAL_UART_Transmit_DMA(self->uart, self->tx_buff + self->tx_read_idx, self->num_to_send);
    }

    UART_HandleTypeDef* uart;
};
