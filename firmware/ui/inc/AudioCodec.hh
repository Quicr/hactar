#pragma once

#include <stm32.h>
#include "stm32f4xx_hal_i2c.h"

class AudioCodec
{
private:
    typedef union
    {
        uint8_t addr;
        uint8_t bytes[2];
    } Register;

public:
    AudioCodec(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c);
    ~AudioCodec();

    bool WriteRegister(uint8_t address, uint16_t data);
    // TODO remove the "debug" param.
    bool WriteRegisterSeries(uint8_t address, uint16_t data, uint8_t debug);

    bool ReadRegister(uint8_t address, uint16_t& value);
    void Send1KHzSignal();

private:
    HAL_StatusTypeDef WriteRegisterToCodec(uint8_t address);

    I2C_HandleTypeDef* i2c;
    I2S_HandleTypeDef* i2s;

    static constexpr uint16_t Write_Condition = 0x34;

    static constexpr uint8_t Num_Registers = 56;
    static constexpr uint8_t Addr_Mask = 0xFE;
    static constexpr uint16_t Top_Bit_Mask = 0x0001;
    static constexpr uint16_t Data_Mask = 0x01FF;
    static constexpr uint8_t Max_Address = 0x55;

    // TODO label all of the registers
    Register registers[Max_Address+1] = {
        { .bytes = { 0x00, 0xa7 } }, // 0x00
        { .bytes = { 0x02, 0xa7 } }, // 0x01
        { .bytes = { 0x04, 0x00 } }, // 0x02
        { .bytes = { 0x06, 0x00 } }, // 0x03
        { .bytes = { 0x08, 0x00 } }, // 0x04
        { .bytes = { 0x0a, 0x08 } }, // 0x05
        { .bytes = { 0x0c, 0x00 } }, // 0x06
        { .bytes = { 0x0e, 0x0a } }, // 0x07
        { .bytes = { 0x11, 0xc0 } }, // 0x08
        { .bytes = { 0x12, 0x00 } }, // 0x09
        { .bytes = { 0x14, 0xff } }, // 0x0a
        { .bytes = { 0x16, 0xff } }, // 0x0b
        { .bytes = { 0x18, 0x00 } }, // 0x0c - Reserved
        { .bytes = { 0x1a, 0x00 } }, // 0x0d - Reserved
        { .bytes = { 0x1c, 0x00 } }, // 0x0e - Reserved
        { .bytes = { 0x1e, 0x00 } }, // 0x0f
        { .bytes = { 0x20, 0x00 } }, // 0x10
        { .bytes = { 0x22, 0x7b } }, // 0x11
        { .bytes = { 0x24, 0x00 } }, // 0x12
        { .bytes = { 0x26, 0x32 } }, // 0x13
        { .bytes = { 0x28, 0x00 } }, // 0x14
        { .bytes = { 0x2a, 0xc3 } }, // 0x15
        { .bytes = { 0x2c, 0xc3 } }, // 0x16
        { .bytes = { 0x2e, 0xc0 } }, // 0x17
        { .bytes = { 0x30, 0x00 } }, // 0x18
        { .bytes = { 0x32, 0x00 } }, // 0x19
        { .bytes = { 0x34, 0x00 } }, // 0x1a
        { .bytes = { 0x36, 0x00 } }, // 0x1b
        { .bytes = { 0x38, 0x00 } }, // 0x1c
        { .bytes = { 0x3a, 0x00 } }, // 0x1e - Reserved
        { .bytes = { 0x3c, 0x00 } }, // 0x1f - Reserved
        { .bytes = { 0x40, 0x00 } }, // 0x20
        { .bytes = { 0x42, 0x00 } }, // 0x21
        { .bytes = { 0x44, 0x50 } }, // 0x22
        { .bytes = { 0x46, 0x00 } }, // 0x1d
        { .bytes = { 0x48, 0x00 } }, // 0x1d
        { .bytes = { 0x4a, 0x50 } }, // 0x25
        { .bytes = { 0x4c, 0x00 } }, // 0x26
        { .bytes = { 0x4e, 0x00 } }, // 0x27
        { .bytes = { 0x50, 0x00 } }, // 0x28
        { .bytes = { 0x52, 0x00 } }, // 0x29
        { .bytes = { 0x54, 0x40 } }, // 0x2a
        { .bytes = { 0x56, 0x00 } }, // 0x2b
        { .bytes = { 0x58, 0x00 } }, // 0x2c
        { .bytes = { 0x5a, 0x50 } }, // 0x2d
        { .bytes = { 0x5c, 0x50 } }, // 0x2e
        { .bytes = { 0x5e, 0x00 } }, // 0x2f
        { .bytes = { 0x60, 0x02 } }, // 0x30
        { .bytes = { 0x62, 0x37 } }, // 0x31
        { .bytes = { 0x64, 0x4c } }, // 0x32 - Reserved
        { .bytes = { 0x66, 0x80 } }, // 0x33
        { .bytes = { 0x68, 0x08 } }, // 0x34
        { .bytes = { 0x6a, 0x31 } }, // 0x35
        { .bytes = { 0x6c, 0x26 } }, // 0x36
        { .bytes = { 0x6e, 0xe9 } }, // 0x37
    };
};