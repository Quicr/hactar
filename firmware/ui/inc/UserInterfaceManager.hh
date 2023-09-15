#pragma once

#include <map>

#include "stm32.h"

#include "EEPROM.hh"
#include "Message.hh"
#include "Packet.hh"
#include "Q10Keyboard.hh"
#include "SerialManager.hh"
#include "SerialInterface.hh"
#include "SettingManager.hh"
#include "Screen.hh"

#define VIEW_ADDRESS 0x00
#define FIRST_BOOT_STARTED 0x01
#define FIRST_BOOT_DONE 0x02

class ViewInterface;

class UserInterfaceManager
{
public:
    UserInterfaceManager(Screen& screen,
                         Q10Keyboard& keyboard,
                         SerialInterface& net_interface,
                         EEPROM& eeprom);
    ~UserInterfaceManager();

    void Run();
    bool HasMessages();
    Vector<Message>& GetMessages();
    void ClearMessages();
    void EnqueuePacket(Packet* packet);
    void ForceRedraw();
    bool RedrawForced();
    void ConnectToWifi();
    void ConnectToWifi(const String& ssid, const String& password);

    uint32_t GetTxStatusColour() const;
    uint32_t GetRxStatusColour() const;

    const std::map<uint8_t, String>& SSIDs() const;
    void ClearSSIDs();
    bool IsConnectedToWifi() const;

    uint8_t NextPacketId();

    template<typename T>
    bool ChangeView()
    {
        if (view != nullptr)
        {
            if (dynamic_cast<T*>(view) != nullptr)
            {
                // Already the view T, don't change views
                return false;
            }

            delete view;
        }

        view = new T(*this, *screen, *keyboard, setting_manager);

        return true;
    }

    const String& GetUsername();

private:
    void HandleIncomingPackets();
    void TimeoutPackets();
    uint32_t GetStatusColour(
        const SerialManager::SerialStatus status) const;

    void SendTestPacket();
    void SendCheckWifiPacket();
    void LoadSettings();
    void LoadUsername();

    static constexpr uint32_t Serial_Read_Wait_Duration = 1000;

    Screen* screen;
    Q10Keyboard* keyboard;
    SerialManager net_layer;
    SettingManager setting_manager;
    ViewInterface* view;
    Vector<Message> received_messages;
    bool force_redraw;
    uint32_t current_time;


    std::map<uint8_t, String> ssids;
    uint32_t last_wifi_check;
    bool is_connected_to_wifi;
    uint32_t attempt_to_connect_timeout;

    uint32_t last_test_packet = 0;

    String username;
};
