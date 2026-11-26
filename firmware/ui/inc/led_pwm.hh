#pragma once

#include "led_driver.hh"
#include "stm32.h"
#include "stm32f4xx_hal_tim.h"
#include <cstdint>

class LEDPwm : public LEDDriver
{
private:
    union Effect
    {
        struct HoldState
        {
            uint16_t state;
        } hold;
        struct BreatheState
        {
            int32_t state;
            uint16_t low;
            uint16_t high;
            int32_t rate;
        } breathe;
        struct PulseState
        {
            uint16_t low;
            uint16_t high;
            uint16_t pattern_mask;
            uint16_t num_patterns;
            uint16_t bit;
        } pulse;
    };

    static constexpr uint16_t Num_Pattern_Bits =
        ((sizeof(Effect::PulseState::pattern_mask) * 8) - 1);

public:
    enum class Mode
    {
        Hold,
        Pulse,
        Breathe
    };

    LEDPwm(TIM_HandleTypeDef& htim, const uint32_t channel, const Polarity polarity);
    ~LEDPwm() = default;

    void Start();
    void Stop();

    // Note- Sets the mode to Hold
    void Set(const uint16_t value) override;
    void Pulse(const uint16_t low,
               const uint16_t high,
               const uint16_t pattern_mask,
               const uint16_t num_patterns);
    void Breathe(const uint16_t starting_point,
                 const uint16_t low,
                 const uint16_t high,
                 const int32_t rate);

    void Update();

private:
    void SetPwm(const uint16_t pwm);
    TIM_HandleTypeDef& timer;
    const uint32_t channel;
    Effect effect;
    Mode mode;
};
