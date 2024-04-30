#pragma once

#include "stm32.h"

#include "EEPROM.hh"
#include "Message.hh"
#include "Q10Keyboard.hh"
#include "SerialPacketManager.hh"
#include "SerialPacket.hh"
#include "SerialInterface.hh"
#include "SettingManager.hh"
#include "Screen.hh"
#include "network.hh"

#include "QChat.hh"

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

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
    bool HasNewMessages();
    std::vector<std::string> TakeMessages();
    void PushMessage(std::string&& str);
    void ClearMessages();
    void EnqueuePacket(std::unique_ptr<SerialPacket> packet);
    void LoopbackPacket(std::unique_ptr<SerialPacket> packet);
    void ForceRedraw();
    bool RedrawForced();

    uint32_t GetTxStatusColour() const;
    uint32_t GetRxStatusColour() const;

    bool GetReadyPackets(
        RingBuffer<std::unique_ptr<SerialPacket>>** buff,
        const SerialPacket::Commands command_type) const;
    bool IsConnectedToWifi() const;

    uint16_t NextPacketId();

    void ChangeRoom(std::unique_ptr<qchat::Room> new_room);
    const std::unique_ptr<qchat::Room>& ActiveRoom() const;

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

        view = new T(*this, *screen, *keyboard, setting_manager, network);

        return true;
    }

private:
    void HandleIncomingPackets();
    void TimeoutPackets();
    uint32_t GetStatusColour(
        const SerialPacketManager::SerialStatus status) const;

    void SendTestPacket();
    void SendCheckWifiPacket();
    void LoadSettings();
    void LoadUsername();
    void HandleMessagePacket(std::unique_ptr<SerialPacket> packet);

    static constexpr uint32_t Serial_Read_Wait_Duration = 1000;

    Screen* screen;
    Q10Keyboard* keyboard;
    SerialPacketManager net_layer;
    SettingManager setting_manager;
    ViewInterface* view;
    Network network;
    std::vector<std::string> received_messages;
    bool has_new_messages;
    std::deque<qchat::Ascii> ascii_messages;
    bool force_redraw;
    uint32_t current_tick;


    std::map<SerialPacket::Commands, RingBuffer<std::unique_ptr<SerialPacket>>> pending_command_packets;
    uint32_t last_wifi_check;
    bool is_connected_to_wifi;
    uint32_t attempt_to_connect_timeout;

    uint32_t last_test_packet = 0;

    // Chat state
    std::unique_ptr<qchat::Room> active_room;

};
