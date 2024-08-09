#pragma once

#include <cstddef>

class SerialInterface
{
public:
    virtual size_t Unread() = 0;
    virtual unsigned char Read() = 0;
    virtual bool ReadyToWrite() = 0;
    virtual void Write(unsigned char* buff, const unsigned short buff_size) = 0;
};