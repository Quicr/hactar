#include "led_pwm.hh"
#include "led_driver.hh"
#include "stm32_hal_legacy.h"
#include "stm32f4xx_hal_tim.h"
#include <cstdint>

LEDPwm::LEDPwm(TIM_HandleTypeDef& htim, const uint32_t channel, const Polarity polarity) :
    LEDDriver(polarity),
    timer(htim),
    channel(channel)
{
}

void LEDPwm::Start()
{
    HAL_TIM_PWM_Start_IT(&timer, channel);
}

void LEDPwm::Stop()
{
    HAL_TIM_PWM_Stop(&timer, channel);
}

void LEDPwm::Set(const uint16_t value)
{
    effect.hold.state = value;
    mode = Mode::Hold;
    Update();
}

void LEDPwm::Pulse(const uint16_t low,
                   const uint16_t high,
                   const uint16_t pattern_mask,
                   const uint16_t num_patterns)
{
    effect.pulse.low = low;
    effect.pulse.high = high;
    effect.pulse.pattern_mask = pattern_mask;
    effect.pulse.num_patterns = num_patterns <= Num_Pattern_Bits ? num_patterns : Num_Pattern_Bits;
    effect.pulse.bit = 0;
    mode = Mode::Pulse;
    Update();
}

void LEDPwm::Breathe(const uint16_t starting_point,
                     const uint16_t low,
                     const uint16_t high,
                     const int32_t rate)
{
    effect.breathe.state = starting_point;
    effect.breathe.low = low;
    effect.breathe.high = high;
    effect.breathe.rate = rate;
    mode = Mode::Breathe;
    Update();
}

void LEDPwm::Update()
{
    switch (mode)
    {
    case Mode::Hold:
    {
        SetPwm(effect.hold.state);
        break;
    }
    case Mode::Pulse:
    {
        const uint16_t bit =
            (effect.pulse.pattern_mask >> ((Num_Pattern_Bits - effect.pulse.bit)) & 0x01);

        ++effect.pulse.bit;
        if (effect.pulse.bit >= effect.pulse.num_patterns)
        {
            effect.pulse.bit = 0;
        }

        SetPwm(bit ? effect.pulse.high : effect.pulse.low);

        break;
    }
    case Mode::Breathe:
    {
        if (effect.breathe.state + effect.breathe.rate <= effect.breathe.low)
        {
            effect.breathe.state = effect.breathe.low;
            effect.breathe.rate *= -1;
        }
        else if (effect.breathe.state + effect.breathe.rate >= effect.breathe.high)
        {
            effect.breathe.state = effect.breathe.high;
            effect.breathe.rate *= -1;
        }
        else
        {
            effect.breathe.state += effect.breathe.rate;
        }

        SetPwm(effect.breathe.state);

        break;
    }
    }
}

void LEDPwm::SetPwm(uint16_t pwm)
{
    switch (polarity)
    {
    case Polarity::Low:
    {
        pwm = (__HAL_TIM_GetAutoreload(&timer) + 1U) - pwm;
    }
    default:
        break;
    }

    __HAL_TIM_SET_COMPARE(&timer, channel, static_cast<uint32_t>(pwm));
}
