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
    tx_buff(tx_buff),
    tx_buff_sz(tx_buff_sz),
    rx_buff(rx_buff),
    rx_buff_sz(rx_buff_sz),
    tx_read(0),
    tx_write(0),
    rx_read(0),
    rx_write(0),
    tr_ring(transactions, Num_Rx_Transactions),
    state(State::Idle),
    unread(0),
    outgoing_len(0),
    incoming_len(0),
    transaction_len(0),
    rx_packets(_link_packet_buff, Rx_Packet_Sz),
    packet(&rx_packets.Write()),
    bytes_read(0)
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

    while (tr_ring.Unread())
    {
        transaction_t& transaction = tr_ring.Read();
        total_bytes_read = 0;
        while (total_bytes_read < transaction.incoming_len)
        {
            if (bytes_read < link_packet_t::Header_Size)
            {
                packet->data[bytes_read++] = rx_ring.Read();
                ++total_bytes_read;
                continue;
            }

            if (bytes_read >= packet->length + link_packet_t::Header_Size)
            {
                bytes_read = 0;
                packet->is_ready = true;
                packet = &rx_packets.Write();
            }
            else if (bytes_read >= link_packet_t::Packet_Size)
            {
                Logger::Log(Logger::Level::Info, "TLV frame size error");

                packet->is_ready = false;
                bytes_read = 0;
            }
            else
            {
                const size_t to_copy = packet->length < transaction.incoming_len - total_bytes_read
                                         ? packet->length
                                         : transaction.incoming_len - total_bytes_read;
                rx_ring.Read(packet->data + bytes_read, to_copy);
                total_bytes_read += to_copy;
            }
        }

        // Move the read head the distance to the write head, according to our transaction
        rx_ring.MoveReadHead(transaction.incoming_len - transaction.outgoing_len);
    }
}

void SPI::Transaction()
{
    if (tx_ring.Unread() == 0)
    {
        return;
    }
    state = State::Start;

    out_slice = tx_ring.Read(outgoing_len);
    in_slice = rx_ring.Write(outgoing_len);

    // Transmit the length
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
        HAL_SPI_TransmitReceive_DMA(&hspi, spi->out_slice, spi->in_slice, spi->outgoing_len);
        break;
    }
    case State::Transacting:
    {
        state = State::Idle;

        transaction_t& transaction = spi->tr_ring.Write();
        transaction.outgoing_len = spi->outgoing_len;
        transaction.incoming_len = spi->incoming_len;
        transaction.rx_read_idx = spi->rx_ring.ReadIdx();
        transaction.rx_write_idx = spi->rx_ring.WriteIdx();

        spi->unread += spi->incoming_len;

        spi->Transaction();
        break;
    }
    default:
    {
        break;
    }
    }
}
