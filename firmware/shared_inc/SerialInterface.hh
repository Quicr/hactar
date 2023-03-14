#pragma once

#include "Packet.hh"

class SerialInterface
{
public:
    virtual unsigned long AvailableBytes() = 0;
    virtual unsigned long Read() = 0;
    virtual bool ReadyToWrite() = 0;
    virtual void Write(unsigned char* buff, const unsigned short buff_size) = 0;
};