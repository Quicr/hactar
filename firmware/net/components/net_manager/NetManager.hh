#pragma once

#include "SerialManager.hh"
#include "Wifi.hh"
#include "qsession.h"

#define MAX_AP 10

class NetManager
{
public:
    NetManager(SerialManager* _ui_layer, std::shared_ptr<QSession> qsession);

    static void HandleSerial(void* param);
    static void HandleNetwork(void* param);
private:
    void HandleSerialCommands(Packet* rx_packet);

    SerialManager* ui_layer;
    hactar_utils::Wifi* wifi;
    std::shared_ptr<QSession> quicr_session = nullptr;
};