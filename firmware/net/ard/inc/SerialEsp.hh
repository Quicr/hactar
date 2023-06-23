#pragma once

#include "HardwareSerial.h"
#include "../shared_inc/SerialInterface.hh"

class SerialEsp : public SerialInterface
{
public:
    SerialEsp(HardwareSerial& serial);
    ~SerialEsp();

    unsigned long AvailableBytes() override;
    unsigned long Read() override;
    bool ReadyToWrite() override;
    void Write(unsigned char* buff, const unsigned short buff_size) override;
private:
    HardwareSerial* uart;
};