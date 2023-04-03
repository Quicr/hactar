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

class ViewBase;

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
    void EnqueuePacket(Packet&& msg);
    void ForceRedraw();
    bool RedrawForced();

    const uint32_t GetTxStatusColour() const;
    const uint32_t GetRxStatusColour() const;

    const uint8_t& UsernameAddr() const;
    uint8_t& UsernameAddr();
    const uint8_t& PasscodeAddr() const;
    uint8_t& PasscodeAddr();
    const uint8_t& SSIDAddr() const;
    uint8_t& SSIDAddr();
    const uint8_t& SSIDPasscodeAddr() const;
    uint8_t& SSIDPasscodeAddr();

    const std::map<uint8_t, String>& SSIDs() const;
    void SendJoinNetworkPacket(Packet& password_packet);

    uint32_t NextPacketId();

    template<typename T>
    void ChangeView()
    {
        if (view != nullptr)
            delete view;

        view = new T(*this, *screen, *keyboard, setting_manager);
    }

private:
    static uint32_t Packet_Id;

    void HandleIncomingPackets();
    void TimeoutPackets();
    const uint32_t GetStatusColour(
        const SerialManager::SerialStatus status) const;

    const void SendTestPacket();

    static constexpr uint32_t Serial_Read_Wait_Duration = 1000;

    Screen* screen;
    Q10Keyboard* keyboard;
    SerialManager net_layer;
    SettingManager setting_manager;
    ViewBase* view;
    Vector<Message> received_messages;
    bool force_redraw;
    uint32_t current_time;

    uint32_t last_test_packet = 0;

    std::map<uint8_t, String> ssids;

    uint8_t username_addr;
    uint8_t passcode_addr;
    uint8_t ssid_addr;
    uint8_t ssid_passcode_addr;
};
