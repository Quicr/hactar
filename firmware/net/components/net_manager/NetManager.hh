#pragma once

#include "SerialManager.hh"

class NetManager
{
public:
    NetManager(SerialManager* serial);

    static void HandleSerial(void* param);
    static void HandleNetwork(void* param);
private:
    void HandleSerialCommands(Packet* rx_packet);

    SerialManager* serial;
};