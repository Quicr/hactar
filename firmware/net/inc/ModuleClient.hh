#include <ESP8266WiFi.h>
#include "../shared_inc/Vector.hh"
#include "../shared_inc/Packet.hh"

class ModuleClient
{
public:
    ModuleClient(String host, unsigned int port);
    ~ModuleClient();

    bool SendMessages();
    void EnqueuePacket(Packet &packet);

    Packet GetMessage();
    bool GetMessage(Packet& incoming_packet);
private:
    bool Connect();

    WiFiClient client;
    Vector<Packet> packets;
    String host;
    unsigned int port;
};