#include "AudioCodec.hh"

// NOTE MCLK = 12Mhz
#include <cmath>

extern UART_HandleTypeDef huart1;

AudioCodec::AudioCodec(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c):
    i2s(&hi2s), i2c(&hi2c), tx_buffer{ 0 }, rx_buffer{ 0 }
{
    // Reset the wm8960
    SetRegister(0x0F, 0b1'0000'0000);
    HAL_Delay(100);

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

    // Set ADCDIV to get 16Khz from SYSCLK (0x03)
    // Set DACDIV to get 16Khz from SYSCLK (0x03)
    // Post scale the PLL to be divided by 2
    // Set the clock (Select the PLL) (0x01)
    SetRegister(0x04, 0b0'1101'1101);

    // Set the clock division
    // D_Clock = sysclk / 16 = 12Mhz / 16 = 0.768Mhz
    // BCLKDIV = SYSCLK / 6 = 2.048Mhz
    SetRegister(0x08, 0b1'1100'1001);

    // Disable soft mute and ADC high pass filter
    SetRegister(0x05, 0b0'0000'0000);

    // Set the Master mode (1), I2S to 16 bit words
    // Set audio data format to i2s mode
    SetRegister(0x07, 0b0'0100'1110);

    // Set the left and right headphone volumes
    SetRegister(0x02, 0b1'0111'1111);
    SetRegister(0x03, 0b1'0111'1111);

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

    // Enable DAC softmute
    SetBit(0x06, 3, 1);

    // Noise gate threshold
    // SetRegister(0x14, 0b0'1111'1001);

    // Slow close enable
    // SetRegister(0x17, 0b1'1100'0001);

    // // Headphone switch enabled
    // SetRegister(0x18, 0b0'0100'0000);

    // // Vmid soft start for anti-pop
    // SetRegister(0x1C, 0b0'0000'0100);

    // SetRegister(0x1D, 0b0'0100'0000);
}

AudioCodec::~AudioCodec()
{
    i2c = nullptr;
    i2s = nullptr;
}

