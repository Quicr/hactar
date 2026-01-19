#pragma once

#include "led.hh"
#include "stm32.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_tim.h"

class RGBLED
{
public:
    RGBLED(LED red, LED green, LED blue, TIM_HandleTypeDef& htim, DMA_HandleTypeDef& hdma);

    enum class Colour
    {
        Red,
        Green,
        Blue,
    };

    enum class Mode
    {
        Off,
        On,
        Breathe,
        Flash,
    };

    enum class FlashStatus
    {
        Off,
        On
    };

    enum class Period
    {
        Slow,
        Medium,
        Fast
    };

    void On(const Colour colour, uint8_t brightness);
    void Off(const Colour colour);
    void Breathe(const Colour colour);
    void Flash(const Colour colour, const Period rate);

    static void Callback(DMA_HandleTypeDef* hdma);

private:
    void StartUpdater(const Colour colour);
    LED& SelectLED(const Colour colour);
    void BreatheUpdate(const Colour colour);
    void FlashUpdate(const Colour colour);
    void CalculateBrightness(const Colour colour, uint8_t brightness);
    void ChangePeriod(const Period colour);

    static constexpr uint32_t Pwm_Size = 20;
    static constexpr int8_t Step_Size = 3;

    LED red;
    LED green;
    LED blue;
    TIM_HandleTypeDef& htim;
    DMA_HandleTypeDef& hdma;
    Colour active_colour;
    Mode mode;

    uint32_t pwm_pattern[Pwm_Size];
    int8_t breathe_current_step;
    int8_t breathe_step;
    FlashStatus flash_status;
};
