#pragma once

#include "constants.hh"
#include "stm32.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_i2s.h"

class AudioChip
{
public:
    AudioChip(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c);

    void Reset();
    bool Init();
    void StartI2S();
    void StopI2S();

    void DACVolumeSet(uint8_t volume);
    void DACVolumeAdjust(int16_t amount);
    uint8_t DACVolume() const;

    void ADCVolumeSet(uint8_t gain);
    void ADCVolumeAdjust(int16_t amount);
    uint8_t ADCVolume() const;

    void ISRCallback();
    void ClearTxBuffer();

    uint16_t* TxBuffer();
    const uint16_t* RxBuffer() const;

private:
    bool WriteRegister(uint8_t address, uint8_t value);
    bool WriteRegisterVerify(uint8_t address, uint8_t value);
    bool WriteRegistersVerify(const uint8_t (*registers)[2], const size_t len);
    int16_t ReadRegister(uint8_t address);

    bool PartialResetSequence();
    bool InitClockManager();
    bool InitSerialData();
    bool InitSystemPower();
    bool InitDACADC();
    bool InitGPIOPath();
    bool CompleteResetSequence();

    void LowPowerMode();

    I2S_HandleTypeDef* i2s;
    I2C_HandleTypeDef* i2c;

    uint16_t tx_buffer[constants::Total_Audio_Buffer_Sz] = {0};
    uint16_t* tx_ptr = tx_buffer;
    uint16_t rx_buffer[constants::Total_Audio_Buffer_Sz] = {0};
    uint16_t* rx_ptr = rx_buffer;
    uint16_t buff_modifier = false;

    uint8_t dac_volume = 0xBF;
    uint8_t adc_volume = 0xBF;

    // TODO move out into wave signal generator
    double phase;
    void SampleSineWave(uint16_t* buff,
                        const uint16_t num_samples,
                        const uint16_t start_idx,
                        const double amplitude,
                        const double freq,
                        double& phase,
                        const bool stereo);

    static constexpr uint16_t Es8311_I2c_Address = 0x18 << 1;

    static constexpr uint8_t reset_0x00 = 0x00;
    static constexpr uint8_t clock_manager_1_0x01 = 0x01;
    static constexpr uint8_t clock_manager_2_0x02 = 0x02;
    static constexpr uint8_t clock_manager_3_0x03 = 0x03;
    static constexpr uint8_t clock_manager_4_0x04 = 0x04;
    static constexpr uint8_t clock_manager_5_0x05 = 0x05;
    static constexpr uint8_t clock_manager_6_0x06 = 0x06;
    static constexpr uint8_t clock_manager_7_0x07 = 0x07;
    static constexpr uint8_t clock_manager_8_0x08 = 0x08;
    static constexpr uint8_t system_power_0x0c = 0x0c;
    static constexpr uint8_t system_power_2_0x0d = 0x0d;
    static constexpr uint8_t system_power_3_0x0e = 0x0e;
    static constexpr uint8_t system_power_4_0x0f = 0x0f;
    static constexpr uint8_t serial_data_port_1_0x09 = 0x09;
    static constexpr uint8_t serial_data_port_2_0x0a = 0x0a;
    static constexpr uint8_t dac_en_0x12 = 0x12;
    static constexpr uint8_t line_input_0x13 = 0x13;
    static constexpr uint8_t hp_dmic_0x14 = 0x14;
    static constexpr uint8_t adc_ramp_0x15 = 0x15;
    static constexpr uint8_t adc_power_0x16 = 0x16;
    static constexpr uint8_t adc_gain_0x17 = 0x17;
    static constexpr uint8_t dac_power_0x31 = 0x31;
    static constexpr uint8_t dac_volume_0x32 = 0x32;
    static constexpr uint8_t dac_output_0x37 = 0x37;
    static constexpr uint8_t dac_output_volume_0x38 = 0x38;
    static constexpr uint8_t dac_mixer_0x39 = 0x39;
    static constexpr uint8_t gpio_adc_dac_path_0x44 = 0x44;

    static constexpr uint8_t Min_DAC_Volume = 0x00;
    static constexpr uint8_t Max_DAC_Volume = 0xFF;
    static constexpr uint8_t Min_ADC_Volume = 0x00;
    static constexpr uint8_t Max_ADC_Volume = 0xFF;
};
