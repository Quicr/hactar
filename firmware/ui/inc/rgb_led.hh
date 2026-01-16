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

    void On(const Colour colour, uint8_t brightness);
    void Off(const Colour colour);
    void Blink(const Colour colour);
    void Strobe(const Colour colour);
    void Breathe(const Colour colour);

    static void Callback(DMA_HandleTypeDef* hdma);

private:
    void StartUpdater(const Colour colour);
    LED& SelectLED(const Colour colour);
    void BreatheUpdate();

    static constexpr uint32_t Pwm_Size = 20;

    LED red;
    LED green;
    LED blue;
    TIM_HandleTypeDef& htim;
    DMA_HandleTypeDef& hdma;
    Colour active_colour;
    Mode mode;

    uint32_t pwm_pattern[Pwm_Size];
    uint8_t breathe_step = 0;
};
