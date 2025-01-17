#pragma once

#include "stm32.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_i2s.h"

#include "audio_codec.hh"

#include "memory"

extern UART_HandleTypeDef huart1;

class AudioChip
{
private:
    typedef union register_t
    {
        uint8_t addr;
        uint8_t bytes[2];

        register_t(uint8_t top, uint8_t bottom): bytes{ top, bottom } {}
    } Register;

public:
    AudioChip(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c);
    ~AudioChip();

    void Init();
    void Reset();

    bool SetRegister(uint8_t address, uint16_t data);
    bool OrRegister(uint8_t address, uint16_t data);
    bool XorRegister(uint8_t address, uint16_t data);
    bool SetBit(uint8_t address, uint8_t bit, uint8_t set);
    bool SetBits(const uint8_t address, const uint16_t bits, const uint16_t set);

    bool ReadRegister(uint8_t address, uint16_t& value);

    void StartI2S();
    void StopI2S();

    void GetAudio(uint16_t* buffer, uint16_t size);

    void TurnOnLeftInput3();
    void TurnOffLeftInput3();
    void TurnOnLeftDifferentialInput();
    void TurnOffLeftDifferentialInput();

    void EnableLeftMicPGA();
    void DisableLeftMicPGA();

    void MuteMic();
    void UnmuteMic();

    bool DataAvailable();

    void HalfCompleteCallback();
    void CompleteCallback();
    void SampleSineWave(uint16_t* buff, const uint16_t num_samples,
        uint16_t start_idx, uint16_t sample_rate,
        float amplitutde, float freq, float& phase, bool stereo);
    void SampleHarmonic(uint16_t* buff, const uint16_t num_samples,
        uint16_t start_idx, uint16_t sample_rate,
        float amplitutde, float freqs[], float phases[],
        const uint16_t num_freqs, bool stereo);
    void SendSawToothWave();

    float phase;
    float phases[3] = {0, 0, 0};


    static constexpr uint16_t Sample_Rate = 16'000; // 16khz

    static uint8_t* ToBinaryString(const uint8_t* data, size_t size)
    {
        uint8_t* bin_string = new uint8_t[(size * 8) + 1] { 0 };

        int bit = 0;

        for (int i = 0; i < size; ++i)
        {
            uint8_t byte = data[i];
            bit = 0;
            for (int j = 7; j >= 0; --j)
            {
                bin_string[(i * 8) + bit] = ((byte >> j) & 1) + '0';
                bit++;
            }
        }
        bin_string[(size * 8)] = '\n'; // Null-terminate the string

        return bin_string;
    }

    static void int_to_string(const unsigned long long input, uint8_t* str, uint16_t& size)
    {
        unsigned long long value = input;
        // Get the num of powers
        unsigned long long tmp = value;
        unsigned long long sz = 0;
        do
        {
            sz++;
            tmp = tmp / 10;
        } while (tmp > 0);
        //159 -> 15 sz 1
        //15  -> 1 sz 2
        //1   -> 0 sz 3

        size = sz + 1;

        str[sz] = '\n';
        unsigned long long mod = 0;
        do
        {
            mod = value % 10;
            str[sz-1] = '0' + (char)mod;
            sz--;
            value = value / 10;
        } while (value > 0);

    }

    static void PrintInt(const unsigned long input)
    {

        const uint8_t newline [] = "\n";
        uint8_t num_str[64]{};
        uint16_t len = 0;
        int_to_string(input, num_str, len);
        HAL_UART_Transmit(&huart1, num_str, len, HAL_MAX_DELAY);
    }

private:
    void PrintRegisterData(const uint8_t addr);

    HAL_StatusTypeDef WriteRegister(uint8_t address);

    static constexpr uint16_t Write_Condition = 0x34;

    static constexpr uint8_t Num_Registers = 56;
    static constexpr uint8_t Addr_Mask = 0xFE;
    static constexpr uint16_t Top_Bit_Mask = 0x0001;
    static constexpr uint16_t Bot_Bit_Mask = 0x00FF;
    static constexpr uint16_t Data_Mask = 0x01FF;
    static constexpr uint8_t Max_Address = 0x37;

    static constexpr uint16_t Audio_Buffer_Sz = 256;

