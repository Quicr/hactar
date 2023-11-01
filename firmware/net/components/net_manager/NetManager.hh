#pragma once

#include "SerialManager.hh"
#include "Wifi.h"
#include "qsession.h"

#define MAX_AP 10

class NetManager
{
public:
    NetManager(SerialManager* _ui_layer, 
                std::shared_ptr<net_wifi::Wifi> wifi_in,
                std::shared_ptr<QSession> qsession);

    static void HandleSerial(void* param);
    static void HandleNetwork(void* param);
private:
    void HandleSerialCommands(Packet* rx_packet);
    void HandleQChatMessages(uint8_t message_type, Packet* rx_packet, size_t offset);

    void HandleQSessionMessages(QuicrObject&& obj);

    SerialManager* ui_layer;
    std::optional<std::thread> handler_thread;
    static constexpr auto inbound_object_timeout = std::chrono::milliseconds(100);
    std::shared_ptr<AsyncQueue<QuicrObject>> inbound_objects;
    std::shared_ptr<net_wifi::Wifi> wifi;
    std::shared_ptr<QSession> quicr_session = nullptr;
};