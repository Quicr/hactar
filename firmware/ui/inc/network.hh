#pragma once

#include <string>

#include "setting_manager.hh"
#include "serial_packet_manager.hh"

#include "screen.hh"

class Network
{
public:
    Network(SettingManager& settings, SerialPacketManager& serial);
    ~Network() = default;

    void Update(uint32_t tick);
    void ConnectToNetwork();
    void ConnectToNetwork(const std::string& ssid, const std::string& pwd);
    void RequestSSIDs();

    bool IsConnected() const;
    const std::map<uint8_t, std::string>& GetSSIDs() const;

private:

    inline void ConnectUsingFallback();
    void IngestNetworkPackets();
    void HandleStatusPackets();
    void SendStatusPacket();
    void SendConnectPacket(const std::string& ssid, const std::string& pwd);

    static constexpr char fallback_Network_ssid[] = "quicr.io";
    static constexpr char fallback_Network_pwd[] = "noPassword";

    SettingManager& settings;
    SerialPacketManager& serial;

    std::map<uint8_t, std::string> ssids;

    // TODO pack these flags into a single byte.
    bool using_fallback;
    bool is_connected;
    bool connection_attempt;
    bool fallback_failed;
    uint32_t next_connection_check;

    uint32_t next_handle_packets;

    int8_t num_connection_attempts;
};