#include "audio_chip.hh"
#include "constants.hh"
#include "logger.hh"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"
#include <math.h>
#include <algorithm>
#include <cstdint>
#include <cstring>

AudioChip::AudioChip(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c) :
    i2s(&hi2s),
    i2c(&hi2c),
    hp_out_buffer{0},
    hp_out_ptr(hp_out_buffer),
    mic_in_buffer{0},
    mic_in_ptr{mic_in_buffer},
    buff_modifier(0),
    dac_volume(0xAF),
    adc_volume(0xE0),
    flags(0)
{
}

bool AudioChip::Init()
{
    Reset();

    PartialResetSequence();
    InitClockManager();
    InitSerialData();
    InitSystemPower();
    InitDACADC();
    InitGPIOPath();
    CompleteResetSequence();

    return true;
}

void AudioChip::Reset()
{
    // 0011'1111
    if (!WriteRegisterVerify(reset_0x00, 0x3F))
    {
        UI_LOG_ERROR("ES8311 failed to reest");
    }

    HAL_Delay(20);
}

void AudioChip::StartI2S()
{
    ClearHpOutBuffer();
    if (HAL_I2SEx_TransmitReceive_DMA(i2s, hp_out_buffer, mic_in_buffer,
                                      constants::Total_Audio_Buffer_Sz)
        != HAL_OK)
    {
        UI_LOG_ERROR("Failed to start I2S DMA");
        return;
    }

    RaiseFlag(AudioChip::Running);
}

void AudioChip::StopI2S()
{
    HAL_I2S_DMAStop(i2s);
    LowerFlag(AudioChip::Running);
}

void AudioChip::DACVolumeSet(uint8_t value)
{
    if (value >= Max_DAC_Volume)
    {
        value = Max_DAC_Volume;
    }

    if (value <= Min_DAC_Volume)
    {
        value = Min_DAC_Volume;
    }

    if (dac_volume == value)
    {
        return;
    }

    dac_volume = value;

    WriteRegisterVerify(dac_volume_0x32, value);
}

void AudioChip::DACVolumeAdjust(int16_t amount)
{
    DACVolumeSet(static_cast<int16_t>(dac_volume) + amount);
}

uint8_t AudioChip::DACVolume() const
{
    return dac_volume;
}

void AudioChip::ADCVolumeSet(uint8_t value)
{
    if (value >= Max_ADC_Volume)
    {
        value = Max_ADC_Volume;
    }

    if (value <= Min_ADC_Volume)
    {
        value = Min_ADC_Volume;
    }

    if (adc_volume == value)
    {
        return;
    }

    adc_volume = value;
    WriteRegisterVerify(adc_gain_0x17, adc_volume);
}

void AudioChip::ADCVolumeAdjust(int16_t amount)
{
    ADCVolumeSet(static_cast<int16_t>(adc_volume) + amount);
}

uint8_t AudioChip::ADCVolume() const
{
    return adc_volume;
}

void AudioChip::ISRCallback()
{
    const uint16_t offset = buff_modifier * constants::Audio_Buffer_Sz;
    hp_out_ptr = hp_out_buffer + offset;
    mic_in_ptr = mic_in_buffer + offset;
    buff_modifier = !buff_modifier;

    std::memset(hp_out_ptr, 0, constants::Audio_Buffer_Sz * sizeof(hp_out_ptr[0]));
}

void AudioChip::ClearHpOutBuffer()
{
    std::memset(hp_out_buffer, 0, sizeof(hp_out_buffer));
}

uint16_t* AudioChip::HpOutPtr()
{
    return hp_out_ptr;
}

const uint16_t* AudioChip::MicInPtr() const
{
    return mic_in_ptr;
}

bool AudioChip::WriteRegister(uint8_t address, uint8_t value)
{
    uint8_t message[] = {address, value};
    // UI_LOG_INFO("ES8311 register 0x%02x = 0x%02x", address, value);
    return HAL_I2C_Master_Transmit(i2c, Es8311_I2c_Address, message, sizeof(message), 100)
        == HAL_OK;
}

bool AudioChip::WriteRegisterVerify(uint8_t address, uint8_t value)
{
    uint8_t message[] = {address, value};
    // UI_LOG_INFO("ES8311 register 0x%02x = 0x%02x", address, value);
    if (HAL_I2C_Master_Transmit(i2c, Es8311_I2c_Address, message, sizeof(message), 100) != HAL_OK)
    {
        UI_LOG_ERROR("Failed to transmit to register %d value %d\n", (int)address, (int)value);
        return false;
    }

    const int16_t stored_value = ReadRegister(address);
    if (stored_value != value)
    {
        UI_LOG_ERROR("Failed to verify value of address to register %d expected %d actual %d\n",
                     (int)address, (int)value, (int)stored_value);
        return false;
    }

    return true;
}

