#pragma once

#include "HardwareSerial.h"

#include "map"

#include "../shared_inc/Packet.hh"
#include "../shared_inc/Vector.hh"

class NetSerialManager
{
public:
    NetSerialManager(HardwareSerial* serial);
    ~NetSerialManager();

    void EnqueuePacket(Packet&& packet);
    void Transmit(Packet&& packet);
    Packet Receive();
private:
    std::map<uint8_t, Packet> sent_packets;
    Vector<Packet> unsent_packets;
    HardwareSerial* uart;
};