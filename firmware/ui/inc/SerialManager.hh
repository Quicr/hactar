#pragma once

#include "stm32.h"
#include "Vector.hh"
#include "String.hh"
#include "Packet.hh"
#include "RingBuffer.hh"

// DELETE
#include "Screen.hh"

#define Start_Bytes 4

class SerialManager
{
public:
    typedef enum
    {
        EMPTY,
        OK,
        PARTIAL,
        BUSY,
        ERROR,
        TIMEOUT,
    } SerialStatus;

    typedef struct
    {
        SerialStatus status;
        Packet packet;
    } SerialMessage;

    SerialManager(UART_HandleTypeDef* uart_handler,
                  const uint16_t rx_buffer_sz = 1,
                  const uint16_t rx_ring_sz = 256);
    ~SerialManager();

    SerialMessage ReadSerial(uint16_t max_size, uint32_t timeout);
    SerialStatus WriteSerial(const Packet& msg, const uint32_t timeout);

    void SetTxFlag();
    SerialStatus WriteSerialInterrupt(const Packet& packet,
                                      const uint32_t current_time);
    void RxEventTrigger(const uint32_t sz);
    SerialStatus ReadSerialInterrupt(const uint32_t current_time);
    SerialStatus ReadSerialInterruptSpecialized(const uint32_t current_time);
    Vector<Packet*>& GetPackets();

private:
    bool TxWatchDog(const uint32_t current_time);
    bool RxWatchDog(const uint32_t current_time);
    void StartRx(const uint16_t num_bytes, const uint32_t current_time);
    void StartRxI();

    UART_HandleTypeDef *uart;

    // Rx
    uint16_t rx_buffer_sz;
    RingBuffer<uint8_t> rx_ring;
    Vector<Packet*> rx_parsed_packets;
    uint8_t* rx_buffer;
    Packet* rx_packet;
    uint32_t rx_packet_timeout;
    uint32_t rx_next_id;

    // Tx
    uint8_t* tx_buffer;
    uint16_t tx_buffer_sz;
    uint32_t tx_watchdog_timeout;
    volatile bool tx_is_free;

    // Legacy
    #if 0
    bool has_rx_data;
    uint32_t rx_watchdog_timeout;
    #endif
};