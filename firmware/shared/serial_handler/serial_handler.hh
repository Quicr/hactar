#pragma once

#include "ring_buffer.hh"
#include "link_packet_t.hh"
#include "logger.hh"

template<size_t rx_buffer_packets>
class SerialHandler
{
protected:
    RingBuffer<link_packet_t, rx_buffer_packets> rx_packets;

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

public:

    static constexpr uint8_t END = 0xC0;
    static constexpr uint8_t ESC = 0xDB;
    static constexpr uint8_t ESC_END = 0xDC;
    static constexpr uint8_t ESC_ESC = 0xDD;

    SerialHandler(
        uint8_t& tx_buff, const uint32_t tx_buff_sz,
        uint8_t& rx_buff, const uint32_t rx_buff_sz,
        void (*Transmit)(void* self),
        void* transmit_arg):
        rx_packets(),
        tx_buff(&tx_buff),
        tx_buff_sz(tx_buff_sz),
        rx_buff(&rx_buff),
        rx_buff_sz(rx_buff_sz),
        tx_write_idx(0),
        tx_read_idx(0),
        tx_free(true),
        unsent(0),
        num_to_send(0),
        rx_write_idx(0),
        rx_read_idx(0),
        unread(0),
        update_cache(0),
        Transmit(Transmit),
        transmit_arg(transmit_arg),
        bytes_read(0),
        escaped(false)
    {
        packet = &rx_packets.Write();
    }

    link_packet_t* Read(){
        // TODO for some reason when the semaphore can't be taken
        // we get a lot of frame errors
        // even though our update cache should be handling that
        // by offsetting the total bytes read
        uint16_t total_bytes_read = 0;
        uint8_t byte = 0;
        while (total_bytes_read + update_cache < unread)
        {
            // Get a byte
            byte = ReadFromRxBuff();
            ++total_bytes_read;

            // Note if we don't cast everything, esp32 freaks out
            if (byte == END && uint16_t(bytes_read) == uint16_t(packet->length + link_packet_t::Header_Size))
            {
                packet->is_ready = true;

                // Null out our packet pointer
                packet = &rx_packets.Write();
                bytes_read = 0;
                continue;
            }
            else if (byte == END)
            {
                Logger::Log(Logger::Level::Info, "Frame len error");

                packet->is_ready = false;
                bytes_read = 0;
                continue;
            }

            if (bytes_read >= PACKET_SIZE)
            {
                Logger::Log(Logger::Level::Info, "Frame overflow error");
                // Hit maximum size and didn't get an end packet

                packet->is_ready = false;
                bytes_read = 0;
                continue;
            }

            if (byte == ESC)
            {
                escaped = true;
                continue;
            }

            if (escaped)
            {
                if (byte == ESC_END)
                {
                    packet->data[bytes_read++] = END;
                }
                else if (byte == ESC_ESC)
                {
                    packet->data[bytes_read++] = ESC;
                }

                escaped = false;
            }
            else
            {
                packet->data[bytes_read++] = byte;
            }
        }

        if (total_bytes_read > 0 || update_cache > 0)
        {
            UpdateUnread(total_bytes_read);
        }

        if (!rx_packets.Peek().is_ready)
        {
            return nullptr;
        }

        link_packet_t* p = &rx_packets.Read();
        p->is_ready = false;
        return p;
    }

    void Write(const uint8_t data){
        Write(&data, 1);
    }

    void Write(const link_packet_t& packet){
      Write(packet.data, packet.length + link_packet_t::Header_Size);
    }

    void Write(const uint8_t* data, const uint16_t size){
        for (uint16_t i = 0 ; i < size; ++i)
        {
            if (data[i] == END)
            {
                unsent += 1;
                WriteToTxBuff(ESC);
                WriteToTxBuff(ESC_END);
            }
            else if (data[i] == ESC)
            {
                unsent += 1;
                WriteToTxBuff(ESC);
                WriteToTxBuff(ESC_ESC);
            }
            else
            {
                WriteToTxBuff(data[i]);
            }
        }

        // End frame
        WriteToTxBuff(END);
        unsent += size + 1;

        if (unsent > tx_buff_sz)
        {
            // TODO ERROR
        }

        if (tx_write_idx > tx_read_idx)
        {
            // TODO error
        }

        if (!tx_free)
        {
            return;
        }

        // Logger::Log(Logger::Level::Info, "Transmit");
        PrepTransmit();
    }

    uint16_t Unread(){
        return unread;
    }

    uint16_t Unsent(){
        return unsent;
    }

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

    inline __attribute__((always_inline)) void WriteToTxBuff(const uint8_t data){
        if (tx_write_idx >= tx_buff_sz)
        {
            tx_write_idx = 0;
        }
        tx_buff[tx_write_idx++] = data;
    }

    inline __attribute__((always_inline)) uint8_t ReadFromRxBuff(){
        if (rx_read_idx >= rx_buff_sz)
        {
            rx_read_idx = 0;
        }
        return rx_buff[rx_read_idx++];
    }

    virtual void UpdateUnread(const uint16_t update) = 0;

    void UpdateRx(const uint16_t num_recv){
        unread += num_recv;
        rx_write_idx += num_recv;
        if (rx_write_idx >= rx_buff_sz)
        {
            rx_write_idx = 0;
        }

        if (unread > rx_buff_sz)
        {
            // TODO
            // Error("SerialStm rx isr", "Overflowed rx buffer");
        }
    }

    void UpdateTx(){
        // TODO esp semaphore of send
        unsent -= num_to_send;
        tx_read_idx += num_to_send;

        if (tx_read_idx >= tx_buff_sz)
        {
            tx_read_idx = 0;
        }

        PrepTransmit();
    }

    void PrepTransmit(){
        // Nothing to send
        if (unsent == 0)
        {
            tx_free = true;
            return;
        }
        tx_free = false;
        num_to_send = unsent;

        // Prevent tx buff overflow
        if (num_to_send + tx_read_idx >= tx_buff_sz)
        {
            num_to_send = tx_buff_sz - tx_read_idx;
        }

        Transmit(transmit_arg);
    }

};