bool AudioChip::WriteRegistersVerify(const uint8_t (*registers)[2], const size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        if (!WriteRegisterVerify(registers[i][0], registers[i][1]))
        {
            return false;
        }
        HAL_Delay(20);
    }
    return true;
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

bool AudioChip::PartialResetSequence()
{
    return WriteRegisterVerify(reset_0x00, 0b0010'0000);
}

bool AudioChip::InitClockManager()
{
    uint8_t lrclk_high = 0x00;
    uint8_t lrclk_low = 0x00;
    uint8_t bclk = 0x40;
    switch (constants::Sample_Rate)
    {
    case (constants::SampleRates::_8khz):
    {
        bclk |= 0x14;
        lrclk_high |= 0x05;
        lrclk_low |= 0xDB;
        break;
    }
    case (constants::SampleRates::_16khz):
    {
        bclk |= 0x0A;
        lrclk_high |= 0x02;
        lrclk_low |= 0xEE;
        break;
    }
    case (constants::SampleRates::_48khz):
    {
        bclk |= 0x02;
        lrclk_high |= 0x00;
        lrclk_low |= 0xF9;
        break;
    }
    }

    const uint8_t clock_manager[][2] = {
        {clock_manager_2_0x02, 0x98},       // DIG_MCLK 1001'1000 DIV4+1, MULT8 19.2Mhz
        {clock_manager_3_0x03, 0x10},       // ADC oversampling
        {clock_manager_4_0x04, 0x10},       // DAC oversampling
        {clock_manager_5_0x05, 0x00},       // ADC/DAC clk divider
        {clock_manager_6_0x06, bclk},       // BCLK
        {clock_manager_7_0x07, lrclk_high}, // LRCLK 48Mhz
        {clock_manager_8_0x08, lrclk_low},  // LRCLK 48Mhz
        {clock_manager_1_0x01, 0x3F},       // Enable clocks
    };

    return WriteRegistersVerify(clock_manager, sizeof(clock_manager) / sizeof(clock_manager[0]));
}

bool AudioChip::InitSerialData()
{
    const uint8_t serial_port[][2] = {
        {serial_data_port_1_0x09, 0x11}, // 0001'0001
        {serial_data_port_2_0x0a, 0x11}, // unmute, normal pol, 32 bit frame, i2s format
    };

    return WriteRegistersVerify(serial_port, sizeof(serial_port) / sizeof(serial_port[0]));
}

bool AudioChip::InitSystemPower()
{
    const uint8_t system_power[][2] = {
        {system_power_2_0x0d, 0x05},
        {system_power_2_0x0d, 0x06},
        {system_power_3_0x0e, 0x0a}, // 0b0000'1010
        {system_power_4_0x0f, 0x00},
    };

    return WriteRegistersVerify(system_power, sizeof(system_power) / sizeof(system_power[0]));
}

bool AudioChip::InitDACADC()
{
    const uint8_t dac_adc_config[][2] = {
        {line_input_0x13, 0x10},       // enable headphone drive
        {hp_dmic_0x14, 0x10},          //
        {adc_power_0x16, 0x04},        //
        {adc_gain_0x17, adc_volume},   //
        {dac_en_0x12, 0x01},           //
        {dac_power_0x31, 0x00},        //
        {dac_volume_0x32, dac_volume}, //
        {dac_output_0x37, 0x08},       // disable eq
    };

    return WriteRegistersVerify(dac_adc_config, sizeof(dac_adc_config) / sizeof(dac_adc_config[0]));
}

bool AudioChip::InitGPIOPath()
{
    const bool res = WriteRegisterVerify(gpio_adc_dac_path_0x44, 0x00);
    HAL_Delay(20);
    return res;
}

bool AudioChip::CompleteResetSequence()
{
    const bool res = WriteRegisterVerify(reset_0x00, 0xC0);
    HAL_Delay(20);
    return res;
}

void AudioChip::LowPowerMode()
{
}

bool AudioChip::ReadFlag(AudioFlag flag) const
{
    return (flags >> flag) & 0x01;
}

inline void AudioChip::RaiseFlag(AudioFlag flag)
{
    flags |= 1 << flag;
}

inline void AudioChip::LowerFlag(AudioFlag flag)
{
    flags &= ~(1 << flag);
}

inline bool AudioChip::ReadAndLowerFlag(AudioFlag flag)
{
    const bool res = (flags >> flag) & 0x01;
    LowerFlag(flag);
    return res;
}
