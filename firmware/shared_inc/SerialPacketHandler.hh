#pragma once

#include "SerialManager.hh"

class SerialPacketHandler
{
public:
    SerialPacketHandler(SerialManager* serial_layer) :
        serial_layer(serial_layer)
    {

    }

    virtual void ConsumeSerialPacket();

private:
    SerialManager* serial_layer;
};