    I2S_HandleTypeDef* i2s;
    I2C_HandleTypeDef* i2c;
    uint16_t tx_buffer[Audio_Buffer_Sz];
    size_t tx_buff_idx;
    uint16_t rx_buffer[Audio_Buffer_Sz];


    AudioCodec codec;
    uint8_t byte_buff[Audio_Buffer_Sz];

    bool data_available;
    bool stereo;

    static constexpr uint16_t running_flag = 0x00;
    static constexpr uint16_t half_complete_flag = 0x01;
    static constexpr uint16_t complete_flag = 0x02;
    static constexpr uint16_t stereo_flag = 0x03;
    static constexpr uint16_t mic_mute_flag = 0x04;

    uint16_t flags = 0x00;

    Register registers[Max_Address + 1] = {
        { Register(0x00, 0xa7) }, // 0x00 - 0000 0000, 1010 0111
        { Register(0x02, 0xa7) }, // 0x01 - 0000 0010, 1010 0111
        { Register(0x04, 0x00) }, // 0x02 - 0000 0100, 0000 0000
        { Register(0x06, 0x00) }, // 0x03 - 0000 0110, 0000 0000
        { Register(0x08, 0x00) }, // 0x04 - 0000 1000, 0000 0000
        { Register(0x0a, 0x08) }, // 0x05 - 0000 1010, 0000 1000
        { Register(0x0c, 0x00) }, // 0x06
        { Register(0x0e, 0x0a) }, // 0x07
        { Register(0x11, 0xc0) }, // 0x08
        { Register(0x12, 0x00) }, // 0x09
        { Register(0x14, 0xff) }, // 0x0a
        { Register(0x16, 0xff) }, // 0x0b
        { Register(0x18, 0x00) }, // 0x0c - Reserved
        { Register(0x1a, 0x00) }, // 0x0d - Reserved
        { Register(0x1c, 0x00) }, // 0x0e - Reserved
        { Register(0x1e, 0x00) }, // 0x0f
        { Register(0x20, 0x00) }, // 0x10
        { Register(0x22, 0x7b) }, // 0x11
        { Register(0x24, 0x00) }, // 0x12
        { Register(0x26, 0x32) }, // 0x13
        { Register(0x28, 0x00) }, // 0x14
        { Register(0x2a, 0xc3) }, // 0x15
        { Register(0x2c, 0xc3) }, // 0x16
        { Register(0x2f, 0xc0) }, // 0x17
        { Register(0x30, 0x00) }, // 0x18
        { Register(0x32, 0x00) }, // 0x19
        { Register(0x34, 0x00) }, // 0x1a
        { Register(0x36, 0x00) }, // 0x1b
        { Register(0x38, 0x00) }, // 0x1c
        { Register(0x3a, 0x00) }, // 0x1d
        { Register(0x3c, 0x00) }, // 0x1e - Reserved
        { Register(0x3e, 0x00) }, // 0x1f - Reserved
        { Register(0x41, 0x00) }, // 0x20
        { Register(0x43, 0x00) }, // 0x21
        { Register(0x44, 0x50) }, // 0x22
        { Register(0x46, 0x00) }, // 0x23
        { Register(0x48, 0x00) }, // 0x24
        { Register(0x4a, 0x50) }, // 0x25
        { Register(0x4c, 0x00) }, // 0x26
        { Register(0x4e, 0x00) }, // 0x27
        { Register(0x50, 0x00) }, // 0x28
        { Register(0x52, 0x00) }, // 0x29
        { Register(0x54, 0x40) }, // 0x2a
        { Register(0x56, 0x00) }, // 0x2b
        { Register(0x58, 0x00) }, // 0x2c
        { Register(0x5a, 0x50) }, // 0x2d
        { Register(0x5c, 0x50) }, // 0x2e
        { Register(0x5e, 0x00) }, // 0x2f
        { Register(0x60, 0x02) }, // 0x30
        { Register(0x62, 0x37) }, // 0x31
        { Register(0x64, 0x4c) }, // 0x32 - Reserved
        { Register(0x66, 0x80) }, // 0x33
        { Register(0x68, 0x08) }, // 0x34
        { Register(0x6a, 0x31) }, // 0x35
        { Register(0x6c, 0x26) }, // 0x36 -
        { Register(0x6e, 0xe9) }, // 0x37 - 0110 1110, 1110 1001
    };
};