#include "spi.hh"
#include "link_packet_t.hh"
#include "stm32f4xx_hal_dma.h"
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
    out_slice(nullptr),
    in_slice(nullptr),
    transactions(),
    tr_ring(transactions, Num_Rx_Transactions),
    state(State::Idle),
    outgoing_len(0),
    incoming_len(0),
    _link_packet_buff(),
    rx_packets(_link_packet_buff, Rx_Packet_Sz)
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
    static uint16_t packet_bytes_read = 0;
    static link_packet_t* packet = &rx_packets.Write();

    uint16_t total_packet_bytes_read = 0;

    while (tr_ring.Unread())
    {
        transaction_t& transaction = tr_ring.Read();
        total_packet_bytes_read = 0;
        while (total_packet_bytes_read < transaction.incoming_len)
        {
            if (packet_bytes_read < link_packet_t::Header_Size)
            {
                packet->data[packet_bytes_read++] = rx_ring.Read();
                ++total_packet_bytes_read;
                continue;
            }

            if (packet_bytes_read >= packet->length + link_packet_t::Header_Size)
            {
                packet_bytes_read = 0;
                packet->is_ready = true;
                packet = &rx_packets.Write();
            }
            else if (packet_bytes_read >= link_packet_t::Packet_Size)
            {
                Logger::Log(Logger::Level::Info, "TLV frame size error");

                packet->is_ready = false;
                packet_bytes_read = 0;
            }
            else
            {
                const size_t to_copy =
                    packet->length < transaction.incoming_len - total_packet_bytes_read
                        ? packet->length
                        : transaction.incoming_len - total_packet_bytes_read;
                rx_ring.Read(packet->data + packet_bytes_read, to_copy);
                total_packet_bytes_read += to_copy;
            }
        }

        // Move the read head the distance to the write head, according to our transaction
        rx_ring.MoveReadHead(transaction.incoming_len - transaction.outgoing_len);
    }

    return GetReadyPacket();
}

const link_packet_t* SPI::GetReadyPacket()
{
    if (!rx_packets.Peek().is_ready)
    {
        return nullptr;
    }

    link_packet_t* p = &rx_packets.Read();
    p->is_ready = false;
    return p;
}

void SPI::Transaction()
{
    if (tx_ring.Unread() == 0)
    {
        return;
    }

    state = State::Start;

    // I really don't like this, ring buffer should be changed to use size_t or at least fix write
    // to use something else
    uint16_t tmp_outgoing_len = tx_ring.Unread();

    out_slice = tx_ring.Read(tmp_outgoing_len);
    in_slice = rx_ring.Write(tmp_outgoing_len);

    outgoing_len = tmp_outgoing_len;

    transaction_t& transaction = tr_ring.Write();
    transaction.outgoing_len = outgoing_len;
    transaction.incoming_len = incoming_len;
    transaction.rx_read_idx = rx_ring.ReadIdx();
    transaction.rx_write_idx = rx_ring.WriteIdx();

    // Transmit the length
    HAL_SPI_TransmitReceive_DMA(&hspi, (const uint8_t*)&outgoing_len, (uint8_t*)&incoming_len,
                                sizeof(outgoing_len));
}

void SPI::TransactionCallback()
{
    while (hspi.hdmatx->State != HAL_DMA_STATE_READY)
    {
        continue;
    }

    switch (state)
    {
    case State::Start:
    {
        state = State::Transacting;
        HAL_SPI_TransmitReceive_DMA(&hspi, out_slice, in_slice, outgoing_len);
        break;
    }
    case State::Transacting:
    {
        state = State::Idle;

        Transaction();
        break;
    }
    default:
    {
        break;
    }
    }
}
