#include "audio_chip.hh"
#include "audio_codec.hh"

#include "main.h"

// NOTE MCLK = 12Mhz
#include <math.h>
// For some reason M_PI is not defined when including math.h even though it
// should be. Therefore, we need to do it ourselves.
#ifndef  M_PI
#define  M_PI  3.1415926535897932384626433
#endif

extern UART_HandleTypeDef huart1;

AudioChip::AudioChip(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c):
    i2s(&hi2s),
    i2c(&hi2c),
    tx_buffer{ 0 },
    tx_ptr{ tx_buffer },
    rx_buffer{ 0 },
    rx_ptr{ rx_buffer },
    buff_mod(0)
{
}

AudioChip::~AudioChip()
{
    i2c = nullptr;
    i2s = nullptr;
}

void AudioChip::Init()
{
    // Reset the wm8960
    Reset();

    // Set the power
    SetRegister(0x19, 0b0'1111'1110);

    // Enable outputs
    SetRegister(0x1A, 0b1'1110'0001);

    // Enable lr mixer ctrl
    // SetRegister(0x2F, 0b0'0000'0000);
    SetRegister(0x2F, 0b0'0010'1100);

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

    // Pg 85 version
    SetRegister(0x35, 0b0'0011'0001);
    SetRegister(0x36, 0b0'0010'0110);
    SetRegister(0x37, 0b0'1110'1001);

    // Set ADCDIV to get 8kHz from SYSCLK
    // Set DACDIV to get 8kHz from SYSCLK
    // Post scale the PLL to be divided by 2
    // Set the clock (Select the PLL) (0x01)
    SetRegister(0x04, 0b1'1011'0101);

    // Set the clock division
    // D_Clock = sysclk / 16 = 12Mhz / 16 = 0.768Mhz
    // BCLKDIV = SYSCLK / 6 = 2.048Mhz // this is for 32Khz audio
    // Expected BCLK = constants::sample_rate * channels * bits per channel, so 16khz * 2 * 16 = 512Khz
    SetRegister(0x08, 0b1'1100'1100);

    // Disable soft mute and ADC high pass filter
    SetRegister(0x05, 0b0'0000'0000);

    // Set the Master mode (1), I2S to 16 bit words
    // Set audio data format to i2s mode
    SetRegister(0x07, 0b0'0100'1110);

    // Set the left and right headphone volumes
    SetRegister(0x02, 0b1'0111'1111);
    SetRegister(0x03, 0b1'0111'1111);

    // Enable mono mixer
    SetBit(0x17, 4, 1);

    // Enable the outputs
    SetRegister(0x31, 0b0'0111'0111);

    // Set DAC left and right volumes
    SetRegister(0x0A, 0b1'1111'1111);
    SetRegister(0x0B, 0b1'1111'1111);

    // Set left and right mixer
    SetRegister(0x22, 0b1'0000'0000);
    SetRegister(0x25, 0b1'0000'0000);

    // Change the ALC sample rate -> 8kHz
    SetRegister(0x1B, 0b0'0000'0101);

    // Enable DAC softmute
    SetBit(0x06, 3, 1);

    // Noise gate threshold
    // SetRegister(0x14, 0b0'1111'1001);

    // Slow close enable
    // SetRegister(0x17, 0b1'1101'0000);

    // // Headphone switch enabled
    // SetRegister(0x18, 0b0'0100'0000);

    // // Vmid soft start for anti-pop
    // SetRegister(0x1C, 0b0'0000'0100);

    // SetRegister(0x1D, 0b0'0100'0000);


    UnmuteMic();
}

void AudioChip::Reset()
{
    // Reset the wm8960
    SetRegister(0x0F, 0b1'0000'0000);
    HAL_Delay(100);
}

HAL_StatusTypeDef AudioChip::WriteRegister(uint8_t address)
{
    // PrintRegisterData(address);

    HAL_StatusTypeDef result = HAL_I2C_Master_Transmit(i2c,
        Write_Condition, registers[address].bytes, 2, HAL_MAX_DELAY);

    HAL_Delay(10);

    return result;
}

