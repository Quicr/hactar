#include "AudioCodec.hh"

// NOTE MCLK = 12Mhz
#include <cmath>


extern UART_HandleTypeDef huart1;

void DebugPins(int output)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PinState((output & 0x01) >> 0));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PinState((output & 0x02) >> 1));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PinState((output & 0x04) >> 2));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PinState((output & 0x08) >> 3));
}


AudioCodec::AudioCodec(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c):
    i2s(&hi2s), i2c(&hi2c), rx_buffer{ 0 }, rx_busy(false)
{
    // Reset the wm8960
    SetRegister(0x0F, 0b0'0000'0000);
    HAL_Delay(100);

    // Set the power
    SetRegister(0x19, 0b1'1111'1110);

    // Enable outputs
    SetRegister(0x1A, 0b1'1110'0001);

    // Enable lr mixer ctrl
    // SetRegister(0x2F, 0b0'0000'0000);
    SetRegister(0x2F, 0b0'0000'1100);

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
    // Pg 64 version
    // SetRegister(0x35, 0b0'0000'1100);
    // SetRegister(0x36, 0b0'1001'0011);
    // SetRegister(0x37, 0b0'1110'1001);

    // Pg 85 version
    SetRegister(0x35, 0b0'0011'0001);
    SetRegister(0x36, 0b0'0010'0110);
    SetRegister(0x37, 0b0'1110'1001);

    // TODO verify math
        // Set ADCDIV to get 16Khz from SYSCLK (0x03)
        // Set DACDIV to get 16Khz from SYSCLK (0x03)
        // Post scale the PLL to be divided by 2
        // Set the clock (Select the PLL) (0x01)
    SetRegister(0x04, 0b0'1101'1101);

    // Set the clock division
    // D_Clock = sysclk / 16 = 12Mhz / 16 = 0.768Mhz
    // BCLKDIV = SYSCLK / 6 = 2.048Mhz
    SetRegister(0x08, 0b1'1100'0110);

    // Disable soft mute and ADC high pass filter
    SetRegister(0x05, 0b0'0000'0000);

    // Set the Master mode (1), I2S to 16 bit words
    // Set audio data format to i2s mode
    SetRegister(0x07, 0b0'0100'0000);

    // Set the left and right headphone volumes
    SetRegister(0x02, 0b1'0111'1111);
    SetRegister(0x03, 0b1'0111'1111);

    // Set the left and right speaker volumes
    // SetRegister(0x28, 0b1'0111'1111);
    // SetRegister(0x29, 0b1'0111'1111);

    // Enable the outputs
    SetRegister(0x31, 0b0'0111'0111);

    // Set DAC left and right volumes
    SetRegister(0x0A, 0b1'1111'1111);
    SetRegister(0x0B, 0b1'1111'1111);

    // Set left and right mixer
    SetRegister(0x22, 0b1'0000'0000);
    SetRegister(0x25, 0b1'0000'0000);

    // Change the ALC sample rate -> 16K
    SetRegister(0x1B, 0b0'0000'0011);


    for (int i = 0 ; i <= Max_Address; ++i)
    {
        PrintRegisterData(i);
        HAL_Delay(10);
    }
}

// TODO read the I2C value to make sure it is writing correctly!
bool AudioCodec::TestRegister()
{
    // return SetRegister(0x34, 0b0'0001'1000);

    return SetRegister(0x0A, 0b1'1111'1111);
}

AudioCodec::~AudioCodec()
{
    i2c = nullptr;
}

HAL_StatusTypeDef AudioCodec::WriteRegister(uint8_t address)
{
    HAL_StatusTypeDef result = HAL_I2C_Master_Transmit(i2c,
        Write_Condition, registers[address].bytes, 2, HAL_MAX_DELAY);

    HAL_Delay(2);

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
    registers[address].bytes[0] |= uint8_t((data >> 8) & Top_Bit_Mask);
    registers[address].bytes[1] = uint8_t(data & 0x00FF);

    const uint8_t end[2] = { '\n','\n' };
    const uint8_t test[4] = { '1', '0', '1', '\n' };
    // HAL_UART_Transmit(&huart1, test, 4, HAL_MAX_DELAY);
    // HAL_UART_Transmit(&huart1, ToBinaryString(&registers[address].addr, 1), 9, HAL_MAX_DELAY);
    // HAL_UART_Transmit(&huart1, ToBinaryString(&registers[address].bytes[0], 1), 9, HAL_MAX_DELAY);
    // HAL_UART_Transmit(&huart1, ToBinaryString(&registers[address].bytes[1], 1), 9, HAL_MAX_DELAY);
    // HAL_UART_Transmit(&huart1, end, 2, HAL_MAX_DELAY);

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
    registers[address].bytes[0] ^= uint8_t((data >> 8) & Top_Bit_Mask);
    registers[address].bytes[1] ^= uint8_t(data & 0x00FF);

    return (WriteRegister(address) == HAL_OK);
}

bool AudioCodec::ReadRegister(uint8_t address, uint16_t& value)
{
    if (address > Max_Address)
    {
        return false;
    }

    value = (uint16_t)((registers[address].bytes[0] & 0x0001) << 8);
    value += registers[address].bytes[1];
    return true;
}

void AudioCodec::RxAudio()
{
    if (rx_busy)
    {
        return;
    }

    rx_busy = true;

    // TODO Rx_Buffer_Sz might need to be in bytes not element sz.
    auto output = HAL_I2S_Receive_DMA(i2s, rx_buffer, Rx_Buffer_Sz * 2);
}

void AudioCodec::RxComplete()
{
    rx_busy = false;
}

void AudioCodec::Send1KHzSignal()
{
    const float freq = 1000.0;
    const float amplitude = 0.5;
    const float PI = 3.14159265358979311599796346854;

    uint16_t sample_rate[1] = { static_cast<uint16_t>(32767.0 * std::sin(2 * PI * freq * HAL_GetTick() / 1000.0)) };
    // uint16_t sample = static_cast<uint16_t>(32767.0 * sin(2 * 3.141592653589793 * 1000.0 * HAL_GetTick() / 1000.0))

    HAL_I2S_Transmit_DMA(i2s, sample_rate, 1);

    // for (int i = 0; i < sample_rate; ++i)
    // {
    //     float value = amplitude * std::sin((2 * PI * i) / sample_rate);
    //     int16_t sample = static_cast<int16_t>(value * 32767);

    //     HAL_I2S_
    // }
}

void AudioCodec::SendAllOnes()
{
    uint32_t sz = 1;
    uint16_t sample_data[sz];
    for (int i = 0; i < sz; ++i)
    {
        sample_data[i] = 0xFFFF;
    }

    HAL_I2S_Transmit(i2s, sample_data, sz, HAL_MAX_DELAY);
}

void AudioCodec::PrintRegisterData(const uint8_t addr)
{
    const uint8_t newline [] = "\n";
    uint8_t num_str[64]{};
    uint16_t len = 0;
    int_to_string(addr, num_str, len);
    HAL_UART_Transmit(&huart1, num_str, len, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, ToBinaryString(&registers[addr].bytes[0], 1), 9, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, ToBinaryString(&registers[addr].bytes[1], 1), 9, HAL_MAX_DELAY);

    HAL_UART_Transmit(&huart1, newline, 1, HAL_MAX_DELAY);

}