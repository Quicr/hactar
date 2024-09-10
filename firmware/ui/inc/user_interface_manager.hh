#pragma once

#include "stm32.h"

#include "audio_chip.hh"
#include "eeprom.hh"
#include "message.hh"
#include "q10_keyboard.hh"
#include "serial_packet_manager.hh"
#include "serial_packet.hh"
#include "serial_interface.hh"
#include "setting_manager.hh"
#include "screen.hh"
#include "network.hh"

#include "qchat.hh"

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
                         EEPROM& eeprom,
                         AudioChip& audio);
    ~UserInterfaceManager();

    void Update();
    bool HasNewMessages();
    std::vector<std::string> TakeMessages();
    void PushMessage(std::string&& str);
    void ClearMessages();
    void LoopbackPacket(std::unique_ptr<SerialPacket> packet);
    void ForceRedraw();
    bool RedrawForced();

    // uint32_t GetTxStatusColour() const;
    // uint32_t GetRxStatusColour() const;

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

        view = new T(*this, screen, keyboard, setting_manager, net_layer, network, audio);

        return true;
    }

private:
    void HandleIncomingPackets();
    void TimeoutPackets();
    // uint32_t GetStatusColour(
    //     const SerialPacketManager::SerialStatus status) const;

    void SendTestPacket();
    void SendCheckWifiPacket();
    void LoadSettings();
    void LoadUsername();
    void HandleMessagePacket(std::unique_ptr<SerialPacket> packet);

    static constexpr uint32_t Serial_Read_Wait_Duration = 1000;

    Screen& screen;
    Q10Keyboard& keyboard;
    SerialPacketManager net_layer;
    SettingManager setting_manager;
    AudioChip& audio;
    ViewInterface* view;
    Network network;
    std::vector<std::string> received_messages;
    bool has_new_messages;
    std::deque<qchat::Ascii> ascii_messages;
    bool force_redraw;
    uint32_t current_tick;

    uint32_t last_test_packet = 0;

    // TODO remove?
    // Audio params
    float phase = 0;
    uint16_t* tmp_audio_buff = new uint16_t[256];

    // Chat state
    std::unique_ptr<qchat::Room> active_room;

};