bool AudioChip::SetRegister(uint8_t address, uint16_t data)
{
    if (address > Max_Address)
    {
        return false;
    }

    // Reset the top bit
    registers[address].bytes[0] &= ~Top_Bit_Mask;

    // Set the register data
    registers[address].bytes[0] |= uint8_t((data >> 8) & Top_Bit_Mask);
    registers[address].bytes[1] = uint8_t(data & Bot_Bit_Mask);

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

bool AudioChip::OrRegister(uint8_t address, uint16_t data)
{
    if (address > Max_Address)
    {
        return false;
    }

    registers[address].bytes[0] |= uint8_t((data >> 8) & Top_Bit_Mask);
    registers[address].bytes[1] |= uint8_t(data & Bot_Bit_Mask);

    return true;
}

bool AudioChip::XorRegister(uint8_t address, uint16_t data)
{
    if (address > Max_Address)
    {
        return false;
    }

    // Update the register data
    registers[address].bytes[0] ^= uint8_t((data >> 8) & Top_Bit_Mask);
    registers[address].bytes[1] ^= uint8_t(data & Bot_Bit_Mask);

    return (WriteRegister(address) == HAL_OK);
}

bool AudioChip::SetBit(uint8_t address, uint8_t bit, uint8_t set)
{
    if (address > Max_Address)
    {
        return false;
    }

    const uint8_t data = set > 0 ? 1 : 0;

    if (bit > 7)
    {
        // Upper bit
        // First make sure only the first bit is actually set
        uint8_t set_mask = (bit - 7) & 0x01;
        uint8_t reset_mask = 0xFE;

        registers[address].bytes[0] &= reset_mask;
        registers[address].bytes[0] |= set_mask;
    }
    else
    {
        // Lower bits
        // Reset mask
        uint8_t set_mask = (1 << bit);
        uint8_t reset_mask = ~set_mask;

        registers[address].bytes[1] &= reset_mask;
        registers[address].bytes[1] |= set_mask;
    }

    return WriteRegister(address) == HAL_OK;
}

bool AudioChip::SetBits(const uint8_t address, const uint16_t bits, const uint16_t set)
{
    if (address > Max_Address)
    {
        return false;
    }

    // ex bits = 0x71F1, set = 0x00F2;
    // Bits < 0x1FF
    // masked bits = 0x01F1
    const uint16_t masked_bits = bits & 0x01FF;

    // reset bits = 0xFE0E
    const uint16_t reset_bits = ~masked_bits;

    // Only use the bits that we said we would be using in bits.
    // masked set = 0x00F2 & 0x1FF = 0x00F2 & 0x01F1 = 0x00F0
    const uint16_t masked_set = masked_bits & (set & 0x1FF);

    registers[address].bytes[0] &= uint8_t(reset_bits >> 8);
    registers[address].bytes[1] &= uint8_t(reset_bits & 0x00FF);

    registers[address].bytes[0] |= uint8_t(masked_set >> 8);
    registers[address].bytes[1] |= uint8_t(masked_set & 0x00FF);

    return WriteRegister(address) == HAL_OK;
}

bool AudioChip::ReadRegister(uint8_t address, uint16_t& value)
{
    if (address > Max_Address)
    {
        return false;
    }

    value = (uint16_t)((registers[address].bytes[0] & 0x0001) << 8);
    value += registers[address].bytes[1];
    return true;
}

void AudioChip::TurnOnLeftInput3()
{
    // Turn off differential input
    TurnOffLeftDifferentialInput();

    EnableLeftMicPGA();

    SetBit(0x20, 7, 1);
    SetBit(0x20, 8, 1);
}

void AudioChip::TurnOffLeftInput3()
{
    SetBit(0x20, 7, 0);
}

void AudioChip::TurnOnLeftDifferentialInput()
{
    // Turn off single input
    EnableLeftMicPGA();

    TurnOffLeftInput3();

    SetBit(0x20, 6, 1);
    SetBit(0x20, 8, 1);
}

void AudioChip::TurnOffLeftDifferentialInput()
{
    SetBit(0x20, 6, 0);
    SetBit(0x20, 8, 0);
}

void AudioChip::EnableLeftMicPGA()
{
    // Set bits for all left input pgas
    SetBit(0x19, 5, 1);
    SetBit(0x2f, 5, 1);
    SetBit(0x20, 3, 1);
}

void AudioChip::DisableLeftMicPGA()
{
    // Set bits for all left input pgas
    SetBit(0x19, 5, 0);
    SetBit(0x2f, 5, 0);
}

void AudioChip::MuteMic()
{
    SetBits(0x00, 0b1'1000'0000, 0b0'1000'0000);

    SetBits(0x2B, 0b0'0111'0000, 0b0'0000'0000);

    SetBit(0x19, 1, 0);
}

void AudioChip::UnmuteMic()
{
    TurnOnLeftDifferentialInput();

    // Disable LINMUTE
    SetBits(0x00, 0b1'1000'0000, 0b1'0000'0000);

    // Set LIN3BOOST to +0dB
    SetBits(0x2B, 0b0'0111'0000, 0b0'0101'0000);

    // Enable MIC bias
    SetBit(0x19, 1, 1);
}

bool AudioChip::TxBufferReady()
{
    return ReadFlag(AudioFlag::Tx_Ready);
}

bool AudioChip::RxBufferReady()
{
    return ReadFlag(AudioFlag::Rx_Ready);
}

void AudioChip::StartI2S()
{
    auto output = HAL_I2SEx_TransmitReceive_DMA(i2s, tx_buffer, rx_buffer, constants::Audio_Buffer_Sz);

    if (output == HAL_OK)
    {
        RaiseFlag(AudioChip::Running);
    }
}

void AudioChip::StopI2S()
{
    HAL_I2S_DMAStop(i2s);

    LowerFlag(AudioChip::Running);
}

uint16_t* AudioChip::TxBuffer()
{
    // LowerFlag(AudioFlag::Tx_Ready);
    return tx_ptr;
}

const uint16_t* AudioChip::RxBuffer()
{
    // LowerFlag(AudioFlag::Rx_Ready);
    return rx_ptr;
}

void AudioChip::ISRCallback()
{
    const uint16_t offset = buff_mod * constants::Audio_Buffer_Sz_2;
    tx_ptr = tx_buffer + offset;
    rx_ptr = rx_buffer + offset;
    buff_mod = !buff_mod;

    // Clear the transmission buffer
    for (uint16_t i = 0; i < constants::Audio_Buffer_Sz_2; ++i)
    {
        tx_ptr[i] = 0;
    }

    RaiseFlag(AudioFlag::Rx_Ready);
    RaiseFlag(AudioFlag::Tx_Ready);
}

void AudioChip::ClearTxBuffer()
{
    for (uint16_t i = 0; i < constants::Audio_Buffer_Sz; ++i)
    {
        tx_buffer[i] = 0;
    }
}

void AudioChip::Transmit(uint16_t* buff, const size_t size)
{
    uint16_t* tmp_ptr = tx_ptr;
    for (size_t i = 0; i < constants::Audio_Buffer_Sz && i < size; ++i)
    {
        tmp_ptr[i] = buff[i];
    }

    LowerFlag(AudioFlag::Tx_Ready);
}

void AudioChip::Recieve(uint16_t* buff, const size_t size)
{
    // Make a tmp pointer so that if there is an interrupt during transfer
    // then our pointer won't change
    uint16_t* tmp_ptr = rx_ptr;
    for (size_t i = 0 ; i < constants::Audio_Buffer_Sz && i < size; ++i)
    {
        buff[i] = tmp_ptr[i];
    }

    LowerFlag(AudioFlag::Rx_Ready);
}

void AudioChip::SampleSineWave(const uint16_t num_samples,
    const uint16_t start_idx, const double amplitude, const double freq,
    double& phase, const bool stereo)
{
    constexpr uint16_t offset = 2000;
    constexpr double TWO_PI = M_PI * 2;
    const double angular_freq = TWO_PI * freq;
    double current_phase = 0.0f;
    uint16_t samples = num_samples;
    if (stereo)
    {
        samples = samples / 2;

        for (uint16_t i = 0; i < samples; ++i)
        {
            const double step = (double)i / constants::Sample_Rate;
            const double sample = amplitude * sin(angular_freq * step + phase);

            // Add offset to handle negative numbers and overflow back around
            // to their regular values for the positive numbers
            const uint16_t int_sample = uint16_t(offset + sample) + 1;

            tx_buffer[start_idx + (i * 2)] = int_sample;
            tx_buffer[start_idx + (i * 2 + 1)] = int_sample;
        }
    }
    else
    {
        for (uint16_t i = 0; i < samples; ++i)
        {
            const double step = (double)i / constants::Sample_Rate;
            const double sample = amplitude * sin(angular_freq * step + phase);

            // Add offset to handle negative numbers and overflow back around
            // to their regular values for the positive numbers
            tx_buffer[start_idx + i] = uint16_t(offset + sample);
        }
    }

    phase += angular_freq * (double(samples) / constants::Sample_Rate);
    while (phase > TWO_PI)
    {
        phase -= TWO_PI;
    }
}

// Static version
void AudioChip::SampleSineWave(uint16_t* buff, const uint16_t num_samples,
    const uint16_t start_idx, const double amplitude, const double freq,
    double& phase, const bool stereo)
{
    constexpr uint16_t offset = 2000;
    constexpr double TWO_PI = M_PI * 2;
    const double angular_freq = TWO_PI * freq;
    double current_phase = 0.0f;
    uint16_t samples = num_samples;
    if (stereo)
    {
        samples = samples / 2;

        for (uint16_t i = 0; i < samples; ++i)
        {
            const double step = (double)i / constants::Sample_Rate;
            const double sample = amplitude * sin(angular_freq * step + phase);

            // Add offset to handle negative numbers and overflow back around
            // to their regular values for the positive numbers
            const uint16_t int_sample = uint16_t(offset + sample) + 1;

            buff[start_idx + (i * 2)] = int_sample;
            buff[start_idx + (i * 2 + 1)] = int_sample;
        }
    }
    else
    {
        for (uint16_t i = 0; i < samples; ++i)
        {
            const double step = (double)i / constants::Sample_Rate;
            const double sample = amplitude * sin(angular_freq * step + phase);

            // Add offset to handle negative numbers and overflow back around
            // to their regular values for the positive numbers
            buff[start_idx + i] = uint16_t(offset + sample);
        }
    }

    phase += angular_freq * (double(samples) / constants::Sample_Rate);
    while (phase > TWO_PI)
    {
        phase -= TWO_PI;
    }
}

void AudioChip::SampleHarmonic(uint16_t* buff, const uint16_t num_samples,
    const uint16_t start_idx, const double amplitutde, double freqs [], double phases [],
    const uint16_t num_freqs, const bool stereo)
{

    // Clear the buffer first
    for (uint16_t i = 0; i < num_samples; ++i)
    {
        buff[start_idx + i] = 0;
    }

    for (uint16_t i = 0 ; i < num_freqs; ++i)
    {
        uint16_t harmonic[num_samples] = { 0 };
        SampleSineWave(harmonic, num_samples, 0,
            amplitutde, freqs[i], phases[i], stereo);

        for (uint16_t i = 0; i < num_samples; ++i)
        {
            buff[start_idx + i] += harmonic[i];
        }
    }
}

inline void AudioChip::RaiseFlag(AudioFlag flag)
{
    flags |= 1 << flag;
}

inline void AudioChip::LowerFlag(AudioFlag flag)
{
    flags &= ~(1 << flag);
}

inline bool AudioChip::ReadFlag(AudioFlag flag)
{
    return (flags >> flag) & 0x01;
}

inline bool AudioChip::ReadAndLowerFlag(AudioFlag flag)
{
    const bool res = (flags >> flag) & 0x01;
    LowerFlag(flag);
    return res;
}
