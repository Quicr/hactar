#pragma once

#include <map>

#include "stm32.h"

#include "Q10Keyboard.hh"
#include "Message.hh"
#include "Packet.hh"
#include "SerialManager.hh"
#include "Screen.hh"

class ViewBase;

class UserInterfaceManager
{
public:
    UserInterfaceManager(Screen &screen,
                         Q10Keyboard &keyboard,
                         SerialManager &net_layer);
    ~UserInterfaceManager();

    void Run();
    bool HasMessages();
    Vector<Message>& GetMessages();
    void ClearMessages();
    void EnqueuePacket(Packet& msg);
    void ForceRedraw();
    bool RedrawForced();

    template<typename T>
    void ChangeView()
    {
        if (view != nullptr)
            delete view;

        view = new T(*this, *screen, *keyboard);
    }
private:
    void HandleIncomingSerial();
    void HandleOutgoingSerial();
    const uint32_t GetStatusColour(
        const SerialManager::SerialStatus status) const;

    static constexpr uint32_t Serial_Read_Wait_Duration = 1000;

    Screen *screen;
    Q10Keyboard *keyboard;
    SerialManager *net_layer;
    ViewBase *view;
    Vector<Message> received_messages;
    Vector<Packet> unsent_tx_packets;
    std::map<uint16_t, Packet> sent_tx_packets;

    bool force_redraw;

    uint32_t current_time;

    uint32_t next_message_receive_timeout;
    uint32_t next_message_transmit_timeout;
    const String TX = "tx";
    const String RX = "rx";
};