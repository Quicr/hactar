#pragma once

#include <map>

#include "stm32.h"

#include "EEPROM.hh"
#include "Message.hh"
#include "Packet.hh"
#include "Q10Keyboard.hh"
#include "SerialManager.hh"
#include "SerialInterface.hh"
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

    uint32_t NextPacketId();

    template<typename T>
    void ChangeView()
    {
        if (view != nullptr)
            delete view;

        view = new T(*this, *screen, *keyboard, *eeprom);
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
    EEPROM* eeprom;
    ViewBase* view;
    Vector<Message> received_messages;
    bool force_redraw;
    uint32_t current_time;

    uint32_t last_test_packet = 0;
};
