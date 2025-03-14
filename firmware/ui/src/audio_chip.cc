#include "audio_chip.hh"
#include "audio_codec.hh"

#include "main.h"
#include "app_main.hh"

#include "logger.hh"

extern UART_HandleTypeDef huart1;

AudioChip::AudioChip(I2S_HandleTypeDef& hi2s, I2C_HandleTypeDef& hi2c):
    i2s(&hi2s),
    i2c(&hi2c),
    tx_buffer{ 0 },
    tx_ptr{ tx_buffer },
    rx_buffer{ 0 },
    rx_ptr{ rx_buffer },
    buff_mod(0),
    flags(0),
    volume(Default_Volume),
    mic_volume(Default_Mic_Volume)
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

    // Disable soft mute and ADC high pass filter
    SetRegister(0x05, 0b0'0000'0000);

    SetClocks();

    // Set the left and right headphone volumes
    MicVolumeSet(mic_volume);
    VolumeSet(volume);

    // Enable mono mixer
    SetBit(0x17, 4, 1);
    SetBit(0x2A, 6, 0);

    // Enable the outputs
    SetRegister(0x31, 0b0'0111'0111);

    // Set DAC left and right volumes
    SetRegister(0x0A, 0b1'1111'1111);
    SetRegister(0x0B, 0b1'1111'1111);

    // Set left and right mixer
    SetRegister(0x22, 0b1'0000'0000);
    SetRegister(0x25, 0b1'0000'0000);

    SetBits(0x2B, 0b0'0111'0000, 0b0'0111'000);

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

            // Set the Master mode (1), I2S to 16 bit words
            // Set audio data format to i2s mode
            SetRegister(0x07, 0b0'0100'0010);

    UnmuteMic();
}

void AudioChip::Reset()
{
    // Reset the wm8960
    SetRegister(0x0F, 0b1'0000'0000);
    HAL_Delay(100);
}

// NOTE- These are hard coded values in the constants.hh file
// for this function
void AudioChip::SetClocks()
{
    switch (constants::Sample_Rate)
    {
        case constants::SampleRates::_8khz:
        {
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

            // Change the ALC sample rate -> 8kHz
            SetRegister(0x1B, 0b0'0000'0101);

            break;
        }
        case constants::SampleRates::_16khz:
        {

            break;
        }
        default:
        {
            Error("Audio chip set frequencies", "Frequency set that doesn't exist");
            break;
        }
    }
}


// NOTE- These are hard coded values in the constants.hh file
// for this function
void AudioChip::SetStereo()
{
    if (constants::Stereo)
    {

    }
}

void AudioChip::VolumeSet(const int16_t vol)
{
    volume = vol;
    if (volume >= Max_Volume)
    {
        volume = Max_Volume;
    }
    else if (volume < Min_Volume)
    {
        volume = Min_Volume;
    }

    // Clear the volume section of the register
    // Set the vol bit to 0 so the volume is set into the intermediate
    // register
    SetBits(0x02, 0b1'0111'1111, volume);

    // Then flip the first bit (which is the update volume bit)
    // for both headphones
    SetBits(0x03, 0b1'0111'1111, 0x100 + volume);
}

void AudioChip::VolumeAdjust(const int16_t db)
{
    VolumeSet(volume + db);
}

void AudioChip::VolumeUp()
{
    VolumeSet(volume + 1);
}

void AudioChip::VolumeDown()
{
    VolumeSet(volume - 1);
}

void AudioChip::VolumeReset()
{
    VolumeSet(Default_Volume);
}

uint16_t AudioChip::Volume()
{
    return volume;
}

void AudioChip::MicVolumeSet(const int16_t vol)
{
    if (vol >= Max_Mic_Volume)
    {
        mic_volume = Max_Mic_Volume;
    }
    else if (vol <= Min_Mic_Volume)
    {
        mic_volume = Min_Mic_Volume;
    }
    else
    {
        mic_volume = vol;
    }

    // Then flip the first bit (which is the update mic_volume bit)
    // for both headphones
    SetBits(0x00, 0b1'0011'1111, 0x100 + mic_volume);
}

void AudioChip::MicVolumeAdjust(const int16_t steps)
{
    MicVolumeSet(mic_volume + steps);
}

void AudioChip::MicVolumeUp()
{
    MicVolumeSet(mic_volume + 1);
}

void AudioChip::MicVolumeDown()
{
    MicVolumeSet(mic_volume - 1);
}

void AudioChip::MicVolumeReset()
{
    MicVolumeSet(Default_Mic_Volume);
}

uint16_t AudioChip::MicVolume()
{
    return mic_volume;
}

HAL_StatusTypeDef AudioChip::WriteRegister(uint8_t address)
{
    // PrintRegisterData(address);

    HAL_StatusTypeDef result = HAL_I2C_Master_Transmit(i2c,
        Write_Condition, registers[address].bytes, 2, HAL_MAX_DELAY);

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
        uint8_t set_mask = (bit - 7) & data;

        // Upper register, so the 8th bit is the lower of the upper register
        uint8_t reset_mask = 0xFE;

        registers[address].bytes[0] &= reset_mask;
        registers[address].bytes[0] |= set_mask;
    }
    else
    {
        // Lower bits
        // Reset mask
        uint8_t set_mask = (data << bit);
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

    SetBits(0x2B, 0b0'0000'1110, 0b0'0000'0000);

    SetBit(0x19, 1, 0);
}

void AudioChip::UnmuteMic()
{
    TurnOnLeftDifferentialInput();

    // Disable LINMUTE
    SetBits(0x00, 0b1'1000'0000, 0b1'0000'0000);

    // Set LIN2BOOST to +0dB
    SetBits(0x2B, 0b0'0000'1110, 0b0'0000'1010);

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
    // NOTE- Do not remove delay the audio chip needs time to stabilize
    HAL_Delay(20);
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
    return tx_ptr;
}

const uint16_t* AudioChip::RxBuffer()
{
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