HAL_StatusTypeDef AudioCodec::WriteRegister(uint8_t address)
{
    // PrintRegisterData(address);

    HAL_StatusTypeDef result = HAL_I2C_Master_Transmit(i2c,
        Write_Condition, registers[address].bytes, 2, HAL_MAX_DELAY);

    HAL_Delay(10);

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

bool AudioCodec::OrRegister(uint8_t address, uint16_t data)
{
    if (address > Max_Address)
    {
        return false;
    }

    registers[address].bytes[0] |= uint8_t((data >> 8) & Top_Bit_Mask);
    registers[address].bytes[1] |= uint8_t(data & Bot_Bit_Mask);

    return true;
}

bool AudioCodec::XorRegister(uint8_t address, uint16_t data)
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

bool AudioCodec::SetBit(uint8_t address, uint8_t bit, uint8_t set)
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

bool AudioCodec::SetBits(const uint8_t address, const uint16_t bits, const uint16_t set)
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

void AudioCodec::TurnOnLeftInput3()
{
    // Turn off differential input
    TurnOffLeftDifferentialInput();

    EnableLeftMicPGA();

    SetBit(0x20, 7, 1);
    SetBit(0x20, 8, 1);
}

void AudioCodec::TurnOffLeftInput3()
{
    SetBit(0x20, 7, 0);
}

void AudioCodec::TurnOnLeftDifferentialInput()
{
    // Turn off single input
    TurnOffLeftInput3();

    EnableLeftMicPGA();

    SetBit(0x20, 6, 1);
    SetBit(0x20, 8, 1);
    // SetBit(0x20, 5, 1);
}

void AudioCodec::TurnOffLeftDifferentialInput()
{
    SetBit(0x20, 6, 0);
    SetBit(0x20, 8, 0);
}

void AudioCodec::EnableLeftMicPGA()
{
    // Set bits for all left input pgas
    SetBit(0x19, 5, 1);
    SetBit(0x2f, 5, 1);
    SetBit(0x20, 3, 1);
}

void AudioCodec::DisableLeftMicPGA()
{
    // Set bits for all left input pgas
    SetBit(0x19, 5, 0);
    SetBit(0x2f, 5, 0);
}

void AudioCodec::MuteMic()
{
    SetBits(0x00, 0b1'1000'0000, 0b0'1000'0000);

    SetBits(0x2B, 0b0'0111'0000, 0b0'0000'0000);

    SetBit(0x19, 1, 0);
}

void AudioCodec::UnmuteMic()
{
    // Disable LINMUTE
    SetBits(0x00, 0b1'1000'0000, 0b1'0000'0000);

    // Set LIN3BOOST to +0dB
    SetBits(0x2B, 0b0'0111'0000, 0b0'0101'0000);

    // Enable MIC bias
    SetBit(0x19, 1, 1);
}

bool AudioCodec::DataAvailable()
{
    return data_available;
}

void AudioCodec::StartI2S()
{
    auto output = HAL_I2SEx_TransmitReceive_DMA(i2s, tx_buffer, rx_buffer, Audio_Buffer_Sz);

    if (output == HAL_OK)
    {
        flags |= (1 << running_flag);
    }
}

void AudioCodec::StopI2S()
{

}

void AudioCodec::GetAudio(uint16_t* buffer, uint16_t size)
{
    for (uint16_t i = 0; i < size && i < Audio_Buffer_Sz; ++i)
    {
        buffer[i] = rx_buffer[i];
    }

    data_available = false;
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

void AudioCodec::HalfCompleteCallback()
{
    // float freqs [] = { 523.25f, 659.26f, 783.99f };
    // SampleHarmonic(tx_buffer, Audio_Buffer_Sz / 2,
    //     0, Sample_Rate,
    //     1000, freqs, phases, 3, true);

    // SampleSineWave(tx_buffer, Audio_Buffer_Sz / 2,
    //     0, Sample_Rate,
    //     1000, 1000, phase, true);


    // copy rx to tx
    for (uint16_t i = 0; i < Audio_Buffer_Sz / 2; i+=2)
    {
        tx_buffer[i] = rx_buffer[i] + 5000;
        tx_buffer[i+1] = rx_buffer[i] + 5000;
    }
}

void AudioCodec::CompleteCallback()
{
    // float freqs [] = { 523.25f, 659.26f, 783.99f};
    // SampleHarmonic(tx_buffer, Audio_Buffer_Sz / 2,
    //     Audio_Buffer_Sz / 2, Sample_Rate,
    //     1000, freqs, phases, 3, true);

    // SampleSineWave(tx_buffer, Audio_Buffer_Sz / 2,
    //     Audio_Buffer_Sz / 2, Sample_Rate,
    //     1000, 1000, phase, true);

    // copy rx to tx
    for (uint16_t i = Audio_Buffer_Sz / 2; i < Audio_Buffer_Sz; i+=2)
    {
        tx_buffer[i] = rx_buffer[i] + 5000;
        tx_buffer[i+1] = rx_buffer[i] + 5000;
    }
}

void AudioCodec::SampleSineWave(uint16_t* buff, const uint16_t num_samples,
    uint16_t start_idx, uint16_t sample_rate,
    float amplitude, float freq, float& phase, bool stereo)
{
    constexpr uint16_t offset = 2000;
    constexpr float TWO_PI = M_PI * 2;
    const float angular_freq = TWO_PI * freq;
    float current_phase = 0.0f;
    uint16_t samples = num_samples;
    if (stereo)
    {
        samples = samples / 2;

        for (uint16_t i = 0; i < samples; ++i)
        {
            const float step = (float)i / sample_rate;
            const float sample = amplitude * sin(angular_freq * step + phase);

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
            const float step = (float)i / sample_rate;
            const float sample = amplitude * sin(angular_freq * step + phase);

            // Add offset to handle negative numbers and overflow back around
            // to their regular values for the positive numbers
            buff[start_idx + i] = uint16_t(offset + sample);
        }
    }

    phase += angular_freq * (float(samples) / sample_rate);
    while (phase > TWO_PI)
    {
        phase -= TWO_PI;
    }
}

void AudioCodec::SampleHarmonic(uint16_t* buff, const uint16_t num_samples,
    uint16_t start_idx, uint16_t sample_rate,
    float amplitutde, float freqs [], float phases [],
    const uint16_t num_freqs, bool stereo)
{

    // Clear the buffer first
    for (uint16_t i = 0; i < num_samples; ++i)
    {
        buff[start_idx + i] = 0;
    }

    for (uint16_t i = 0 ; i < num_freqs; ++i)
    {
        uint16_t harmonic[num_samples] = { 0 };
        SampleSineWave(harmonic, num_samples, 0, sample_rate,
            amplitutde, freqs[i], phases[i], stereo);

        for (uint16_t i = 0; i < num_samples; ++i)
        {
            buff[start_idx + i] += harmonic[i];
        }
    }
}

void AudioCodec::SendSawToothWave()
{
    uint32_t sample_rate = 16'000;
    float amplitude = 2;
    float freq = 1000.0;
    float period = sample_rate / freq;

    uint16_t num_samples = 256;
    static uint16_t buff[256];

    for (int i = 0; i < num_samples; ++i)
    {
        float phase = fmod(i, period) / period;

        float sawtooth_value = amplitude * (2 * phase - 1);

        if (sawtooth_value > 1.0f)
        {
            sawtooth_value = 1.0f;
        }
        else if (sawtooth_value < -1.0f)
        {
            sawtooth_value = -1.0f;
        }

        buff[i] = (uint16_t)((sawtooth_value + 1.0f) * 32767);
    }

    HAL_I2S_Transmit(i2s, buff, num_samples, HAL_MAX_DELAY);

    HAL_Delay(10);
}
