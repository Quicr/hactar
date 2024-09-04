#include "main.h"
#include "app_audio.hh"

#include "audio_chip.hh"

#include "Screen.hh"

#include <cmath>

extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s3;

extern UART_HandleTypeDef huart1;

extern SPI_HandleTypeDef hspi1;



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

// AudioChip* audio;
    port_pin cs = { DISP_CS_GPIO_Port, DISP_CS_Pin };
    port_pin dc = { DISP_DC_GPIO_Port, DISP_DC_Pin };
    port_pin rst = { DISP_RST_GPIO_Port, DISP_RST_Pin };
    port_pin bl = { DISP_BL_GPIO_Port, DISP_BL_Pin };

    Screen screen(
        hspi1,
        cs,
        dc,
        rst,
        bl,
        Screen::Orientation::left_landscape
    );

int app_main()
{
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);

    // audio = new AudioChip(hi2s3, hi2c1);

    HAL_Delay(4000);

    // audio->Init();

    // audio->EnableLeftMicPGA();
    // audio->TurnOnLeftDifferentialInput();
    // // audio->TurnOnLeftInput3();
    // audio->UnmuteMic();
    // audio->MuteMic();

    // Need about 1s delay before starting i2s
    // HAL_Delay(1000);

    // audio->StartI2S();

    uint32_t next = HAL_GetTick();


    screen.Begin();
    screen.EnableBackLight();
    screen.FillScreen(C_BLACK);
    screen.FillRectangleAsync(0, screen.ViewWidth(), 0, screen.ViewHeight(), C_GREEN);
    screen.FillScreen(C_BLACK);
    screen.FillRectangleAsync(0, screen.ViewWidth(), 0, screen.ViewHeight(), C_GREEN);

    // screen.FillRectangleAsync(20, 30, 20, 30, C_BLUE);
    // screen.FillRectangleAsync(0, screen.ViewWidth(), 0, 1, C_RED);
    // screen.FillRectangleAsync(0, 10, 1, 11, C_GREEN);

    uint32_t current_tick;
    while (true)
    {
        current_tick = HAL_GetTick();
        screen.Update(current_tick);

        if (HAL_GetTick() > next)
        {
            HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
            next = HAL_GetTick() + 5000;
        }
    }
    return 0;
}

void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef* hi2s)
{
    // HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
    // audio->HalfCompleteCallback();
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef* hi2s)
{
    // HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
    // audio->CompleteCallback();
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    UNUSED(hspi);
    screen.ReleaseSPI();
}
