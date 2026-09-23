#pragma once

#include "constants.hh"
#include "stm32.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_i2s.h"

class AudioChip
{
public:
    AudioChip(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c);

    void BootupSequence();
    void Reset();
    bool Init();
    void StartI2S();
    void StopI2S();

    void VolumeSet(int16_t volume);
    void VolumeAdjust(int16_t amount);
    uint8_t Volume() const;

    void MicPreampSet(int16_t gain);
    void MicPreampAdjust(int16_t amount);
    uint8_t MicPreamp() const;

    void ISRCallback();
    void ClearTxBuffer();

    uint16_t* TxBuffer();
    const uint16_t* RxBuffer() const;

private:
    void Boot();
    bool WriteRegister(uint8_t address, uint8_t value);
    int16_t ReadRegister(uint8_t address);

    I2S_HandleTypeDef* i2s;
    I2C_HandleTypeDef* i2c;

    uint16_t tx_buffer[constants::Total_Audio_Buffer_Sz] = {0};
    uint16_t* tx_ptr = tx_buffer;
    uint16_t rx_buffer[constants::Total_Audio_Buffer_Sz] = {0};
    uint16_t* rx_ptr = rx_buffer;
    uint16_t buff_modifier = false;

    uint8_t volume = 0xc0;
    uint8_t mic_preamp = 0x10;

    double phase;
    void SampleSineWave(uint16_t* buff,
                        const uint16_t num_samples,
                        const uint16_t start_idx,
                        const double amplitude,
                        const double freq,
                        double& phase,
                        const bool stereo);
};
