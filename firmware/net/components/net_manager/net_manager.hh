#pragma once

#include "serial_packet_manager.hh"
#include "serial_packet.hh"
#include "wifi.hh"
#include "qsession.hh"
#include "ring_buffer.hh"

#define MAX_AP 10

class NetManager
{
public:
    // NetManager(SerialPacketManager& ui_layer,
    //             Wifi& wifi,
    //             std::shared_ptr<QSession> qsession,
    //             std::shared_ptr<AsyncQueue<QuicrObject>> inbound_objects);
    NetManager(SerialPacketManager& ui_layer,
                Wifi& wifi);

    NetManager(Wifi& wifi);

    void Update();

    static void HandleAudio(void* param);

    static void HandleSerial(void* param);
    // static void HandleNetwork(void* param);
private:
    void HandleSerialCommands();
    void HandleQChatMessages(uint8_t message_type, const std::unique_ptr<SerialPacket>& rx_packet, size_t offset);
    static void HandleWatchMessage(void* watch);

    // void HandleQSessionMessages(QuicrObject&& obj);

    void GetSSIDsCommand();
    void ConnectToWifiCommand(const std::unique_ptr<SerialPacket>& packet);
    void GetWifiStatusCommand();
    void GetRoomsCommand();

    SerialPacketManager& ui_layer;


    Wifi& wifi;
    // std::optional<std::thread> handler_thread;
    static constexpr auto inbound_object_timeout = std::chrono::milliseconds(100);
    std::shared_ptr<QSession> quicr_session;
    std::shared_ptr<AsyncQueue<QuicrObject>> inbound_objects;
    RingBuffer<std::unique_ptr<SerialPacket>> audio_buffer;
    bool audio_buffered;
};
