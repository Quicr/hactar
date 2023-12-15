#pragma once

#include <stm32.h>
#include "stm32f4xx_hal_i2c.h"

class AudioCodec
{
public:
    AudioCodec(I2C_HandleTypeDef& hi2c);
    ~AudioCodec();

    void WriteRegister(uint8_t register_data[2]);

    // Will return the cached register
    void ReadRegister();

public:
    // inline void SetRegisterBits(uint8_t data[2], uint8_t bit,);
    inline void SetBit(uint8_t data[2], uint8_t bit, bool set);

    I2C_HandleTypeDef* i2c;

    typedef struct Register
    {
        uint8_t high;
        uint8_t low;
    };

    static constexpr uint16_t Write_Condition = 0x34;
    static constexpr uint16_t Read_Condition = 0x35; // Can't be done

    // Power register and it's bits
    uint8_t Pwr_Register_1[2] = { 0x19, 0x00 };
    static constexpr uint8_t Vmid_Divider_En_High = 8; // Vmid divider high bit
    static constexpr uint8_t Vmid_Divider_En_Low = 7; // Vmid divider low bit
    static constexpr uint8_t VRef_Bit = 6; // Vref enable
    static constexpr uint8_t AINL_Bit = 5; // Analogue input PGA & boost left
    static constexpr uint8_t AINR_Bit = 4; // Analogue input PGA & boost right
    static constexpr uint8_t ADCL_Bit = 3; // Left adc
    static constexpr uint8_t ADCR_Bit = 2; // Right adc
    static constexpr uint8_t MICB_Bit = 1; // Mic bias
    static constexpr uint8_t MCLK_Disable_Bit = 0; // Master clock enable

    // Power register 2
    uint8_t Pwr_Register_2[2] = { 0x19, 0x00 };
    static constexpr uint8_t Vmid_Divider_En_High = 8; // Vmid divider high bit
    static constexpr uint8_t Vmid_Divider_En_Low = 7; // Vmid divider low bit
    static constexpr uint8_t VRef_Bit = 6; // Vref enable
    static constexpr uint8_t AINL_Bit = 5; // Analogue input PGA & boost left
    static constexpr uint8_t AINR_Bit = 4; // Analogue input PGA & boost right
    static constexpr uint8_t ADCL_Bit = 3; // Left adc
    static constexpr uint8_t ADCR_Bit = 2; // Right adc
    static constexpr uint8_t MICB_Bit = 1; // Mic bias
    static constexpr uint8_t MCLK_Disable_Bit = 0; // Master clock enable

};