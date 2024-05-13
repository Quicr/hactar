#include "main.h"
#include "app_audio.hh"

#include "AudioCodec.hh"

extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s3;

extern UART_HandleTypeDef huart1;



void int_to_string(const unsigned long input, uint8_t* str, uint16_t& size)
{
    unsigned long value = input;
    // Get the num of powers
    unsigned long tmp = value;
    unsigned long sz = 0;
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
    unsigned long mod = 0;
    do
    {
        mod = value % 10;
        str[sz - 1] = '0' + (char)mod;
        sz--;
        value = value / 10;
    } while (value > 0);

}

void generateTriangleWave(uint16_t* buffer, int numSamples, int sampleRate, float amplitude, float frequency)
{
    float period = 1.0f / frequency; // Period of the wave in seconds
    float increment = period / (sampleRate / 2.0f); // Increment for each sample

    float currentValue = 0.0f;
    float direction = 1.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float value = currentValue * amplitude;
        // Scale the value to fit into the range of uint16_t
        buffer[i] = (uint16_t)(value * INT16_MAX);

        // Update current value and direction
        currentValue += direction * increment;

        // Change direction at the peak and trough
        if (currentValue >= 1.0f || currentValue <= -1.0f)
        {
            direction *= -1.0f;
        }
    }
}

// void GenerateSawtoothWave(uint16_t* buffer, )
// {

// }

AudioCodec* audio;

int app_main()
{
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);

    HAL_Delay(5000);
    const uint8_t start [] = "Start\n";
    HAL_UART_Transmit(&huart1, start, 7, HAL_MAX_DELAY);
    audio = new AudioCodec(hi2s3, hi2c1);

    // audio.SendAllOnes();


    #define SAMPLE_RATE 16'000 // 16Khz

    // Lookup table for sine wave generation (values are between 0 and DAC_RESOLUTION)
    const uint16_t sine_wave [] = {
        2048, 2447, 2831, 3185, 3495, 3750, 3939, 4056,
        4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
        2048, 1648, 1264, 910, 600, 345, 156, 39,
        0, 39, 156, 345, 600, 910, 1264, 1648
        // Add more values as needed to cover a complete cycle
    };

    const uint16_t SOUND_BUFFER_SZ = 16000;
    uint16_t rx_sound_buff[SOUND_BUFFER_SZ] = { 0 };
    uint16_t tx_sound_buff[SOUND_BUFFER_SZ] = { 0 };
    int i = 0;
    while (i < SOUND_BUFFER_SZ)
    {
        tx_sound_buff[i++] = 0x00FF;
        // for (unsigned int j = 0; j < sizeof(sine_wave) / sizeof(sine_wave[0]); ++j)
        // {
        //     tx_sound_buff[i++] = sine_wave[j];
        // }
    }

    const uint16_t Constant_Sound_Sz = 256;
    uint16_t constant_sound_buff[Constant_Sound_Sz] = { 0 };

    for (i = 0; i < Constant_Sound_Sz; ++i)
    {
        constant_sound_buff[i] = i + 0xFF;
    }

    audio->EnableLeftMicPGA();
    audio->TurnOnLeftInput3();
    audio->UnmuteMic();

    uint16_t buff[256];

    HAL_UART_Transmit(&huart1, (uint8_t*)"Loopstart\n", 10, HAL_MAX_DELAY);


    while (true)
    {


        // audio.Send1KHzSignal();
        audio->SendSawToothWave();
        // HAL_I2S_Transmit(&hi2s3, constant_sound_buff, Constant_Sound_Sz, HAL_MAX_DELAY);

        // HAL_Delay(10);



        // audio->RxAudioBlocking(buff, 256);
        // HAL_UART_Transmit(&huart1, (uint8_t*)"Donerecv\n", 9, HAL_MAX_DELAY);

        // for (int i = 0; i < 256; ++i)
        // {
        //     uint8_t* bytes = (uint8_t*)buff[i];

        //     uint8_t num_str[64]{};
        //     uint16_t len = 0;
        //     int_to_string(bytes[0], num_str, len);
        //     HAL_UART_Transmit(&huart1, num_str, len, HAL_MAX_DELAY);

        //     int_to_string(bytes[1], num_str, len);
        //     HAL_UART_Transmit(&huart1, num_str, len, HAL_MAX_DELAY);

        //     // HAL_UART_Transmit(&huart1, bytes, 2, HAL_MAX_DELAY);
        //     HAL_UART_Transmit(&huart1, (uint8_t*)("\n"), 1, HAL_MAX_DELAY);

        //     HAL_Delay(10);
        // }

    }
    return 0;
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{

}