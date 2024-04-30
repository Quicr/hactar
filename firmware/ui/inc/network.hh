#pragma once

#include <string>

#include "SettingManager.hh"
#include "SerialPacketManager.hh"

#include "Screen.hh"

class Network
{
public:
    Network(SettingManager& settings, SerialPacketManager& serial, Screen& screen);
    ~Network() = default;

    void Update(uint32_t tick);
    void ConnectToNetwork();
    void ConnectToNetwork(const std::string& ssid, const std::string& pwd);
    void RequestSSIDs();

    const std::map<uint8_t, std::string>& GetSSIDs() const;

    Screen& screen;

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
    uint32_t next_connection_check;

    // Used for when we are connected to the fallback Network
    uint32_t next_attempt_to_connect;

    uint32_t next_handle_packets;
};