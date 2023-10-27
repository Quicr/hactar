#include "Packet.hh"
#include "SerialManager.hh"
#include "QChat.hh"

void SerialLoopbackQMessage(SerialManager* serial_layer, qchat::Ascii ascii)
{
    // Loop it back onto the tx
    // Need to make a new packet, as this is a hack
    Packet* packet = new Packet(0, 1);
    packet->SetData(Packet::Types::Message, 0, 6);
    packet->SetData(1, 6, 8);

    // The packet length is set in the encode function
    // TODO encode probably could just generate a packt instead...
    qchat::Codec::encode(packet, 14, ascii);

    uint64_t new_offset = packet->BitsUsed();
    // Expiry time
    packet->SetData(0xFFFFFFFF, new_offset, 32);
    new_offset += 32;
    // Creation time
    packet->SetData(0, new_offset, 32);
    new_offset += 32;

    serial_layer->EnqueuePacket(packet);
}