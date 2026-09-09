#include "audio_chip.hh"
#include "logger.hh"
#include <algorithm>
#include <cstring>

namespace
{
constexpr uint16_t Es8311_I2c_Address = 0x18 << 1;

constexpr uint8_t Reset = 0x00;
constexpr uint8_t Clock_Manager_1 = 0x01;
constexpr uint8_t Clock_Manager_2 = 0x02;
constexpr uint8_t Clock_Manager_3 = 0x03;
constexpr uint8_t Clock_Manager_4 = 0x04;
constexpr uint8_t Clock_Manager_5 = 0x05;
constexpr uint8_t Clock_Manager_6 = 0x06;
constexpr uint8_t Clock_Manager_7 = 0x07;
constexpr uint8_t Clock_Manager_8 = 0x08;
constexpr uint8_t System_Power = 0x0c;
constexpr uint8_t System_Power_2 = 0x0d;
constexpr uint8_t System_Power_3 = 0x0e;
constexpr uint8_t Serial_Data_Port_1 = 0x09;
constexpr uint8_t Serial_Data_Port_2 = 0x0a;
constexpr uint8_t Adc_Power = 0x16;
constexpr uint8_t Adc_Gain = 0x17;
constexpr uint8_t Dac_Power = 0x32;
constexpr uint8_t Dac_Volume = 0x32;
constexpr uint8_t Dac_Output = 0x37;
constexpr uint8_t Dac_Output_Volume = 0x38;
constexpr uint8_t Dac_Mixer = 0x39;
constexpr uint8_t Dac_Output_Power = 0x44;

constexpr uint8_t Min_Volume = 0x00;
constexpr uint8_t Max_Volume = 0xc0;
constexpr uint8_t Min_Mic_Preamp = 0x00;
constexpr uint8_t Max_Mic_Preamp = 0x1f;
} // namespace

AudioChip::AudioChip(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c) :
    i2s(&hi2s),
    i2c(&hi2c)
{
}

bool AudioChip::Init()
{
    HoldInReset();

    // The ES8311 receives a fixed 12 MHz MCLK. The codec PLL converts it for an 8 kHz sample rate.
    const uint8_t setup[][2] = {
        {Reset, 0x80},
        {Clock_Manager_1, 0x1C},
        {Clock_Manager_2, 0x00},
        {Clock_Manager_3, 0x17},
        {Clock_Manager_4, 0x17},
        {Clock_Manager_5, 0x00},
        {Clock_Manager_6, 0x00},
        {Clock_Manager_7, 0x00},
        {Clock_Manager_8, 0xff},
        {System_Power, 0x00},
        {System_Power_2, 0x00},
        {System_Power_3, 0x00},
        // I2S slave, Philips framing, 16-bit data.
        {Serial_Data_Port_1, 0x0c},
        {Serial_Data_Port_2, 0x00},
        {Adc_Power, 0x02},
        {Adc_Gain, mic_preamp},
        {Dac_Power, 0x00},
        {Dac_Output, 0x08},
        {Dac_Output_Volume, volume},
        {Dac_Mixer, 0x00},
        {Dac_Output_Power, 0x00},
    };

    UI_LOG_INFO("ES8311 starting writing registers");
    for (const auto& entry : setup)
    {
        if (!WriteRegister(entry[0], entry[1]))
        {
            UI_LOG_ERROR("ES8311 register 0x%02x write failed", entry[0]);
            return false;
        }

        UI_LOG_INFO("ES8311 register 0x%02x = 0x%02x", entry[0], entry[1]);
        HAL_Delay(20);
    }

    HAL_Delay(50);
    return true;
}

void AudioChip::HoldInReset()
{
    if (!WriteRegister(Reset, 0xBF))
    {
        UI_LOG_ERROR("ES8311 failed to reest");
    }

    HAL_Delay(50);
}

void AudioChip::StartI2S()
{
    ClearTxBuffer();
    if (HAL_I2SEx_TransmitReceive_DMA(i2s, tx_buffer, rx_buffer, constants::Total_Audio_Buffer_Sz)
        != HAL_OK)
    {
        UI_LOG_ERROR("Failed to start I2S DMA");
    }
}

void AudioChip::StopI2S()
{
    HAL_I2S_DMAStop(i2s);
}

void AudioChip::VolumeSet(int16_t value)
{
    volume = static_cast<uint8_t>(
        std::clamp(value, static_cast<int16_t>(Min_Volume), static_cast<int16_t>(Max_Volume)));
    WriteRegister(Dac_Output_Volume, volume);
}

void AudioChip::VolumeAdjust(int16_t amount)
{
    VolumeSet(static_cast<int16_t>(volume) + amount);
}

uint8_t AudioChip::Volume() const
{
    return volume;
}

void AudioChip::MicPreampSet(int16_t value)
{
    mic_preamp = static_cast<uint8_t>(std::clamp(value, static_cast<int16_t>(Min_Mic_Preamp),
                                                 static_cast<int16_t>(Max_Mic_Preamp)));
    WriteRegister(Adc_Gain, mic_preamp);
}

void AudioChip::MicPreampAdjust(int16_t amount)
{
    MicPreampSet(static_cast<int16_t>(mic_preamp) + amount);
}

uint8_t AudioChip::MicPreamp() const
{
    return mic_preamp;
}

void AudioChip::ISRCallback()
{
    const uint16_t offset = second_buffer ? constants::Audio_Buffer_Sz : 0;
    tx_ptr = tx_buffer + offset;
    rx_ptr = rx_buffer + offset;
    second_buffer = !second_buffer;
}

void AudioChip::ClearTxBuffer()
{
    std::memset(tx_buffer, 0, sizeof(tx_buffer));
}

uint16_t* AudioChip::TxBuffer()
{
    return tx_ptr;
}

const uint16_t* AudioChip::RxBuffer() const
{
    return rx_ptr;
}

bool AudioChip::WriteRegister(uint8_t address, uint8_t value)
{
    uint8_t message[] = {address, value};
    return HAL_I2C_Master_Transmit(i2c, Es8311_I2c_Address, message, sizeof(message), 100)
        == HAL_OK;
}
