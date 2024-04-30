#include "network.hh"

// TODO rename to network

Network::Network(SettingManager& settings, SerialPacketManager& serial, Screen& screen):
    settings(settings),
    serial(serial),
    screen(screen),
    using_fallback(false),
    is_connected(false),
    connection_attempt(false),
    next_connection_check(0),
    next_attempt_to_connect(0),
    next_handle_packets(0)
{

}

void Network::Update(uint32_t tick)
{
    if (tick > next_connection_check)
    {
        SendStatusPacket();
        next_connection_check = tick + 10000;
    }

    // TODO shouldn't this be somewhere else?
    // if (tick > next_attempt_to_connect && !is_connected)
    // {
    //     ConnectToNetwork();
    //     next_attempt_to_connect = tick + 5000;
    // }

    if (tick > next_handle_packets)
    {
        IngestNetworkPackets();
        next_handle_packets = tick + 1000;
    }
}

void Network::ConnectToNetwork()
{
    const std::string* ssid = settings.SSID();
    if (!ssid)
    {
        // No ssid was loaded, try to connect to the fallback Network
        ConnectUsingFallback();
        return;
    }

    const std::string* pwd = settings.SSIDPassword();
    if (!pwd)
    {
        // No ssid pwd was loaded, try to connect to the fallback Network
        ConnectUsingFallback();
        return;
    }

    // TODO what about checking for all of the ssids in range before trying to connect ot Network?
    // TODO send the connection request anyways?
    // TODO check if the ssid is in range.
    RequestSSIDs();

    // Get the ssids from the manager

    // Check if our ssid is in the list.
    if (true)
    {
        ConnectToNetwork(*ssid, *pwd);
    }
    else
    {
        ConnectUsingFallback();
    }
}

void Network::ConnectToNetwork(const std::string& ssid, const std::string& pwd)
{
    using_fallback = false;
    SendConnectPacket(ssid, pwd);
}

void Network::RequestSSIDs()
{
    std::unique_ptr<SerialPacket> ssid_req_packet = std::make_unique<SerialPacket>();
    ssid_req_packet->SetData(SerialPacket::Types::Command, 0, 1);
    ssid_req_packet->SetData(serial.NextPacketId(), 1, 2);
    ssid_req_packet->SetData(3, 3, 2);
    ssid_req_packet->SetData(SerialPacket::Commands::Wifi, 5, 2);
    ssid_req_packet->SetData(SerialPacket::WifiTypes::SSIDs, 7, 1);
    serial.EnqueuePacket(std::move(ssid_req_packet));
}

const std::map<uint8_t, std::string>& Network::GetSSIDs() const
{
    return ssids;
}

void Network::ConnectUsingFallback()
{
    using_fallback = true;
    SendConnectPacket(fallback_Network_ssid, fallback_Network_ssid);
}

void Network::SendStatusPacket()
{
    // Check a check Network status packet
    std::unique_ptr<SerialPacket> check_Network = std::make_unique<SerialPacket>(HAL_GetTick());

    // Set the command
    check_Network->SetData(SerialPacket::Types::Command, 0, 1);

    // Set the id
    check_Network->SetData(serial.NextPacketId(), 1, 2);

    // Set the size
    check_Network->SetData(3, 3, 2);

    // Set the data
    check_Network->SetData(SerialPacket::Commands::Wifi, 5, 2);
    check_Network->SetData(SerialPacket::WifiTypes::Status, 7, 1);

    Logger::Log(Logger::Level::Info, "Send check Network to esp");

    serial.EnqueuePacket(std::move(check_Network));
}

