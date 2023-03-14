#pragma once

#include "SerialInterface.hh"
#include "Vector.hh"
#include "String.hh"
#include "Packet.hh"
#include "RingBuffer.hh"

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

    SerialManager(SerialInterface* uart);
    ~SerialManager();

    SerialStatus ReadSerial(const unsigned long current_time);
    SerialStatus WriteSerial(const Packet& packet,
                             const unsigned long current_time);
    Vector<Packet*>& GetPackets();

private:
    SerialInterface* uart;

    Vector<Packet*> rx_parsed_packets;
    Packet* rx_packet;
    unsigned long rx_packet_timeout;
    unsigned long rx_next_id;
};