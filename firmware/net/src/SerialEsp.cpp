#include "../inc/SerialEsp.hh"

SerialEsp::SerialEsp(HardwareSerial& serial) :
    uart(&serial)
{

}

SerialEsp::~SerialEsp()
{
    uart = nullptr;
}

unsigned long SerialEsp::AvailableBytes()
{
    return uart->available();
}

unsigned long SerialEsp::Read()
{
    return uart->read();
}

bool SerialEsp::ReadyToWrite()
{
    return uart->availableForWrite();
}

void SerialEsp::Write(unsigned char* buff, const unsigned short buff_size)
{
    uart->write(0xFF);
    uart->write(buff, buff_size);
}