#include "link_packet_t.hh"
#include "ring_buffer.hh"
#include "stm32.h"
#include <cstdint>

class SPI
{
public:
    enum class State
    {
        Idle,
        Start,
        Transacting
    };

    SPI(SPI_HandleTypeDef& hspi,
        uint8_t* tx_buff,
        uint16_t tx_buff_sz,
        uint8_t* rx_buff,
        uint16_t rx_buff_sz);

    void Init();
    void Deinit();
    void Transmit(const uint8_t* data, const uint16_t len);
    void Transmit(const link_packet_t& packet);
    const link_packet_t* Receive();
    void Transaction();
    void TransactionCallback(SPI* spi);

private:
    static constexpr size_t Linear_Buff_Size = link_packet_t::Packet_Size * 2;
    static constexpr size_t Rx_Packet_Sz = 5;

    void CallbackHandler();

    SPI_HandleTypeDef& hspi;

    RingBuffer<uint8_t> tx_ring;
    RingBuffer<uint8_t> rx_ring;

    uint8_t* tx_buff;
    uint16_t tx_buff_sz;
    uint8_t* rx_buff;
    uint16_t rx_buff_sz;

    State state;

    uint16_t tx_write_idx;
    uint16_t tx_read_idx;
    uint16_t untransmitted;

    uint16_t rx_write_idx;
    uint16_t rx_read_idx;
    uint16_t unread;

    uint32_t outgoing_len;
    uint32_t incoming_len;
    uint32_t in_queue;

    link_packet_t _link_packet_buff[Rx_Packet_Sz];
    RingBuffer<link_packet_t> rx_packets;

    uint8_t linear_tx_buff[Linear_Buff_Size];
    uint8_t linear_rx_buff[Linear_Buff_Size];
};
