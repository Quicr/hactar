#include "led_pwm.hh"
#include "led_driver.hh"
#include "stm32_hal_legacy.h"
#include "stm32f4xx_hal_tim.h"
#include <sys/types.h>
#include <algorithm>
#include <cstdint>
#include <limits>

static constexpr uint16_t Pwm_Max = 4095;

LEDPwm::LEDPwm(TIM_HandleTypeDef& htim, const uint32_t channel, const Polarity polarity) :
    LEDDriver(polarity),
    timer(htim),
    channel(channel),
    effect{}
{
}

void LEDPwm::Start()
{
    HAL_TIM_PWM_Start(&timer, channel);
}

void LEDPwm::Stop()
{
    HAL_TIM_PWM_Stop(&timer, channel);
}

void LEDPwm::Set(const uint16_t value)
{
    effect.hold.prev_state = 0;
    effect.hold.state = value;

    mode = Mode::Hold;
    Update();
}

void LEDPwm::Toggle()
{
    if (mode != Mode::Hold)
    {
        return;
    }

    const uint16_t tmp = effect.hold.state;
    effect.hold.state = effect.hold.prev_state;
    effect.hold.prev_state = tmp;
}

void LEDPwm::Pulse(const uint16_t low,
                   const uint16_t high,
                   const uint16_t pattern_mask,
                   const uint16_t num_patterns,
                   const uint16_t timeout_ms)
{
    effect.pulse.low = low;
    effect.pulse.high = high;
    effect.pulse.pattern_mask = pattern_mask;
    effect.pulse.num_patterns = num_patterns <= Num_Pattern_Bits ? num_patterns : Num_Pattern_Bits;
    effect.pulse.timeout_ms = timeout_ms;
    effect.pulse.last_update_ms = 0;
    effect.pulse.bit = 0;
    mode = Mode::Pulse;
    Update();
}

void LEDPwm::Breathe(const uint16_t low, const uint16_t high, const int32_t rate)
{
    effect.breathe.state = low;
    effect.breathe.low = low;
    effect.breathe.high = high;
    effect.breathe.rate = rate;
    mode = Mode::Breathe;
    Update();
}

void LEDPwm::Update(const uint32_t ticks)
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
        if (ticks - effect.pulse.last_update_ms < effect.pulse.timeout_ms)
        {
            return;
        }
        effect.pulse.last_update_ms = ticks;

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

void LEDPwm::Update()
{
    Update(std::numeric_limits<uint32_t>().max());
}

void LEDPwm::SetPwm(uint16_t pwm)
{
    pwm = std::min(Pwm_Max, pwm);
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
