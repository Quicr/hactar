#include "AudioCodec.hh"

// NOTE MCLK = 12Mhz
#include "app_main.hh"

#include <cmath>

void DebugPins(int output)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PinState((output & 0x01) >> 0));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PinState((output & 0x02) >> 1));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PinState((output & 0x04) >> 2));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PinState((output & 0x08) >> 3));
}


AudioCodec::AudioCodec(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c) :
    i2s(&hi2s), i2c(&hi2c), rx_buffer{0}, rx_busy(false)
{
    // Reset the chip
    SetRegister(0x2F, 0x0000);

    // Set the power
    SetRegister(0x19, 0b1'1100'0000);

    // Enable lr mixer ctrl
    SetRegister(0x2F, 0b0'0000'1100);

    // Set the clock division
    // D_Clock = sysclk / 16 = 12Mhz / 16 = 0.768Mhz
    // BCLKDIV = SYSCLK / 6 = 2.048Mhz
    SetRegister(0x08, 0b1'1100'0110);

    // Set ADCDIV to get 16Khz from SYSCLK (0x03)
    // Set DACDIV to get 16Khz from SYSCLK (0x03)
    // Post scale the PLL to be divided by 2
    // Set the clock (Select the PLL) (0x01)
    SetRegister(0x04, 0b0'1101'1101);

    // Enable PLL integer mode.
    /** MCLK = 24MHz / 12, ReqCLK = 12.288MHz
    *   5 < PLLN < 13
    *   int R = f2 / 12
    *   PLLN = int R.
    *   f2 = 2 * 2 * ReqCLK = 98.304Mhz
    *   R = 98.304 / 12 = 8.192
    *   int R = 0x8
    *   K = int(2^24 * (R - PLLN))
    *     = int(2^24 * (8.192 - 8))
    *     = 3221225
    *     = 0x3126E9 -> but the table says 0x3126E8
    *                                       = 0b0011'0001'0010'0110'1110'1001
    **/
    SetRegister(0x34, 0b0'0001'1000);

    // Write the fractional K into the registers
    SetRegister(0x35, 0b0'0000'1100);
    SetRegister(0x36, 0b0'1001'0011);
    SetRegister(0x37, 0b0'1110'1001);

    // Disable soft mute and ADC high pass filter
    SetRegister(0x05, 0b0'0000'0000);

    // Set the Master mode (1), I2S to 16 bit words
    // Set audio data format to i1s mode
    SetRegister(0x07, 0b0'0100'0010);

    // Set the left and right headphone volumes
    SetRegister(0x02, 0b1'0111'1111);
    SetRegister(0x03, 0b1'0111'1111);

    // Set the left and right speaker volumes
    SetRegister(0x28, 0b1'0111'1111);
    SetRegister(0x29, 0b1'0111'1111);

    // TODO verify these registers
    // Enable the outputs
    SetRegister(0x31, 0b0'1111'0111);

    // Set DAC left and right volumes
    SetRegister(0x0A, 0b1'0110'1111);
    SetRegister(0x0B, 0b1'0110'1111);

    // Set left and right mixer
    SetRegister(0x22, 0b1'1000'0000);
    SetRegister(0x25, 0b1'1000'0000);

    // Enable outputs
    SetRegister(0x1A, 0b1'1111'1001);

}

// TODO read the I2C value to make sure it is writing correctly!
bool AudioCodec::TestRegister()
{
    return SetRegister(0x34, 0b0'0001'1000);
}

AudioCodec::~AudioCodec()
{
    i2c = nullptr;
}

HAL_StatusTypeDef AudioCodec::WriteRegister(uint8_t address)
{
    HAL_StatusTypeDef result = HAL_I2C_Master_Transmit(i2c,
        Write_Condition, registers[address].bytes, 2, HAL_MAX_DELAY);

    return result;
}

bool AudioCodec::SetRegister(uint8_t address, uint16_t data)
{
    if (address > Max_Address)
    {
        return false;
    }

    // Reset the top bit
    registers[address].bytes[0] &= ~Top_Bit_Mask;

    // Set the register data
    registers[address].bytes[0] |= uint8_t((data >> 1) & Top_Bit_Mask);
    registers[address].bytes[1] = uint8_t(data & 0x00FF);

    // Write the register to the chip
    return (WriteRegister(address) == HAL_OK);
}

bool AudioCodec::XorRegister(uint8_t address, uint16_t data)
{
    if (address > Max_Address)
    {
        return false;
    }

    // Update the register data
    registers[address].bytes[0] ^= uint8_t((data >> 1) & Top_Bit_Mask);
    registers[address].bytes[1] ^= uint8_t(data & 0x00FF);

    return (WriteRegister(address) == HAL_OK);
}

bool AudioCodec::ReadRegister(uint8_t address, uint16_t& value)
{
    if (address > Max_Address)
    {
        return false;
    }

    // Deep magic.
    uint8_t* bytes = registers[address].bytes;
    // TODO unionize this
    value = (*(reinterpret_cast<uint16_t*>(bytes))) & Data_Mask;
    return true;
}

void AudioCodec::RxAudio(Screen* screen)
{
    if (rx_busy)
        return;
    // We need this because if the mic stays on it will write to USART3 in
    // bootloader mode which locks up the main chip
    // TODO remove enable mic bits [2,3]
    XorRegister(0x19, 0b0'0000'1100);

    rx_busy = true;

    // TODO Rx_Buffer_Sz might need to be in bytes not element sz.
    auto output = HAL_I2S_Receive_DMA(i2s, rx_buffer, Rx_Buffer_Sz*2);
    screen->DrawText(0, 100, String::int_to_string((int)output), font7x12, C_GREEN, C_BLACK);

}

void AudioCodec::RxComplete()
{
    rx_busy = false;

    // We need this because if the mic stays on it will write to USART3 in
    // bootloader mode which locks up the main chip
    // TODO remove disable mic bits [2,3]
    XorRegister(0x19, 0b0'0000'1100);
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