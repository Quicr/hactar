// In this order
// Get i2s flowing both ways between devices
// use all synthetic data like ramp
//
// Audio out path - get it play a buzz, watch on scope
//
// Audio in path - ADC
//
// sine wave/sawtooth from signal gen
// turn off all equalizers
// try to get the gains right.
#include "audio_chip.hh"
#include "constants.hh"
#include "logger.hh"
#include "stm32f4xx_hal_i2c.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace
{
constexpr uint16_t Es8311_I2c_Address = 0x18 << 1;

constexpr uint8_t reset_0x00 = 0x00;
constexpr uint8_t clock_manager_1_0x01 = 0x01;
constexpr uint8_t clock_manager_2_0x02 = 0x02;
constexpr uint8_t clock_manager_3_0x03 = 0x03;
constexpr uint8_t clock_manager_4_0x04 = 0x04;
constexpr uint8_t clock_manager_5_0x05 = 0x05;
constexpr uint8_t clock_manager_6_0x06 = 0x06;
constexpr uint8_t clock_manager_7_0x07 = 0x07;
constexpr uint8_t clock_manager_8_0x08 = 0x08;
constexpr uint8_t system_power_0x0c = 0x0c;
constexpr uint8_t system_power_2_0x0d = 0x0d;
constexpr uint8_t system_power_3_0x0e = 0x0e;
constexpr uint8_t serial_data_port_1_0x09 = 0x09;
constexpr uint8_t serial_data_port_2_0x0a = 0x0a;
constexpr uint8_t system_dac_en_0x12 = 0x12;
constexpr uint8_t system_line_input_0x13 = 0x13;
constexpr uint8_t system_hp_dmic_0x14 = 0x14;
constexpr uint8_t adc_ramp_0x15 = 0x15;
constexpr uint8_t adc_power_0x16 = 0x16;
constexpr uint8_t adc_gain_0x17 = 0x17;
constexpr uint8_t dac_power_0x31 = 0x31;
constexpr uint8_t dac_volume_0x32 = 0x32;
constexpr uint8_t dac_output_0x37 = 0x37;
constexpr uint8_t dac_output_volume_0x38 = 0x38;
constexpr uint8_t dac_mixer_0x39 = 0x39;
constexpr uint8_t gpio_adc_dac_path_0x44 = 0x44;

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

void AudioChip::BootupSequence()
{
    Init();
    HAL_Delay(10);
    Reset();
    HAL_Delay(10);
    Init();
    HAL_Delay(10);
    Reset();
    HAL_Delay(10);
    Init();
}
// NOTE- there is an internal loopback on register 0x44 ADC-DAC

bool AudioChip::Init()
{
    Reset();

    // The ES8311 receives a fixed 12 MHz MCLK. The codec PLL converts it for an 8 kHz sample rate.
    const uint8_t setup[][2] = {
        {reset_0x00, 0b0110'0000},
        {clock_manager_2_0x02, 0x98}, // DIG_MCLK 1001'1000 DIV4+1, MULT8 19.2Mhz
        {clock_manager_3_0x03, 0x19}, // ADC oversampling
        {clock_manager_4_0x04, 0x19}, // DAC oversampling
        {clock_manager_5_0x05, 0x00},
        {clock_manager_6_0x06, 0x42},
        {clock_manager_7_0x07, 0x00}, // LRCLK 48Mhz
        {clock_manager_8_0x08, 0xF9}, // LRCLK 48Mhz
        {clock_manager_1_0x01, 0x3F},
        {serial_data_port_1_0x09, 0x11}, // 0001'0000
        {serial_data_port_2_0x0a, 0x11}, // unmute, normal pol, 32 bit frame, i2s format
        {system_power_0x0c, 0x00},
        {dac_power_0x31, 0x60},
        {dac_volume_0x32, 0x00},
        {dac_output_0x37, 0x08},               // disable eq
        {gpio_adc_dac_path_0x44, 0b0110'0000}, // ADC->DAC loopback disabled, filled both channels
        {reset_0x00, 0xC0},
        {system_power_2_0x0d, 0x01},
        {system_power_3_0x0e, 0x02}, // 0b0000'0010
        {system_dac_en_0x12, 0x00},
        {system_line_input_0x13, 0x10}, // enable headphone drive
                                        // End startup seq

        {system_hp_dmic_0x14, 0x50}, // enable headphone drive
        {adc_power_0x16, 0x04},
        {adc_gain_0x17, mic_preamp},
    };

    UI_LOG_INFO("ES8311 starting writing registers");
    for (const auto& entry : setup)
    {
        if (!WriteRegister(entry[0], entry[1]))
        {
            UI_LOG_ERROR("ES8311 register 0x%02x write failed", entry[0]);
            return false;
        }

        // UI_LOG_INFO("ES8311 register 0x%02x = 0x%02x", entry[0], entry[1]);
        HAL_Delay(20);
    }

    HAL_Delay(20);
    WriteRegister(dac_power_0x31, 0x00);
    HAL_Delay(20);
    WriteRegister(dac_volume_0x32, 0xFF);

    HAL_Delay(100);
    return true;
}

void AudioChip::Reset()
{
    // 0011'1111
    if (!WriteRegister(reset_0x00, 0x3F))
    {
        UI_LOG_ERROR("ES8311 failed to reest");
    }

    HAL_Delay(1000);
}

void AudioChip::Boot()
{
    // 0011'1111HoldInReset
    if (!WriteRegister(reset_0x00, 0xC0))
    {
        UI_LOG_ERROR("ES8311 failed to boot");
    }

    HAL_Delay(200);
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
    WriteRegister(dac_output_volume_0x38, volume);
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
    WriteRegister(adc_gain_0x17, mic_preamp);
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
    const uint16_t offset = buff_modifier * constants::Audio_Buffer_Sz;
    tx_ptr = tx_buffer + offset;
    rx_ptr = rx_buffer + offset;
    buff_modifier = !buff_modifier;
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
    // UI_LOG_INFO("ES8311 register 0x%02x = 0x%02x", address, value);
    return HAL_I2C_Master_Transmit(i2c, Es8311_I2c_Address, message, sizeof(message), 100)
        == HAL_OK;
}

int16_t AudioChip::ReadRegister(uint8_t address)
{
    uint8_t message = address;
    // UI_LOG_INFO("ES8311 register 0x%02x = 0x%02x", address, value);
    if (HAL_I2C_Master_Transmit(i2c, Es8311_I2c_Address, &message, sizeof(message), 100) != HAL_OK)
    {
        return -1;
    }

    // UI_LOG_INFO("ES8311 register 0x%02x = 0x%02x", address, value);
    if (HAL_I2C_Master_Receive(i2c, Es8311_I2c_Address, &message, sizeof(message), 100) != HAL_OK)
    {
        return -1;
    }

    return static_cast<int16_t>(message);
}
