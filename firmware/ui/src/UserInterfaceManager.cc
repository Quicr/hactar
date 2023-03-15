#include "UserInterfaceManager.hh"
#include "String.hh"
#include "LoginView.hh"
#include "ChatView.hh"
#include "TeamView.hh"

// Init the static var
uint32_t UserInterfaceManager::Packet_Id = 0;

UserInterfaceManager::UserInterfaceManager(Screen &screen,
                                           Q10Keyboard &keyboard,
                                           SerialInterface &net_interface) :
    screen(&screen),
    keyboard(&keyboard),
    net_layer(&net_interface),
    view(nullptr),
    received_messages(), // TODO limit?
    force_redraw(false),
    current_time(HAL_GetTick())
{
    ChangeView<LoginView>();
}

UserInterfaceManager::~UserInterfaceManager()
{
    // TODO note - Do we need to delete the next 3?
    // Probably not since they are allocated outside of this class
    // However it does manage it
    delete screen;
    delete keyboard;
    delete view;
}

// TODO should update this to be a draw/update architecture
void UserInterfaceManager::Run()
{
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

        // P_type will only be message or debug by this point
        if (p_type == Packet::Types::Message)
        {
            // Write a message to the screen
            Message in_msg;
            // TODO The message should be parsed some how here.
            in_msg.Timestamp("00:00");
            in_msg.Sender("Server");

            String body;

            // Skip the type and length, add the whole message
            unsigned short packet_len = rx_packet.GetData(14, 10);
            for (uint32_t j = 0; j < packet_len; ++j)
            {
                body.push_back((char)rx_packet.GetData(24 + (j * 8), 8));
            }

            in_msg.Body(body);
            received_messages.push_back(in_msg);
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
