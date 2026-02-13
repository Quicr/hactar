#include "spi.hh"
#include "link_packet_t.hh"
#include "stm32f4xx_hal_spi.h"
#include <cstring>

SPI::SPI(SPI_HandleTypeDef& hspi,
         uint8_t* tx_buff,
         uint16_t tx_buff_sz,
         uint8_t* rx_buff,
         uint16_t rx_buff_sz) :
    hspi(hspi),
    tx_ring(tx_buff, tx_buff_sz),
    rx_ring(rx_buff, rx_buff_sz),
    tx_buff(tx_buff),
    tx_buff_sz(tx_buff_sz),
    rx_buff(rx_buff),
    rx_buff_sz(rx_buff_sz),
    tx_write_idx(0),
    tx_read_idx(0),
    rx_write_idx(0),
    rx_read_idx(0),
    // transaction_len(0),
    rx_packets()
{
}

void SPI::Init()
{
    HAL_SPI_Init(&hspi);
}

void SPI::Deinit()
{
    HAL_SPI_DeInit(&hspi);
}

void SPI::Transmit(const uint8_t* data, const uint16_t len)
{
    tx_ring.Write(data, len);
    untransmitted += len;

    if (state == State::Idle)
    {
        Transaction();
    }
}

void SPI::Transmit(const link_packet_t& packet)
{
    Transmit(packet.data, packet.length + link_packet_t::Header_Size);
}

const link_packet_t* SPI::Receive()
{
    uint16_t total_bytes_read = 0;
    uint8_t byte = 0;

    while (total_bytes_read < unread)
    {
    }
}

void SPI::Transaction()
{

    if (untransmitted == 0)
    {
        return;
    }

    if (untransmitted > Linear_Buff_Size)
    {
        outgoing_len = Linear_Buff_Size;
    }
    else
    {
        outgoing_len = untransmitted;
    }

    tx_ring.Read(linear_tx_buff, outgoing_len);

    // Transmit the length
    state = State::Start;
    HAL_SPI_TransmitReceive_DMA(&hspi, (const uint8_t*)&outgoing_len, (uint8_t*)&incoming_len,
                                sizeof(outgoing_len));
}

void SPI::TransactionCallback(SPI* spi)
{
    switch (state)
    {
    case State::Start:
    {
        state = State::Transacting;
        HAL_SPI_TransmitReceive_DMA(&hspi, linear_tx_buff, linear_rx_buff, outgoing_len);
        break;
    }
    case State::Transacting:
    {
        state = State::Idle;
        spi->untransmitted -= outgoing_len;
        spi->unread += incoming_len;

        // Copy the data out of the rx buff

        spi->Transaction();
        break;
    }
    default:
    {
        break;
    }
    }
    // Check the first 5 bytes of the next expected packet, if all zeroes, then ignore it and move
    // on otherwise, count the length
    // skip the type, we just need the size
    // uint16_t len = spi.rx_buff[spi.rx_write_idx + 1];
    // len += spi.rx_buff[spi.rx_write_idx + 2] << 8;
    // len += spi.rx_buff[spi.rx_write_idx + 3] << 16;
    // len += spi.rx_buff[spi.rx_write_idx + 4] << 24;
    //
    // if (len > 0)
    // {
    //     spi.unread += len;
    // }
    //
    // // Since the ui chip is the master we can assume our transaction_len is the amount we
    // // transmitted
    // spi.tx_write_idx += spi.transaction_len;
    // spi.rx_write_idx += spi.transaction_len;
    //
    // if (spi.tx_write_idx >= spi.tx_buff_sz)
    // {
    //     spi.tx_write_idx = 0;
    // }
    //
    // if (spi.rx_write_idx >= spi.rx_buff_sz)
    // {
    //     spi.rx_write_idx = 0;
    // }
}
