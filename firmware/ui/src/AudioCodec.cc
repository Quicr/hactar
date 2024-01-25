#include "AudioCodec.hh"

// NOTE MCLK = 12Mhz
#include "main.hh"

#include <cmath>

void DebugPins(int output)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PinState((output & 0x01) >> 0));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PinState((output & 0x02) >> 1));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PinState((output & 0x04) >> 2));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PinState((output & 0x08) >> 3));
}


AudioCodec::AudioCodec(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c) :
    i2s(&hi2s), i2c(&hi2c)
{
    // Reset the chip
    WriteRegisterSeries(0x2F, 0x0000, 1);

    // Set the power
    WriteRegisterSeries(0x19, 0b111000000, 2);

    // Add 0x0001 for pll
    WriteRegisterSeries(0x1A, 0b111111001, 3);

    // Enable lr mixer ctrl
    WriteRegisterSeries(0x2F, 0b000001100, 4);

    // Set the clock division
    // D_Clock = sysclk / 16 = 12Mhz / 16
    WriteRegisterSeries(0x08, 0b111000001, 5);

    // Set the clock (Select the PLL)
    WriteRegisterSeries(0x04, 0b000000001, 6);

    // Enable PLL integer mode.
    /** MCLK = 12MHz, ReqCLK = 12.288MHz
    *   5 < PLLN < 13
    *   int R = f2 / 12
    *   PLLN = int R.
    *   f2 = 4 * 2 * ReqCLK = 94.304Mhz
    *   R = 98.304 / 12 = 8.192
    *   int R = 8.
    *   K = int(2^24 * (R - PLLN))
    *     = int(2^24 * (8.192 - 8))
    *     = 3221225
    *     = 0x3126E9
    **/
    WriteRegisterSeries(0x34, 0b000101000, 7);

    // Write the fractional K into the registers
    WriteRegisterSeries(0x35, 0b000110001, 8);
    WriteRegisterSeries(0x36, 0b000100110, 9);
    WriteRegisterSeries(0x37, 0b011101001, 10);

    // Disable soft mute and ADC high pass filter
    WriteRegisterSeries(0x05, 0b000000000, 11);

    // Set the Master mode (1) I2S to 16 bit words
    // TODO maybe use 24 bit words
    WriteRegisterSeries(0x07, 0b0'0100'0010, 12);

    // Set the left and right headphone volumes
    WriteRegisterSeries(0x02, 0b101101111, 13);
    WriteRegisterSeries(0x03, 0b101101111, 14);

    // Set the left and right speaker volumes
    WriteRegisterSeries(0x28, 0b101101111, 15);
    WriteRegisterSeries(0x29, 0b101101111, 1);

    // Enable the outputs
    WriteRegisterSeries(0x31, 0b011110111, 2);

    // Set DAC left and right volumes
    WriteRegisterSeries(0x0A, 0b101101111, 3);
    WriteRegisterSeries(0x0B, 0b101101111, 4);

    // Set left and right mixer
    WriteRegisterSeries(0x22, 0b110000000, 5);
    WriteRegisterSeries(0x25, 0b110000000, 15);

}

AudioCodec::~AudioCodec()
{
    i2c = nullptr;
}

HAL_StatusTypeDef AudioCodec::WriteRegisterToCodec(uint8_t address)
{
    // const uint8_t read_condition = 0x34 + 1;
    HAL_StatusTypeDef audio_select = HAL_I2C_Master_Transmit(i2c,
        Write_Condition, registers[address].bytes, 2, HAL_MAX_DELAY);

    return audio_select;
}

bool AudioCodec::WriteRegister(uint8_t address, uint16_t data)
{
    if (address > Max_Address)
    {
        return false;
    }
    // Update the data
    registers[address].bytes[0] |= uint8_t((data >> 1) & Top_Bit_Mask);
    registers[address].bytes[1] = uint8_t(data & 0x00FF);

    // Write the register to the chip
    return (WriteRegisterToCodec(address) == HAL_OK);
}

bool AudioCodec::WriteRegisterSeries(uint8_t address, uint16_t data, uint8_t debug)
{
    bool res = WriteRegister(address, data);
    HAL_Delay(10);

    if (res == false)
    {
        DebugPins(0);
    }
    else
    {
        DebugPins(debug);
    }

    return res;
}

bool AudioCodec::ReadRegister(uint8_t address, uint16_t& value)
{
    if (address > Max_Address)
    {
        return false;
    }

    // Deep magic.
    uint8_t* bytes = registers[address].bytes;
    value = (*(reinterpret_cast<uint16_t*>(bytes))) & Data_Mask;
    return true;
}

void AudioCodec::Send1KHzSignal()
{
    const float freq = 1000.0;
    const float amplitude = 0.5;
    const float PI = 3.14159265358979311599796346854;

    uint16_t sample_rate[1] = {static_cast<uint16_t>(32767.0 * std::sin(2 * PI * freq * HAL_GetTick() / 1000.0))};
    // uint16_t sample = static_cast<uint16_t>(32767.0 * sin(2 * 3.141592653589793 * 1000.0 * HAL_GetTick() / 1000.0))

    HAL_I2S_Transmit(i2s, sample_rate, 1, HAL_MAX_DELAY);

    // for (int i = 0; i < sample_rate; ++i)
    // {
    //     float value = amplitude * std::sin((2 * PI * i) / sample_rate);
    //     int16_t sample = static_cast<int16_t>(value * 32767);

    //     HAL_I2S_
    // }
}