#pragma once

#include "../../shared_inc/link_packet_t.hh"
#include "../../shared_inc/ring_buffer.hh"

#ifdef PLATFORM_ESP
#include <mutex>
#endif

class SerialHandler
{
public:
    static constexpr uint8_t END = 0xC0;
    static constexpr uint8_t ESC = 0xDB;
    static constexpr uint8_t ESC_END = 0xDC;
    static constexpr uint8_t ESC_ESC = 0xDD;

    SerialHandler(const uint16_t num_rx_packets,
                  uint8_t& tx_buff,
                  const uint32_t tx_buff_sz,
                  uint8_t& rx_buff,
                  const uint32_t rx_buff_sz,
                  void (*Transmit)(void* self),
                  void* transmit_arg);
    ~SerialHandler();

    link_packet_t* Read();

    void Write(const uint8_t data, const bool end_frame = true);
    void Write(const link_packet_t& packet, const bool end_frame = true);
    void Write(const uint8_t* data, const uint16_t size, const bool end_frame = true);

    uint16_t Unread();
    uint16_t Unsent();

    // TODO DELETEME
    uint16_t RxBuffWriteIdx()
    {
        return rx_write_idx;
    }
    uint16_t RxBuffReadIdx()
    {
        return rx_read_idx;
    }

protected:
    inline __attribute__((always_inline)) void WriteToTxBuff(const uint8_t data);
    inline __attribute__((always_inline)) uint8_t ReadFromRxBuff();
    virtual void UpdateUnread(const uint16_t update) = 0;

    void UpdateRx(const uint16_t num_recv);
    bool UpdateTx();
    bool PrepTransmit();

    RingBuffer<link_packet_t> rx_packets;

    uint8_t* tx_buff;
    uint32_t tx_buff_sz;
    uint8_t* rx_buff;
    uint32_t rx_buff_sz;

    uint16_t tx_write_idx;
    uint16_t tx_read_idx;
    bool tx_free;
    uint16_t unsent;
    uint16_t num_to_send;

    uint16_t rx_write_idx;
    uint16_t rx_read_idx;
    volatile uint16_t unread;
    volatile uint16_t update_cache;

    void (*Transmit)(void* self);
    void* transmit_arg;

    link_packet_t* packet;
    uint32_t bytes_read;
    bool escaped;

#ifdef PLATFORM_ESP
    std::mutex write_mux;
#endif
};