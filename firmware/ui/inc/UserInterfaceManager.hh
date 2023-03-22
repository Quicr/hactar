#pragma once

#include <map>

#include "stm32.h"

#include "Q10Keyboard.hh"
#include "Message.hh"
#include "Packet.hh"
#include "SerialManager.hh"
#include "SerialInterface.hh"
#include "Screen.hh"

class ViewBase;

class UserInterfaceManager
{
public:
    UserInterfaceManager(Screen& screen,
                         Q10Keyboard& keyboard,
                         SerialInterface& net_interface);
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

        view = new T(*this, *screen, *keyboard);
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
    ViewBase* view;
    Vector<Message> received_messages;
    bool force_redraw;
    uint32_t current_time;

    uint32_t last_test_packet = 0;
};
