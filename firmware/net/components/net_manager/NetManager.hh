#pragma once

#include "SerialManager.hh"
#include "Wifi.hh"

#define MAX_AP 10

class NetManager
{
public:
    NetManager(SerialManager* _ui_layer);

    static void HandleSerial(void* param);
    static void HandleNetwork(void* param);
private:
    void HandleSerialCommands(Packet* rx_packet);

    SerialManager* ui_layer;
    hactar_utils::Wifi* wifi;
};