#include <iostream>

#include "../shared_inc/Packet.hh"
#include "../shared_inc/Vector.hh"
#include "../shared_inc/String.hh"

unsigned char packet_id = 1;

unsigned char GetNextPacketId()
{
    if (packet_id == 0xFE)
    {
        std::cout << "Reached 0xFE" << std::endl;
        packet_id = 1;
    }
    return packet_id++;
}

int PointerTest(int* v)
{
    return *v;
}
int x = 20;
int& ReferenceTest()
{
    return x;
}

Vector<Packet*> packets;
Vector<Packet*>& GetPackets()
{
    Packet* packet = new Packet();
    packet->SetData(12, 0, 8);
    packets.push_back(packet);

    return packets;
}

int main()
{

    Vector<Packet*>* rx_packets = &GetPackets();

    std::cout << (*rx_packets)[0]->GetData(0, 8) << std::endl;
    delete (*rx_packets)[0];
    rx_packets->erase(0);

    Packet packet;

    int count = 1;

    String message = "hello";
    std::cout << message.c_str() << std::endl;
    std::cout << message.length() << std::endl;

    char ch = 1;
    int i;
    for (i = 0 ; i < message.length(); ++i)
    {
        ch = message.c_str()[i];
        std::cout << ch << std::endl;
        if (ch == '\0') break;
    }

    std::cout << "num elements " << i << std::endl;

    int* int_ptr = new int(10);
    std::cout << PointerTest(int_ptr) << std::endl;
    delete int_ptr;

    int_ptr = &ReferenceTest();
    std::cout << *int_ptr << std::endl;
    return 0;

    while (count++ < 255)
    {
        packet.SetData(1, 0, 6);
        packet.SetData(GetNextPacketId(), 6, 8);
        packet.SetData(1, 14, 10);
        packet.SetData(1, 24, 8);

        std::cout << packet.GetData(0, 6) << " "
                  << packet.GetData(6, 8) << " "
                  << packet.GetData(14, 10) << " "
                  << packet.GetData(24, 8) << " -- "
                  << packet.GetData(0, 8) << " "
                  << packet.GetData(8, 8) << " "
                  << packet.GetData(16, 8) << " "
                  << packet.GetData(24, 8) << " "
                  << std::endl;
    }
}