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
    mode(Mode::Off),
    pwm_pattern{0},
    breathe_current_step(0),
    breathe_step(Step_Size),
    flash_status(FlashStatus::Off)
{
    rgb = this;
}

void RGBLED::SimpleOn(const Colour colour)
{
    LED& led = SelectLED(colour);
    led.On();
}

void RGBLED::SimpleOff(const Colour colour)
{
    LED& led = SelectLED(colour);
    led.Off();
}

void RGBLED::Yellow()
{
    red.On();
    green.On();
}

void RGBLED::AllOff()
{
    red.Off();
    green.Off();
    blue.Off();
}

void RGBLED::On(const Colour colour, uint8_t brightness)
{
    CalculateBrightness(colour, brightness);
    mode = Mode::On;
    HAL_DMA_UnRegisterCallback(&hdma, HAL_DMA_XFER_CPLT_CB_ID);
    StartUpdater(colour);
}

void RGBLED::Off(const Colour colour)
{
    LED& led = SelectLED(colour);

    for (uint32_t i = 0; i < Pwm_Size; ++i)
    {
        pwm_pattern[i] = led.BSRROffValue();
    }

    mode = Mode::Off;
    HAL_DMA_UnRegisterCallback(&hdma, HAL_DMA_XFER_CPLT_CB_ID);
    StartUpdater(colour);
}

void RGBLED::Breathe(const Colour colour)
{
    breathe_current_step = 0;
    breathe_step = Step_Size;
    mode = Mode::Breathe;
    htim.Init.Period = 199;
    BreatheUpdate(colour);
    HAL_DMA_RegisterCallback(&hdma, HAL_DMA_XFER_CPLT_CB_ID, RGBLED::Callback);
    StartUpdater(colour);
}

void RGBLED::Flash(const Colour colour, const Period period)
{
    flash_status = FlashStatus::Off;
    mode = Mode::Flash;
    ChangePeriod(period);
    FlashUpdate(colour);
    HAL_DMA_RegisterCallback(&hdma, HAL_DMA_XFER_CPLT_CB_ID, RGBLED::Callback);
    StartUpdater(colour);
}

void RGBLED::Callback(DMA_HandleTypeDef* hdma)
{
    if (rgb == nullptr)
    {
        return;
    }

    switch (rgb->mode)
    {
    case Mode::Off:
    {
        break;
    }
    case Mode::On:
    {
        break;
    }
    case Mode::Breathe:
    {
        rgb->BreatheUpdate(rgb->active_colour);
        break;
    }
    case Mode::Flash:
    {
        rgb->FlashUpdate(rgb->active_colour);
        break;
    }
    }
}

void RGBLED::StartUpdater(const Colour colour)
{
    // Stop current updater
    HAL_TIM_Base_Stop(&htim);
    HAL_TIM_Base_Init(&htim);
    active_colour = colour;

    LED* led;
    switch (colour)
    {
    case Colour::Red:
        green.Off();
        blue.Off();
        led = &red;
        break;
    case Colour::Green:
        red.Off();
        blue.Off();
        led = &green;
        break;
    case Colour::Blue:
        red.Off();
        green.Off();
        led = &blue;
        break;
    }

    HAL_DMA_Start_IT(&hdma, (uint32_t)pwm_pattern, (uint32_t)led->BSRRAddr(), Pwm_Size);
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

void RGBLED::BreatheUpdate(const Colour colour)
{
    CalculateBrightness(colour, breathe_current_step);
    breathe_current_step += breathe_step;
    if (breathe_current_step > 100)
    {
        breathe_step = -Step_Size;
        breathe_current_step = 100 - breathe_step;
    }
    else if (breathe_current_step < 5)
    {
        breathe_step = Step_Size;
        breathe_current_step = breathe_step;
    }
}

void RGBLED::FlashUpdate(const Colour colour)
{
    LED& led = SelectLED(colour);

    if (flash_status == FlashStatus::Off)
    {
        for (uint32_t i = 0; i < Pwm_Size; ++i)
        {
            pwm_pattern[i] = led.BSRROnValue();
        }
        flash_status = FlashStatus::On;
    }
    else
    {
        for (uint32_t i = 0; i < Pwm_Size; ++i)
        {
            pwm_pattern[i] = led.BSRROffValue();
        }
        flash_status = FlashStatus::Off;
    }
}

void RGBLED::CalculateBrightness(const Colour colour, uint8_t brightness)
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
            pwm_pattern[i] = led.BSRROnValue();
            on_steps -= 1;
        }
        else
        {
            pwm_pattern[i] = led.BSRROffValue();
        }
    }
}

void RGBLED::ChangePeriod(const Period period)
{

    switch (period)
    {
    case Period::Slow:
    {
        htim.Init.Period = 1999;
        break;
    }
    case Period::Medium:
    {
        htim.Init.Period = 999;
        break;
    }
    case Period::Fast:
    {
        htim.Init.Period = 499;
        break;
    }
    }
}
