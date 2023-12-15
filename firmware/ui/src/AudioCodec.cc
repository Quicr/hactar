#include "AudioCodec.hh"

AudioCodec::AudioCodec(I2C_HandleTypeDef& hi2c): i2c(&hi2c)
{
    // Set the power register
    // TODO figure out if this should be little endian?
    Pwr_Register_1[1] = 0x01001100;
    WriteRegister(Pwr_Register_1);
}

AudioCodec::~AudioCodec()
{
    i2c = nullptr;
}

void AudioCodec::WriteRegister(uint8_t register_data[2])
{
    // const uint8_t read_condition = 0x34 + 1;
    HAL_StatusTypeDef audio_select = HAL_I2C_Master_Transmit(i2c,
        Write_Condition, register_data, 2, HAL_MAX_DELAY);
}

/** Private functions */

void AudioCodec::SetBit(uint8_t data[2], uint8_t bit, bool set)
{
    if (bit > 8)
    {
        // Just do nothing outside of bit register range
        return;
    }

    if (bit == 8)
    {
        // Clear the bit
        data[0] &= 0x7FU;

        // Set the bit
        data[0] = ((uint8_t)set) << 0x07U;
    }
    else
    {
        uint8_t mask = 0x01U << bit;

        // Clear the bit
        data[1] &= ~mask;

        // Set the bit
        data[1] |= mask;
    }
}