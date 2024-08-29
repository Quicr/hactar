#pragma once

#include <cstddef>

class SerialInterface
{
public:
    virtual size_t Unread() = 0;
    virtual unsigned char Read() = 0;
    virtual bool TransmitReady() = 0;
    virtual void Transmit(unsigned char* buff, const unsigned short buff_size) = 0;
};