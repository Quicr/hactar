#include "main.h"
#include "app_audio.hh"

#include "AudioCodec.hh"

extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s3;

extern UART_HandleTypeDef huart1;

void generateTriangleWave(uint16_t* buffer, int numSamples, int sampleRate, float amplitude, float frequency) {
    float period = 1.0f / frequency; // Period of the wave in seconds
    float increment = period / (sampleRate / 2.0f); // Increment for each sample

    float currentValue = 0.0f;
    float direction = 1.0f;

    for (int i = 0; i < numSamples; ++i) {
        float value = currentValue * amplitude;
        // Scale the value to fit into the range of uint16_t
        buffer[i] = (uint16_t)(value * INT16_MAX);

        // Update current value and direction
        currentValue += direction * increment;

        // Change direction at the peak and trough
        if (currentValue >= 1.0f || currentValue <= -1.0f) {
            direction *= -1.0f;
        }
    }
}

int app_main()
{
    HAL_Delay(1009);
    const uint8_t start [] = "Start\n";
    HAL_UART_Transmit(&huart1, start, 7, HAL_MAX_DELAY);
    AudioCodec audio(hi2s3, hi2c1);

    // audio.SendAllOnes();


    #define SAMPLE_RATE 44100
    #define DAC_RESOLUTION 4095 // 12-bit DAC

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
        for (unsigned int j = 0; j < sizeof(sine_wave) / sizeof(sine_wave[0]); ++j)
        {
            tx_sound_buff[i++] = sine_wave[j];
        }
    }

    while (true)
    {


        // audio.Send1KHzSignal();
        HAL_I2S_Transmit(&hi2s3, tx_sound_buff, SOUND_BUFFER_SZ, HAL_MAX_DELAY);

        // HAL_Delay(10);

    }
    return 0;
}