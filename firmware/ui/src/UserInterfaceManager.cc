#include "UserInterfaceManager.hh"
#include "String.hh"
#include "FirstBootView.hh"
#include "LoginView.hh"
#include "ChatView.hh"
#include "TeamView.hh"

#include "SettingManager.hh"

// Init the static var
uint32_t UserInterfaceManager::Packet_Id = 1;

UserInterfaceManager::UserInterfaceManager(Screen &screen,
                                           Q10Keyboard &keyboard,
                                           SerialInterface &net_interface,
                                           EEPROM& eeprom) :
    screen(&screen),
    keyboard(&keyboard),
    net_layer(&net_interface),
    eeprom(&eeprom),
    view(nullptr),
    received_messages(), // TODO limit?
    force_redraw(false),
    current_time(HAL_GetTick())
{
    // Get if first boot or not


    // The first byte will be the length of the next set of data
    if (eeprom.ReadByte(1) == FIRST_BOOT_DONE)
        ChangeView<LoginView>();
    else
        ChangeView<FirstBootView>();

}

UserInterfaceManager::~UserInterfaceManager()
{
    screen = nullptr;
    keyboard = nullptr;
    net_layer = nullptr;
    eeprom = nullptr;
}

// TODO should update this to be a draw/update architecture
void UserInterfaceManager::Run()
{
    // if (current_time > last_test_packet)
    // {
    //     SendTestPacket();
    //     last_test_packet = current_time + 5000;
    // }


    current_time = HAL_GetTick();

    view->Run();

    if (RedrawForced())
    {
        force_redraw = false;
        return;
    }

    // Run the receive and transmit
    net_layer.RxTx(current_time);

    // TODO we probably should keep a small list of the most recent messages
    //      in the user interface manager instead of chat view
    //      otherwise it will be bizarre having to get all of the old messages.
    // TODO this should only occur in the chat view mode?

    HandleIncomingPackets();
}

bool UserInterfaceManager::HasMessages()
{
    return received_messages.size();
}

Vector<Message>& UserInterfaceManager::GetMessages()
{
    return received_messages;
}

void UserInterfaceManager::ClearMessages()
{
    received_messages.clear();
}

void UserInterfaceManager::EnqueuePacket(Packet&& packet)
{
    // TODO maybe make this into a linked list?
    net_layer.EnqueuePacket(std::move(packet));
}

void UserInterfaceManager::ForceRedraw()
{
    force_redraw = true;
    // TODO change this to draw
    Run();
}

bool UserInterfaceManager::RedrawForced()
{
    return force_redraw;
}

uint32_t UserInterfaceManager::NextPacketId()
{
    return UserInterfaceManager::Packet_Id++;
}

void UserInterfaceManager::HandleIncomingPackets()
{
    if (!net_layer.HasRxPackets()) return;

    // Get the packets
    Vector<Packet*>& packets = net_layer.GetRxPackets();

    // Handle incoming packets
    while (packets.size() > 0)
    {
        // Get the type
        Packet& rx_packet = *packets[0];
        uint8_t p_type = rx_packet.GetData(0, 6);
        switch (p_type)
        {
            // P_type will only be message or debug by this point
            case (Packet::Types::Message):
            {
                // Write a message to the screen
                Message in_msg;
                // TODO The message should be parsed some how here.
                in_msg.Timestamp("00:00");
                in_msg.Sender("Server");

                String body;

                // Skip the type and length, add the whole message
                uint16_t packet_len = rx_packet.GetData(14, 10);
                for (uint32_t j = 0; j < packet_len; ++j)
                {
                    body.push_back((char)rx_packet.GetData(24 + (j * 8), 8));
                }

                in_msg.Body(body);
                received_messages.push_back(in_msg);
                break;
            }
            case (Packet::Types::Setting):
            {
                // For settings we expect the data to dedicate 16 bits to the id
                uint16_t packet_len = rx_packet.GetData(14, 10);

                // Get the setting id
                uint16_t setting_id = rx_packet.GetData(24, 16);

                // Get the data from the packet and set the setting
                // for now we expect a 32 bit value for each setting
                uint32_t setting_data = rx_packet.GetData(40, 32);

                // TODO Set the setting

                break;
            }
            default:
            {
                // We'll do nothing if it doesn't fit these types
            }
        }
        packets.erase(0);
    }
}

const uint32_t UserInterfaceManager::GetTxStatusColour() const
{
    return GetStatusColour(net_layer.GetTxStatus());
}

const uint32_t UserInterfaceManager::GetRxStatusColour() const
{
    return GetStatusColour(net_layer.GetRxStatus());
}

const uint8_t& UserInterfaceManager::UsernameAddr() const
{
    return username_addr;
}

uint8_t& UserInterfaceManager::UsernameAddr()
{
    return username_addr;
}

const uint8_t& UserInterfaceManager::PasscodeAddr() const
{
    return passcode_addr;
}

uint8_t& UserInterfaceManager::PasscodeAddr()
{
    return passcode_addr;
}

Vector<String> RequestSSIDs()
{

}

void ConnectToWifi(const String& password)
{

}

const uint32_t UserInterfaceManager::GetStatusColour(
    const SerialManager::SerialStatus status) const
{
    if (status == SerialManager::SerialStatus::OK)
        return C_GREEN;
    else if (status == SerialManager::SerialStatus::PARTIAL)
        return C_CYAN;
    else if (status == SerialManager::SerialStatus::TIMEOUT)
        return C_MAGENTA;
    else if (status == SerialManager::SerialStatus::BUSY)
        return C_YELLOW;
    else if (status == SerialManager::SerialStatus::ERROR)
        return C_RED;
    else
        return C_WHITE;
}

const void UserInterfaceManager::SendTestPacket()
{
    Packet test_packet(current_time);

    test_packet.SetData(Packet::Types::Message, 0, 6);
    test_packet.SetData(NextPacketId(), 6, 8);
    test_packet.SetData(5, 14, 10);
    test_packet.SetData('H', 24, 8);
    test_packet.SetData('e', 32, 8);
    test_packet.SetData('l', 40, 8);
    test_packet.SetData('l', 48, 8);
    test_packet.SetData('o', 56, 8);

    EnqueuePacket(std::move(test_packet));
}
