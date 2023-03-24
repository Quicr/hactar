#pragma once

#include "PortPin.hh"
#include "Vector.hh"

class EEPROM
{
public:

    EEPROM(I2C_HandleTypeDef& hi2c);
    ~EEPROM();

    void Write(uint8_t* data, const uint16_t sz);
    void Read(const uint8_t address, uint8_t* data, const uint16_t sz);

private:
    static constexpr uint8_t Read_Condition     = 0xA1;
    static constexpr uint8_t Write_Condition    = 0xA0;
    I2C_HandleTypeDef* i2c;
};