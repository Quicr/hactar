#include "rgb_led.hh"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_tim.h"

static RGBLED* rgb = nullptr;

RGBLED::RGBLED(LED red, LED green, LED blue, TIM_HandleTypeDef& htim, DMA_HandleTypeDef& hdma) :
    red(red),
    green(green),
    blue(blue),
    htim(htim),
    hdma(hdma),
    active_colour(Colour::Red),
    mode(Mode::Off)
{
    rgb = this;
}

void RGBLED::On(const Colour colour, uint8_t brightness)
{
    LED& led = SelectLED(colour);
    if (brightness > 100)
    {
        brightness = 100;
    }

    uint8_t on_steps = (Pwm_Size * brightness) / 100;

    for (uint32_t i = 0; i < Pwm_Size; ++i)
    {
        if (on_steps > 0)
        {
            pwm_pattern[i] = led.BSSROnValue();
        }
        else
        {
            pwm_pattern[i] = led.BSSROffValue();
        }
    }

    mode = Mode::On;
    StartUpdater(colour);
}

void RGBLED::Off(const Colour colour)
{
    LED& led = SelectLED(colour);

    for (uint32_t i = 0; i < Pwm_Size; ++i)
    {
        pwm_pattern[i] = led.BSSROffValue();
    }

    mode = Mode::Off;
    StartUpdater(colour);
}

void RGBLED::Blink(const Colour colour)
{
}

void RGBLED::Strobe(const Colour colour)
{
}

void RGBLED::Breathe(const Colour colour)
{
}

void RGBLED::Callback(DMA_HandleTypeDef* hdma)
{
    if (rgb == nullptr)
    {
        return;
    }
}

void RGBLED::StartUpdater(const Colour colour)
{
    // Stop current updater
    if (colour != active_colour)
    {
        HAL_TIM_Base_Stop(&htim);
        active_colour = colour;
    }

    LED* led;
    switch (colour)
    {
    case Colour::Red:
        green.Off();
        blue.Off();
        led = &red;
    case Colour::Green:
        red.Off();
        blue.Off();
        led = &green;
    case Colour::Blue:
        red.Off();
        green.Off();
        led = &blue;
    }

    HAL_DMA_RegisterCallback(&hdma, HAL_DMA_XFER_CPLT_CB_ID, RGBLED::Callback);
    HAL_DMA_Start_IT(&hdma, (uint32_t)pwm_pattern, (uint32_t)led->BSSRAddr(), Pwm_Size);
    __HAL_TIM_ENABLE_DMA(&htim, TIM_DMA_UPDATE);
    HAL_TIM_Base_Start(&htim);
}

LED& RGBLED::SelectLED(const Colour colour)
{
    switch (colour)
    {
    case Colour::Red:
        return red;
    case Colour::Green:
        return green;
    case Colour::Blue:
        return blue;
    }

    return red;
}

void BreatheUpdate()
{
}
