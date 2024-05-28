#include "main.h"
#include "app_audio.hh"

#include "AudioCodec.hh"

#include <cmath>

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

    const uint16_t SOUND_BUFFER_SZ = 256;
    uint16_t rx_sound_buff[SOUND_BUFFER_SZ] = { 0 };
    uint16_t tx_sound_buff[SOUND_BUFFER_SZ] = { 0 };

    audio->EnableLeftMicPGA();
    audio->TurnOnLeftInput3();
    audio->UnmuteMic();
    // audio->MuteMic();

    HAL_UART_Transmit(&huart1, (uint8_t*)"Loopstart\n", 10, HAL_MAX_DELAY);


    // audio->SampleSineWave(tx_sound_buff, 256,
    //     0, 16'000, 100, 440, audio->phase, true);

    HAL_Delay(1000);
    // for (int i =0 ; i < 256; ++i)
    // {
    //     AudioCodec::PrintInt(tx_sound_buff[i]);
    //     HAL_Delay(100);
    // }

    audio->TxRxAudio();

    while (true)
    {
        unsigned int x = 0x8000;
        unsigned int z = 0xFFFF;

        unsigned int y = 0xEFFF;

        // audio.Send1KHzSignal();
        // audio->SendSawToothWave();
        // HAL_I2S_Transmit(&hi2s3, constant_sound_buff, Constant_Sound_Sz, HAL_MAX_DELAY);

        // HAL_Delay(10);

        // HAL_UART_Transmit(&huart1, (uint8_t*)"Donerecv\n", 9, HAL_MAX_DELAY);

        // if (audio->DataAvailable())
        // {
        //     for (int i = 0; i < sz; ++i)
        //     {
        //         audio->GetAudio(buff, 256);
        //         uint8_t num_str[64]{};
        //         uint16_t len = 0;
        //         int_to_string(buff[i], num_str, len);
        //         HAL_UART_Transmit(&huart1, num_str, len, HAL_MAX_DELAY);

        //         // HAL_UART_Transmit(&huart1, bytes, 2, HAL_MAX_DELAY);
        //         HAL_UART_Transmit(&huart1, (uint8_t*)("\n"), 1, HAL_MAX_DELAY);

        //         HAL_Delay(50);
        //     }
        //     HAL_UART_Transmit(&huart1, (uint8_t*)("Done\n"), 5, HAL_MAX_DELAY);

        //     audio->RxAudio();
        // }

    }
    return 0;
}

void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef* hi2s)
{
    HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
    audio->HalfCompleteCallback();
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef* hi2s)
{
    HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
    audio->CompleteCallback();
}