void Network::SendConnectPacket(const std::string& ssid, const std::string& pwd)
{
    connection_attempt = true;

    const uint16_t command_byte_length = 2;
    const uint16_t Network_command_byte_length = 1;
    const uint16_t Network_cred_lengths = 2;

    // Save to the eeprom? or only save if connected.
    std::unique_ptr<SerialPacket> connect_packet = std::make_unique<SerialPacket>(HAL_GetTick());
    connect_packet->SetData(SerialPacket::Types::Command, 0, 1);
    connect_packet->SetData(serial.NextPacketId(), 1, 2);

    uint16_t ssid_len = ssid.length();
    uint16_t ssid_password_len = pwd.length();
    uint16_t length = ssid_len
        + ssid_password_len
        + command_byte_length
        + Network_command_byte_length
        + (Network_cred_lengths * 2);
    connect_packet->SetData(length, 3, 2);

    connect_packet->SetData(SerialPacket::Commands::Wifi, 5, 2);
    connect_packet->SetData(SerialPacket::WifiTypes::Connect, 7, 1);

    // Set the length of the ssid
    connect_packet->SetData(ssid_len, 8, Network_cred_lengths);

    // Populate with the ssid
    uint16_t i;
    uint16_t offset = 10;
    for (i = 0; i < ssid_len; ++i)
    {
        connect_packet->SetData(ssid[i], offset, 1);
        offset += 1;
    }

    // Set the length of the password
    connect_packet->SetData(ssid_password_len, offset, Network_cred_lengths);
    offset += 2;

    // Populate with the password
    uint16_t j;
    for (j = 0; j < ssid_password_len; ++j)
    {
        connect_packet->SetData(pwd[j], offset, 1);
        offset += 1;
    }

    // Enqueue the message
    serial.EnqueuePacket(std::move(connect_packet));
}

void Network::IngestNetworkPackets()
{
    RingBuffer<std::unique_ptr<SerialPacket>>* Network_packets;

    if (!serial.GetCommandPackets(&Network_packets, SerialPacket::Commands::Wifi))
    {
        return;
    }

    screen.DrawText(0, 160, "We have packets!", font6x8, C_WHITE, C_BLACK);
    // Ingest packets
    while (Network_packets->Unread() > 0)
    {
        auto packet = std::move(Network_packets->Read());

        auto Network_type = static_cast<SerialPacket::WifiTypes>(
            packet->GetData<uint8_t>(7, 1));

        screen.DrawText(0, 168, "Network type: ", font6x8, C_WHITE, C_BLACK);
        screen.DrawText(0 + 14 * 6, 168, std::to_string((int)Network_type), font6x8, C_WHITE, C_BLACK);
        switch (Network_type)
        {
            case SerialPacket::WifiTypes::Status:
            {
                uint8_t status = packet->GetData<uint8_t>(8, 1);
                screen.DrawText(0, 176, "status:", font6x8, C_WHITE, C_BLACK);
                screen.DrawText(0 + 7 * 6, 176, std::to_string(status), font6x8, C_WHITE, C_BLACK);


                if (status)
                {
                    is_connected = true;
                    connection_attempt = false;
                    continue; //?
                }

                if (connection_attempt)
                {
                    // Already attempting to connect.
                    continue;
                }

                // Not connected so send a connect packet.
                ConnectToNetwork();

                break;
            }
            case SerialPacket::WifiTypes::SSIDs:
            {
                // Got the new ssids in range, so clear the old ones.
                screen.DrawText(0, 184, "got ssid", font6x8, C_WHITE, C_BLACK);
                ssids.clear();

                // Get the ssid id
                uint8_t ssid_id = packet->GetData<uint8_t>(8, 1);

                // Get the packet len
                uint16_t ssid_length = packet->GetData<uint16_t>(9, 1);

                // Build the string
                std::string str;
                for (uint8_t i = 0; i < ssid_length - 2; ++i)
                {
                    str.push_back(packet->GetData<char>(10 + i, 1));
                }

                ssids[ssid_id] = std::move(str);
                break;
            }
            case SerialPacket::WifiTypes::Connect:
            {

                break;
            }
            case SerialPacket::WifiTypes::Disconnect:
            {

                break;
            }
            case SerialPacket::WifiTypes::Disconnected:
            {

                break;
            }
            case SerialPacket::WifiTypes::FailedToConnect:
            {
                connection_attempt = false;
                break;
            }
            case SerialPacket::WifiTypes::SignalStrength:
            {

                break;
            }
            default:
            {
                break;
            }
        }
    }
}